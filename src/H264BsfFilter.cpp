#include "H264BsfFilter.h"
#include <cstring>
#include <algorithm>
#include <assert.h>

H264BsfFilter::H264BsfFilter() : sps_pps_extracted_(false) {
    output_buffer_.reserve(1024 * 1024); // Reserve 1MB for output buffer
}

H264BsfFilter::~H264BsfFilter() {
    deinit();
}

int H264BsfFilter::init(AVCodecParameters* codecPar) {
    if (!codecPar) {
        MMLOGE("H264BsfFilter::init: codecPar is NULL");
        return -1;
    }
    
    if (codecPar->codec_id != AV_CODEC_ID_H264) {
        MMLOGE("H264BsfFilter::init: codec_id is not H264");
        return -1;
    }
    
    // Extract SPS/PPS from extradata if available
    if (codecPar->extradata && codecPar->extradata_size > 0) {
        parseAVCCHeader(codecPar->extradata, codecPar->extradata_size);
    }
    
    //MMLOGI("H264BsfFilter::init: initialized successfully");
    return 0;
}

int H264BsfFilter::filter(AVPacket* pkt) {
    if (!pkt || !pkt->data || pkt->size <= 4) {
        MMLOGE("H264BsfFilter::filter: invalid packet");
        return -1;
    }
    int nalsize = 0;
    for (int i = 0; i < 4; i++)
        nalsize = ((unsigned)nalsize << 8) | pkt->data[i];

    if (nalsize != pkt->size) {
        MMLOGW("Invalid nal size:%d, pkt->size:%d", nalsize, pkt->size);
    }
    // Check if data is already in Annex-B format (has start code)
    if (hasStartCode(pkt->data, pkt->size)) {
        MMLOGI("H264BsfFilter::filter: data already in Annex-B format");
        if (isIDRFrame(pkt->data, pkt->size)) {
            pkt->flags |= AV_PKT_FLAG_KEY;
        }
        return 0; // Already in Annex-B format, no conversion needed
    }
    
    return convertAVCCToAnnexB(pkt);
}

void H264BsfFilter::deinit() {
    sps_data_.clear();
    pps_data_.clear();
    output_buffer_.clear();
    sps_pps_extracted_ = false;
    MMLOGI("H264BsfFilter::deinit: cleaned up");
}

int H264BsfFilter::parseAVCCHeader(const uint8_t* data, int size) {
    if (!data || size < 7) {
        MMLOGE("H264BsfFilter::parseAVCCHeader: invalid data or size too small");
        return -1;
    }
    
    // AVCC format: [version(1)] [profile(1)] [compatibility(1)] [level(1)] [lengthSizeMinusOne(1)] [numOfSequenceParameterSets(1)] ...
    int offset = 5; // Skip version, profile, compatibility, level, lengthSizeMinusOne
    
    if (offset >= size) {
        MMLOGE("H264BsfFilter::parseAVCCHeader: data too short for AVCC header");
        return -1;
    }
    
    uint8_t num_sps = data[offset] & 0x1F;
    offset++;
    
    // Parse SPS
    for (int i = 0; i < num_sps && offset + 2 <= size; i++) {
        uint16_t sps_length = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        
        if (offset + sps_length <= size) {
            sps_data_.assign(data + offset, data + offset + sps_length);
            MMLOGI("H264BsfFilter::parseAVCCHeader: extracted SPS, length=%d", sps_length);
            offset += sps_length;
        } else {
            MMLOGE("H264BsfFilter::parseAVCCHeader: SPS data truncated");
            return -1;
        }
    }
    
    // Parse PPS
    if (offset + 1 <= size) {
        uint8_t num_pps = data[offset];
        offset++;
        
        for (int i = 0; i < num_pps && offset + 2 <= size; i++) {
            uint16_t pps_length = (data[offset] << 8) | data[offset + 1];
            offset += 2;
            
            if (offset + pps_length <= size) {
                pps_data_.assign(data + offset, data + offset + pps_length);
                MMLOGI("H264BsfFilter::parseAVCCHeader: extracted PPS, length=%d", pps_length);
                offset += pps_length;
            } else {
                MMLOGE("H264BsfFilter::parseAVCCHeader: PPS data truncated");
                return -1;
            }
        }
    }
    
    sps_pps_extracted_ = true;
    return 0;
}

int H264BsfFilter::convertAVCCToAnnexB(AVPacket* pkt) {

    output_buffer_.clear();
    bool key_frame = false;

    // Extract NAL units from AVCC format
    std::vector<std::vector<uint8_t>> nal_units;
    if (extractNALUnits(pkt->data, pkt->size, nal_units, key_frame) < 0) {
        MMLOGE("H264BsfFilter::convertAVCCToAnnexB: failed to extract NAL units");
        return -1;
    }
    if (key_frame) {
        // Add SPS and PPS if not already present in the packet
        if (sps_pps_extracted_ && !sps_data_.empty()) {
            addStartCode(output_buffer_, sps_data_.data(), sps_data_.size());
        }
    
        if (sps_pps_extracted_ && !pps_data_.empty()) {
            addStartCode(output_buffer_, pps_data_.data(), pps_data_.size());
        }
        pkt->flags |= AV_PKT_FLAG_KEY;
    }
    else if (pkt->flags & AV_PKT_FLAG_KEY) {
        MMLOGE("Invalid h264 key packet");
        assert(0);
    }
    // Convert each NAL unit to Annex-B format
    for (const auto& nal_unit : nal_units) {
        addStartCode(output_buffer_, nal_unit.data(), nal_unit.size());
    }
    
    // Update packet data
    if (output_buffer_.size() > pkt->size) {
        int ret = av_grow_packet(pkt, output_buffer_.size() - pkt->size);
        if (ret != 0) {
            MMLOGE("convertAVCCToAnnexB failed to allocate memory for packet data, ret:%d", ret);
            return -1;
        }
    }
    // Copy converted data
    memcpy(pkt->data, output_buffer_.data(), output_buffer_.size());

    pkt->size = output_buffer_.size();
    return 0;
}

int H264BsfFilter::addStartCode(std::vector<uint8_t>& output, const uint8_t* nal_data, int nal_size) {
    if (!nal_data || nal_size <= 0) {
        return -1;
    }
    
    // Add start code (0x00000001)
    output.push_back(0x00);
    output.push_back(0x00);
    output.push_back(0x00);
    output.push_back(0x01);
    
    // Add NAL unit data
    output.insert(output.end(), nal_data, nal_data + nal_size);
    
    return 0;
}

int H264BsfFilter::extractNALUnits(const uint8_t* data, int size, std::vector<std::vector<uint8_t>>& nal_units, bool& isKeyFrame) {
    if (!data || size <= 0) {
        return -1;
    }
    
    nal_units.clear();
    isKeyFrame = false;
    // Determine NAL unit length size (usually 4 bytes for AVCC)
    int nal_length_size = 4; // Default to 4 bytes
    
    int offset = 0;
    while (offset + nal_length_size <= size) {
        // Read NAL unit length
        uint32_t nal_length = 0;
        for (int i = 0; i < nal_length_size; i++) {
            nal_length = (nal_length << 8) | data[offset + i];
        }
        offset += nal_length_size;
        
        // Check if we have enough data for the NAL unit
        if (offset + nal_length <= size) {
            std::vector<uint8_t> nal_unit(data + offset, data + offset + nal_length);
            int nal_type = nal_unit[0] & 0x1f;
            if (H264_NAL_IDR_SLICE == nal_type) {
                isKeyFrame = true;
            }
            nal_units.push_back(std::move(nal_unit));
            offset += nal_length;
        } else {
            MMLOGE("H264BsfFilter::extractNALUnits: NAL unit truncated");
            return -1;
        }
    }
    
    //MMLOGI("H264BsfFilter::extractNALUnits: extracted %zd NAL units", nal_units.size());
    return 0;
}

bool H264BsfFilter::hasStartCode(const uint8_t* data, int size) {
    if (!data || size < 4) {
        return false;
    }
    int i = 0;
    while (data[i] == 0x00 && i < size) i++;
    if (i >= 2 && i <= 3 && data[i] == 1) {
        return true;
    }
    
    return false;
}

uint8_t H264BsfFilter::getNalUnitType(const uint8_t* nal_data, int nal_size) {
    if (!nal_data || nal_size < 1) {
        return H264_NAL_UNSPECIFIED;
    }
    
    return nal_data[0] & 0x1F;
}

const uint8_t* H264BsfFilter::skipStartCode(const uint8_t* data, int& size) {
    int i = 0;
    int zero_count = 0;
    for (int i = 0; i < size; i++) {
        if (data[i] == 0x00) {
            zero_count++;
        } else if (data[i] == 0x01 && zero_count >= 2) {
            size -= i + 1;
            return data + i + 1;
        }
        else {
            zero_count = 0;
        }

    }

    return NULL;
}

bool H264BsfFilter::isIDRFrame(const uint8_t* data, int size) {
    do {
        data = skipStartCode(data, size);
        if (data && isIDRNalType(data)) {
            return true;
        }
    } while (data && size > 4);
    return false;
}

bool H264BsfFilter::isIDRNalType(const uint8_t * data) {
    if (data && (data[0] & 0x1F) == H264_NAL_IDR_SLICE) {
        return true;
    }

    return false;
}