#ifndef __UGREEN_BSF_FILTER_H__
#define __UGREEN_BSF_FILTER_H__
#include "FFmpegUtils.h"
#include "BsfFilter.hpp"
#include <memory>

// Forward declarations
class H264BsfFilter;
class H265BsfFilter;

class UGreenBsfFilter: public IBsfFilter {
public:
    UGreenBsfFilter(AVCodecParameters* par);
    virtual ~UGreenBsfFilter() {}

    virtual int init(AVCodecParameters* codecPar);
    virtual int filter(AVPacket* pkt);
    virtual void deinit();

protected:
    std::shared_ptr<IBsfFilter> bsfFilter_;
};
#endif