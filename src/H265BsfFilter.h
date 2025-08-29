#ifndef __H265_BSF_FILTER_H__
#define __H265_BSF_FILTER_H__

#include "BsfFilter.hpp"
#include <vector>
#include <memory>

// H.265 (HEVC) NAL Unit types
enum H265NalUnitType {
    H265_NAL_TRAIL_N = 0,
    H265_NAL_TRAIL_R = 1,
    H265_NAL_TSA_N = 2,
    H265_NAL_TSA_R = 3,
    H265_NAL_STSA_N = 4,
    H265_NAL_STSA_R = 5,
    H265_NAL_RADL_N = 6,
    H265_NAL_RADL_R = 7,
    H265_NAL_RASL_N = 8,
    H265_NAL_RASL_R = 9,
    H265_NAL_BLA_W_LP = 16,
    H265_NAL_BLA_W_RADL = 17,
    H265_NAL_BLA_N_LP = 18,
    H265_NAL_IDR_W_RADL = 19,
    H265_NAL_IDR_N_LP = 20,
    H265_NAL_CRA_NUT = 21,
    H265_NAL_VPS = 32,
    H265_NAL_SPS = 33,
    H265_NAL_PPS = 34,
    H265_NAL_AUD = 35,
    H265_NAL_EOS = 36,
    H265_NAL_EOB = 37,
    H265_NAL_FD = 38,
    H265_NAL_SEI_PREFIX = 39,
    H265_NAL_SEI_SUFFIX = 40
};

class H265BsfFilter : public IBsfFilter {
public:
    H265BsfFilter();
    virtual ~H265BsfFilter();

    virtual int init(AVCodecParameters* codecPar) override;
    virtual int filter(AVPacket* pkt) override;
    virtual void deinit() override;

private:
    // Parse HVCC format and extract VPS/SPS/PPS
    int parseHVCCHeader(const uint8_t* data, int size);
    
    // Convert HVCC packet to Annex-B format
    int convertHVCCToAnnexB(AVPacket* pkt);
    
    // Add start code (0x00000001) to NAL unit
    int addStartCode(std::vector<uint8_t>& output, const uint8_t* nal_data, int nal_size);
    
    // Extract NAL units from HVCC format
    int extractNALUnits(const uint8_t* data, int size, std::vector<std::vector<uint8_t>>& nal_units, bool& keyFrame);
    
    // Check if data contains start code
    bool hasStartCode(const uint8_t* data, int size);
    
    // Get NAL unit type
    uint8_t getNalUnitType(const uint8_t* nal_data, int nal_size);

private:
    std::vector<uint8_t> vps_data_;  // VPS NAL unit data
    std::vector<uint8_t> sps_data_;  // SPS NAL unit data
    std::vector<uint8_t> pps_data_;  // PPS NAL unit data
    bool vps_sps_pps_extracted_;     // Whether VPS/SPS/PPS have been extracted
    std::vector<uint8_t> output_buffer_; // Output buffer for converted data
};

#endif // __H265_BSF_FILTER_H__
