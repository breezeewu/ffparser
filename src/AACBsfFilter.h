#ifndef __AAC_BSF_FILTER_H__
#define __AAC_BSF_FILTER_H__

#include "BsfFilter.hpp"
#include <vector>
#include <memory>

// ADTS (Audio Data Transport Stream) header structure
struct ADTSHeader {
    uint8_t syncword[2];      // 0xFFF (12 bits)
    uint8_t id;               // MPEG identifier (1 bit)
    uint8_t layer;            // Layer (2 bits)
    uint8_t protection_absent; // Protection absent (1 bit)
    uint8_t profile;          // Profile (2 bits)
    uint8_t sampling_freq_index; // Sampling frequency index (4 bits)
    uint8_t private_bit;      // Private bit (1 bit)
    uint8_t channel_configuration; // Channel configuration (3 bits)
    uint8_t original_copy;    // Original/copy (1 bit)
    uint8_t home;             // Home (1 bit)
    uint8_t emphasis;         // Emphasis (2 bits)
    uint8_t copyright_id_bit; // Copyright ID bit (1 bit)
    uint8_t copyright_id_start; // Copyright ID start (1 bit)
    uint16_t frame_length;    // Frame length (13 bits)
    uint16_t adts_buffer_fullness; // ADTS buffer fullness (11 bits)
    uint8_t number_of_raw_data_blocks_in_frame; // Number of raw data blocks (2 bits)
};

class AACBsfFilter : public IBsfFilter {
public:
    AACBsfFilter();
    virtual ~AACBsfFilter();

    virtual int init(AVCodecParameters* codecPar) override;
    virtual int filter(AVPacket* pkt) override;
    virtual void deinit() override;

private:
    // Parse AAC configuration from extradata
    int parseAACConfig(const uint8_t* data, int size);
    
    // Convert AAC raw data to ADTS format
    int convertAACToADTS(AVPacket* pkt);
    
    // Build ADTS header
    int buildADTSHeader(uint8_t* header, int aac_frame_length);
    
    // Write ADTS header to output buffer
    int writeADTSHeader(std::vector<uint8_t>& output, int aac_frame_length);
    
    // Get sampling frequency index from sample rate
    uint8_t getSamplingFreqIndex(int sample_rate);
    
    // Get AAC profile from object type
    uint8_t getAACProfile(uint8_t object_type);

private:
    // AAC configuration parameters
    uint8_t aac_profile_;           // AAC profile (LC, HE, etc.)
    uint8_t sampling_freq_index_;   // Sampling frequency index
    uint8_t channel_config_;        // Channel configuration
    int sample_rate_;               // Sample rate in Hz
    int channels_;                  // Number of channels
    
    std::vector<uint8_t> output_buffer_; // Output buffer for ADTS data
    bool config_parsed_;            // Whether AAC config has been parsed
};

#endif // __AAC_BSF_FILTER_H__
