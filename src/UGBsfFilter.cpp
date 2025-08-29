#include "UGBsfFilter.h"
#include "H264BsfFilter.h"
#include "H265BsfFilter.h"
#include "AACBsfFilter.h"

UGreenBsfFilter::UGreenBsfFilter(AVCodecParameters* par)
{
    if (par) {
        //bsfFilter_ = std::make_shared<BsfFilter>(par);
        init(par);
    }
}

int UGreenBsfFilter::init(AVCodecParameters* codecPar)
{
    if(!codecPar) {
        return -1;
    }

    if(AV_CODEC_ID_H264 == codecPar->codec_id) {
        bsfFilter_ = std::make_shared<H264BsfFilter>();
    } else if(AV_CODEC_ID_HEVC == codecPar->codec_id) {
        bsfFilter_ = std::make_shared<H265BsfFilter>();
    } else if(AV_CODEC_ID_AAC == codecPar->codec_id) {
        bsfFilter_ = std::make_shared<AACBsfFilter>();
    } else {
        return -1;
    }
    return bsfFilter_->init(codecPar);
}

int UGreenBsfFilter::filter(AVPacket* pkt)
{
    if(bsfFilter_)
    {
        return bsfFilter_->filter(pkt);
    }
    return -1;
}

void UGreenBsfFilter::deinit()
{
    if(bsfFilter_)
    {
        bsfFilter_->deinit();
    }
}
