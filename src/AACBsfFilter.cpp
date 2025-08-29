#include "AACBsfFilter.h"
#include <cstring>
#include <algorithm>

AACBsfFilter::AACBsfFilter() : aac_profile_(0), sampling_freq_index_(0), channel_config_(0), 
                               sample_rate_(0), channels_(0), config_parsed_(false) {
    output_buffer_.reserve(1024 * 1024); // Reserve 1MB for output buffer
}

AACBsfFilter::~AACBsfFilter() {
    deinit();
}

int AACBsfFilter::init(AVCodecParameters* codecPar) {
    if (!codecPar) {
        MMLOGE("AACBsfFilter::init: codecPar is NULL");
        return -1;
    }
    
    if (codecPar->codec_id != AV_CODEC_ID_AAC) {
        MMLOGE("AACBsfFilter::init: codec_id is not AAC");
        return -1;
    }
    
    // Store basic parameters
    sample_rate_ = codecPar->sample_rate;
    channels_ = codecPar->ch_layout.nb_channels;
    
    // Parse AAC configuration from extradata if available
    if (codecPar->extradata && codecPar->extradata_size >= 2) {
        parseAACConfig(codecPar->extradata, codecPar->extradata_size);
    } else {
        // Use default values if no extradata
        sampling_freq_index_ = getSamplingFreqIndex(sample_rate_);
        channel_config_ = channels_;
        aac_profile_ = 1; // AAC-LC profile
        config_parsed_ = true;
        MMLOGI("AACBsfFilter::init: using default AAC configuration");
    }
    
    MMLOGI("AACBsfFilter::init: initialized successfully, sample_rate=%d, channels=%d", 
           sample_rate_, channels_);
    return 0;
}

int AACBsfFilter::filter(AVPacket* pkt) {
    if (!pkt || !pkt->data || pkt->size <= 0) {
        MMLOGE("AACBsfFilter::filter: invalid packet");
        return -1;
    }
    
    // Check if data already has ADTS header (starts with 0xFFF)
    if (pkt->size >= 2 && (pkt->data[0] == 0xFF && (pkt->data[1] & 0xF0) == 0xF0)) {
        MMLOGI("AACBsfFilter::filter: data already in ADTS format");
        return 0; // Already in ADTS format, no conversion needed
    }
    
    return convertAACToADTS(pkt);
}

void AACBsfFilter::deinit() {
    output_buffer_.clear();
    config_parsed_ = false;
    MMLOGI("AACBsfFilter::deinit: cleaned up");
}

int AACBsfFilter::parseAACConfig(const uint8_t* data, int size) {
    if (!data || size < 2) {
        MMLOGE("AACBsfFilter::parseAACConfig: invalid data or size too small");
        return -1;
    }
    
    // AAC configuration format: [object_type(5)] [sample_rate_index(4)] [channel_config(4)] [GASpecificConfig]
    uint8_t object_type = (data[0] >> 3) & 0x1F;
    uint8_t sample_rate_index = ((data[0] & 0x07) << 1) | ((data[1] >> 7) & 0x01);
    uint8_t channel_config = (data[1] >> 3) & 0x0F;
    
    // Convert object type to ADTS profile
    aac_profile_ = getAACProfile(object_type);
    
    // Store sampling frequency index
    sampling_freq_index_ = sample_rate_index;
    
    // Store channel configuration
    channel_config_ = channel_config;
    
    config_parsed_ = true;
    
    MMLOGI("AACBsfFilter::parseAACConfig: object_type=%d, sample_rate_index=%d, channel_config=%d", 
           object_type, sample_rate_index, channel_config);
    
    return 0;
}

int AACBsfFilter::convertAACToADTS(AVPacket* pkt) {
    if (!config_parsed_) {
        MMLOGE("AACBsfFilter::convertAACToADTS: AAC config not parsed");
        return -1;
    }
    
    // Clear output buffer
    output_buffer_.clear();
    
    // Write ADTS header
    int aac_frame_length = pkt->size + 7; // ADTS header is 7 bytes
    writeADTSHeader(output_buffer_, aac_frame_length);
    
    // Copy AAC data
    output_buffer_.insert(output_buffer_.end(), pkt->data, pkt->data + pkt->size);
    
    // Update packet data
    /*if (pkt->data) {
        av_free(pkt->data);
    }*/
    int ret = av_grow_packet(pkt, output_buffer_.size() - pkt->size);
    //pkt->data = (uint8_t*)av_malloc(output_buffer_.size());
    if (ret != 0) {
        MMLOGE("AACBsfFilter::convertAACToADTS: failed to allocate memory for packet data, ret:%d", ret);
        return -1;
    }
    
    memcpy(pkt->data, output_buffer_.data(), output_buffer_.size());
    pkt->size = output_buffer_.size();
    
    //MMLOGI("AACBsfFilter::convertAACToADTS: converted %d bytes to ADTS format", pkt->size);
    
    return 0;
}

int AACBsfFilter::buildADTSHeader(uint8_t* header, int aac_frame_length) {
    if (!header) {
        MMLOGE("AACBsfFilter::buildADTSHeader: header is NULL");
        return -1;
    }
    
    // Clear header
    memset(header, 0, 7);
    
    // Syncword: 0xFFF (12 bits)
    header[0] = 0xFF;
    header[1] = 0xF0;
    
    // ID: 0 for MPEG-4, 1 for MPEG-2
    header[1] |= 0x00; // MPEG-4
    
    // Layer: always 0 for AAC
    header[1] |= 0x00;
    
    // Protection absent: 1 (no CRC)
    header[1] |= 0x01;
    
    // Profile: AAC profile (2 bits)
    header[2] = (aac_profile_ & 0x03) << 6;
    
    // Sampling frequency index (4 bits)
    header[2] |= (sampling_freq_index_ & 0x0F) << 2;
    
    // Private bit: 0
    header[2] |= 0x00;
    
    // Channel configuration (3 bits)
    header[2] |= (channel_config_ >> 1) & 0x01;
    header[3] = (channel_config_ & 0x01) << 7;
    
    // Original/copy: 0
    header[3] |= 0x00;
    
    // Home: 0
    header[3] |= 0x00;
    
    // Emphasis: 0
    header[3] |= 0x00;
    
    // Copyright ID bit: 0
    header[3] |= 0x00;
    
    // Copyright ID start: 0
    header[3] |= 0x00;
    
    // Frame length (13 bits)
    header[3] |= (aac_frame_length >> 11) & 0x03;
    header[4] = (aac_frame_length >> 3) & 0xFF;
    header[5] = (aac_frame_length & 0x07) << 5;
    
    // ADTS buffer fullness (11 bits)
    header[5] |= 0x7FF >> 6; // 0x7FF = 2047 (full buffer)
    header[6] = 0x7F;
    
    // Number of raw data blocks in frame: 0
    header[6] |= 0x00;
    
    return 0;
}

int AACBsfFilter::writeADTSHeader(std::vector<uint8_t>& output, int aac_frame_length) {
    uint8_t header[7];
    int ret = buildADTSHeader(header, aac_frame_length);
    if (ret != 0) {
        return ret;
    }
    
    output.insert(output.end(), header, header + 7);
    return 0;
}

uint8_t AACBsfFilter::getSamplingFreqIndex(int sample_rate) {
    // AAC sampling frequency index table
    switch (sample_rate) {
        case 96000: return 0;
        case 88200: return 1;
        case 64000: return 2;
        case 48000: return 3;
        case 44100: return 4;
        case 32000: return 5;
        case 24000: return 6;
        case 22050: return 7;
        case 16000: return 8;
        case 12000: return 9;
        case 11025: return 10;
        case 8000:  return 11;
        case 7350:  return 12;
        default:    return 4; // Default to 44.1kHz
    }
}

uint8_t AACBsfFilter::getAACProfile(uint8_t object_type) {
    // AAC object type to ADTS profile mapping
    switch (object_type) {
        case 1: return 0; // AAC Main
        case 2: return 1; // AAC LC (Low Complexity)
        case 3: return 2; // AAC SSR (Scalable Sample Rate)
        case 4: return 3; // AAC LTP (Long Term Prediction)
        case 5: return 4; // AAC SBR (Spectral Band Replication)
        case 6: return 5; // AAC Scalable
        case 7: return 6; // TwinVQ
        case 8: return 7; // CELP
        case 9: return 8; // HXVC
        default: return 1; // Default to AAC LC
    }
}
