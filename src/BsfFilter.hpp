#ifndef __AVC_TO_ANNEXB_HPP__
#define __AVC_TO_ANNEXB_HPP__
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif
#include "FFmpegUtils.h"

class IBsfFilter {
public:
    virtual ~IBsfFilter() {}
    virtual int init(AVCodecParameters* codecPar) = 0;
    virtual int filter(AVPacket* pkt) = 0;
    virtual void deinit() = 0;
};

class BsfFilter:public IBsfFilter
{
public:
    BsfFilter(AVCodecParameters* codecPar = NULL)
    {
        if (codecPar) {
            init(codecPar);
        }
    }
    ~BsfFilter(){
        deinit();
    }

    int init(AVCodecParameters* codecPar)
    {
        const AVBitStreamFilter* bsf = NULL;
        const char* pbsfName = NULL;
        if(AV_CODEC_ID_H264 == codecPar->codec_id)
        {
            pbsfName = "h264_mp4toannexb";
        }
        else if(AV_CODEC_ID_HEVC == codecPar->codec_id)
        {
            pbsfName = "hevc_mp4toannexb";
        }
        else if(AV_CODEC_ID_AAC == codecPar->codec_id)
        {
            pbsfName = "aac_adtstoasc";
        }
        bsf = av_bsf_get_by_name(pbsfName);
        if(NULL == bsf)
        {
            MMLOGE("bsf:%p = av_bsf_get_by_name(pbsfName:%s) failed", bsf, pbsfName);
            return -1;
        }
        MMLOGI("bsf:%p = av_bsf_get_by_name(pbsfName:%s)", bsf, pbsfName);
        av_bsf_alloc(bsf, &bsfCtx_);
        if(NULL == bsfCtx_)
        {
            MMLOGE("av_bsf_alloc(bsf:%p, &bsfCtx:%p)",  bsf, bsfCtx_);
            return -1;
        }

        avcodec_parameters_copy(bsfCtx_->par_in, codecPar);
        av_bsf_init(bsfCtx_);
        return 0;
    }

    int filter(AVPacket* pkt)
    {
        if(NULL == bsfCtx_)
        {
            MMLOGE("bsf context not init, bsfCtx:%p", bsfCtx_);
            return -1;
        }
        int ret = av_bsf_send_packet(bsfCtx_, pkt);
        if(ret != 0)
        {
            MMLOGE("ret:%d = av_bsf_send_packet(bsfCtx_, pkt) failed, reason:%s", ret, ffmpegErrStr(ret));
            return 0;
        }
        av_packet_unref(pkt);
        ret = av_bsf_receive_packet(bsfCtx_, pkt);
        if(0 != ret)
        {
            MMLOGE("ret:%d = av_bsf_receive_packet(bsfCtx_, pkt) failed, reason:%s", ret, ffmpegErrStr(ret));
            return ret;
        }

        //MMLOGI("pkt index:%d data:%02x%02x%02x%02x", pkt->stream_index, pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3]);
        return 0;

    }

    void deinit()
    {
        if(bsfCtx_)
        {
            //MMLOGI("av_bsf_free(&bsfCtx:%p)", bsfCtx_);
            av_bsf_free(&bsfCtx_);
            bsfCtx_ = NULL;
        }
    }
private:
    //AVBitStreamFilter* bsf_ = NULL;
    AVBSFContext* bsfCtx_ = NULL;
};
#endif