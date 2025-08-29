#ifndef __H264_BSF_FILTER_H__
#define __H264_BSF_FILTER_H__

#include "BsfFilter.hpp"
#include <vector>
#include <memory>

// H.264 NAL Unit types
enum H264NalUnitType {
    H264_NAL_UNSPECIFIED = 0,
    H264_NAL_SLICE = 1,
    H264_NAL_DPA = 2,
    H264_NAL_DPB = 3,
    H264_NAL_DPC = 4,
    H264_NAL_IDR_SLICE = 5,
    H264_NAL_SEI = 6,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8,
    H264_NAL_AUD = 9,
    H264_NAL_END_SEQUENCE = 10,
    H264_NAL_END_STREAM = 11,
    H264_NAL_FILLER_DATA = 12,
    H264_NAL_SPS_EXT = 13,
    H264_NAL_PREFIX = 14,
    H264_NAL_SUBSET_SPS = 15,
    H264_NAL_DEPTH_PARAMETER_SET = 16,
    H264_NAL_RESERVED_17 = 17,
    H264_NAL_RESERVED_18 = 18,
    H264_NAL_AUXILIARY_SLICE = 19,
    H264_NAL_RESERVED_20 = 20,
    H264_NAL_RESERVED_21 = 21,
    H264_NAL_RESERVED_22 = 22,
    H264_NAL_RESERVED_23 = 23,
    H264_NAL_UNSPECIFIED_24 = 24,
    H264_NAL_UNSPECIFIED_25 = 25,
    H264_NAL_UNSPECIFIED_26 = 26,
    H264_NAL_UNSPECIFIED_27 = 27,
    H264_NAL_UNSPECIFIED_28 = 28,
    H264_NAL_UNSPECIFIED_29 = 29,
    H264_NAL_UNSPECIFIED_30 = 30,
    H264_NAL_UNSPECIFIED_31 = 31
};

class H264BsfFilter : public IBsfFilter {
public:
    H264BsfFilter();
    virtual ~H264BsfFilter();

    virtual int init(AVCodecParameters* codecPar) override;
    virtual int filter(AVPacket* pkt) override;
    virtual void deinit() override;

private:
    // Parse AVCC format and extract SPS/PPS
    int parseAVCCHeader(const uint8_t* data, int size);
    
    // Convert AVCC packet to Annex-B format
    int convertAVCCToAnnexB(AVPacket* pkt);
    
    // Add start code (0x00000001) to NAL unit
    int addStartCode(std::vector<uint8_t>& output, const uint8_t* nal_data, int nal_size);
    
    // Extract NAL units from AVCC format
    int extractNALUnits(const uint8_t* data, int size, std::vector<std::vector<uint8_t>>& nal_units, bool& isKeyFrame);
    
    // Check if data contains start code
    bool hasStartCode(const uint8_t* data, int size);
    
    // Get NAL unit type
    uint8_t getNalUnitType(const uint8_t* nal_data, int nal_size);

    const uint8_t* skipStartCode(const uint8_t* data, int& size);

    bool isIDRFrame(const uint8_t* data, int size);

    bool isIDRNalType(const uint8_t* data);

private:
    std::vector<uint8_t> sps_data_;  // SPS NAL unit data
    std::vector<uint8_t> pps_data_;  // PPS NAL unit data
    bool sps_pps_extracted_;         // Whether SPS/PPS have been extracted
    std::vector<uint8_t> output_buffer_; // Output buffer for converted data
};

#endif // __H264_BSF_FILTER_H__
