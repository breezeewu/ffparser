#include "H265BsfFilter.h"
#include <cstring>
#include <algorithm>

H265BsfFilter::H265BsfFilter() : vps_sps_pps_extracted_(false) {
    output_buffer_.reserve(1024 * 1024); // Reserve 1MB for output buffer
}

H265BsfFilter::~H265BsfFilter() {
    deinit();
}

int H265BsfFilter::init(AVCodecParameters* codecPar) {
    if (!codecPar) {
        MMLOGE("H265BsfFilter::init: codecPar is NULL");
        return -1;
    }
    
    if (codecPar->codec_id != AV_CODEC_ID_HEVC) {
        MMLOGE("H265BsfFilter::init: codec_id is not HEVC");
        return -1;
    }
    
    // Extract VPS/SPS/PPS from extradata if available
    if (codecPar->extradata && codecPar->extradata_size > 0) {
        parseHVCCHeader(codecPar->extradata, codecPar->extradata_size);
    }
    
    //MMLOGI("H265BsfFilter::init: initialized successfully");
    return 0;
}

int H265BsfFilter::filter(AVPacket* pkt) {
    if (!pkt || !pkt->data || pkt->size <= 0) {
        MMLOGE("H265BsfFilter::filter: invalid packet");
        return -1;
    }
    
    // Check if data is already in Annex-B format (has start code)
    if (hasStartCode(pkt->data, pkt->size)) {
        MMLOGI("H265BsfFilter::filter: data already in Annex-B format");
        return 0; // Already in Annex-B format, no conversion needed
    }
    
    return convertHVCCToAnnexB(pkt);
}

void H265BsfFilter::deinit() {
    vps_data_.clear();
    sps_data_.clear();
    pps_data_.clear();
    output_buffer_.clear();
    vps_sps_pps_extracted_ = false;
    MMLOGI("H265BsfFilter::deinit: cleaned up");
}

int H265BsfFilter::parseHVCCHeader(const uint8_t* data, int size) {
    if (!data || size < 23) { // Minimum HVCC header size
        MMLOGE("H265BsfFilter::parseHVCCHeader: invalid data or size too small");
        return -1;
    }
    
    // HVCC format structure:
    // [configurationVersion(1)] [general_profile_space(2) + general_tier_flag(1) + general_profile_idc(5)(1)]
    // [general_profile_compatibility_flags(4)] [general_constraint_indicator_flags(6)]
    // [general_level_idc(1)] [min_spatial_segmentation_idc(2)] [parallelismType(2)]
    // [chromaFormat(2)] [bitDepthLumaMinus8(3)] [bitDepthChromaMinus8(3)]
    // [avgFrameRate(2)] [constantFrameRate(2) + numTemporalLayers(3) + temporalIdNested(1)(1)]
    // [lengthSizeMinusOne(2)] [numOfArrays(1)] ...
    
    int offset = 22; // Skip to numOfArrays
    
    if (offset >= size) {
        MMLOGE("H265BsfFilter::parseHVCCHeader: data too short for HVCC header");
        return -1;
    }
    
    uint8_t num_arrays = data[offset];
    offset++;
    
    // Parse each parameter set array
    for (int i = 0; i < num_arrays && offset + 3 <= size; i++) {
        uint8_t array_completeness = (data[offset] >> 7) & 0x01;
        uint8_t reserved = (data[offset] >> 1) & 0x3F;
        // Correct NAL unit type extraction: bits 1-6 of the first byte
        uint8_t nal_unit_type = data[offset] & 0x3f;//(data[offset] >> 1) & 0x3F;
        offset++;
        
        if (offset + 2 > size) {
            MMLOGE("H265BsfFilter::parseHVCCHeader: insufficient data for num_nal_units");
            return -1;
        }
        
        uint16_t num_nal_units = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        
        // Parse NAL units for this array
        for (int j = 0; j < num_nal_units && offset + 2 <= size; j++) {
            uint16_t nal_unit_length = (data[offset] << 8) | data[offset + 1];
            offset += 2;
            
            if (offset + nal_unit_length <= size) {
                // Store VPS, SPS, or PPS based on NAL unit type
                switch (nal_unit_type) {
                    case H265_NAL_VPS:
                        vps_data_.assign(data + offset, data + offset + nal_unit_length);
                        //MMLOGI("H265BsfFilter::parseHVCCHeader: extracted VPS, length=%d", nal_unit_length);
                        break;
                    case H265_NAL_SPS:
                        sps_data_.assign(data + offset, data + offset + nal_unit_length);
                        //MMLOGI("H265BsfFilter::parseHVCCHeader: extracted SPS, length=%d", nal_unit_length);
                        break;
                    case H265_NAL_PPS:
                        pps_data_.assign(data + offset, data + offset + nal_unit_length);
                        //MMLOGI("H265BsfFilter::parseHVCCHeader: extracted PPS, length=%d", nal_unit_length);
                        break;
                    default:
                        MMLOGI("H265BsfFilter::parseHVCCHeader: ignored NAL unit type %d, length=%d", 
                               nal_unit_type, nal_unit_length);
                        break;
                }
                offset += nal_unit_length;
            } else {
                MMLOGE("H265BsfFilter::parseHVCCHeader: NAL unit data truncated");
                return -1;
            }
        }
    }
    
    vps_sps_pps_extracted_ = true;
    return 0;
}

int H265BsfFilter::convertHVCCToAnnexB(AVPacket* pkt) {
    output_buffer_.clear();

    // Extract NAL units from HVCC format
    std::vector<std::vector<uint8_t>> nal_units;
    bool key_frame = false;
    if (extractNALUnits(pkt->data, pkt->size, nal_units, key_frame) < 0) {
        MMLOGE("H265BsfFilter::convertHVCCToAnnexB: failed to extract NAL units");
        return -1;
    }
    if (key_frame) {
        // Add VPS, SPS, and PPS if not already present in the packet
        if (vps_sps_pps_extracted_ && !vps_data_.empty()) {
            addStartCode(output_buffer_, vps_data_.data(), vps_data_.size());
        }

        if (vps_sps_pps_extracted_ && !sps_data_.empty()) {
            addStartCode(output_buffer_, sps_data_.data(), sps_data_.size());
        }

        if (vps_sps_pps_extracted_ && !pps_data_.empty()) {
            addStartCode(output_buffer_, pps_data_.data(), pps_data_.size());
        }
    }
    // Convert each NAL unit to Annex-B format
    for (const auto& nal_unit : nal_units) {
        addStartCode(output_buffer_, nal_unit.data(), nal_unit.size());
    }
    
    // Update packet data
    if (output_buffer_.size() > 0) {
        // reallocate new buffer for packet data
        int ret = av_grow_packet(pkt, output_buffer_.size() - pkt->size);
        if (ret != 0) {
            MMLOGE("convertHVCCToAnnexB failed to allocate memory for packet data, ret:%d", ret);
            return -1;
        }
    }
    // Copy converted data
        memcpy(pkt->data, output_buffer_.data(), output_buffer_.size());
        pkt->size = output_buffer_.size();
    return 0;
}

int H265BsfFilter::addStartCode(std::vector<uint8_t>& output, const uint8_t* nal_data, int nal_size) {
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

int H265BsfFilter::extractNALUnits(const uint8_t* data, int size, std::vector<std::vector<uint8_t>>& nal_units, bool& keyFrame) {
    if (!data || size <= 0) {
        return -1;
    }

    nal_units.clear();
    keyFrame = false;
    // Determine NAL unit length size (usually 4 bytes for HVCC)
    int nal_length_size = 4; // Default to 4 bytes

    int offset = 0;
    int nal_type = 0;
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
            nal_type = (nal_unit[0] >> 1) & 0x3F;  // Consistent with getNalUnitType
            if (H265_NAL_BLA_W_LP <= nal_type && nal_type <=H265_NAL_CRA_NUT) {
                keyFrame = true;
            }
            nal_units.push_back(std::move(nal_unit));
            offset += nal_length;
        } else {
            MMLOGE("H265BsfFilter::extractNALUnits: NAL unit truncated");
            return -1;
        }
    }

    //MMLOGI("H265BsfFilter::extractNALUnits: extracted %d NAL units", nal_units.size());
    return 0;
}

bool H265BsfFilter::hasStartCode(const uint8_t* data, int size) {
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

uint8_t H265BsfFilter::getNalUnitType(const uint8_t* nal_data, int nal_size) {
    if (!nal_data || nal_size < 2) {
        return H265_NAL_TRAIL_N; // Default to trail N
    }
    
    // HEVC NAL unit type is in the first byte, bits 1-6
    return (nal_data[0] >> 1) & 0x3F;
}
