#pragma once
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
//#include <libavformat/internal.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/display.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#ifdef __cplusplus
}
#endif
#include <stdio.h>
#define MMLOGI printf
#define MMLOGW printf
#define MMLOGE printf

class IOnFrameReady
{
public:
		virtual int OnFrameReady(int index, AVFrame* pframe) = 0;
};
constexpr AVRational kFFmpegMilliSecondsBaseQ = {1, 1000};

const char* ffmpegErrStr(int averr);

int64_t timeScale(int64_t pts, const AVRational* dst_ral, const AVRational* src_ral);

void dumpPacketInfo(std::string name, const AVPacket* pkt);

void dumpSubtitleInfo(std::string name, AVSubtitle* sub);

void avPacketTimeScale(AVPacket* pkt, const AVRational* dst_ral, const AVRational* src_ral);

bool isTextSubtitle(int codec_id);

void av_log_cb(void *ptr, int level, const char *fmt, va_list vl);

std::string getCodecNameByCodecID(AVCodecID codec_id);

void avFrameTimeScale(AVFrame* frame, const AVRational* dst_ral, const AVRational* src_ral);

std::string dumpFrame(const std::string str, AVFrame* frame, const AVRational* ptime_base);

int writeFrameFromPacket(AVCodecContext* decCtx, AVPacket* pkt, const char* path);

int DecoderPacket(AVCodecContext* decCtx, AVPacket* pkt, IOnFrameReady* frame_ready);

int writeFrameFileFromPacket(AVCodecContext* decCtx, AVPacket* pkt, FILE* pfile);

int flushCodecContext(AVCodecContext* codecCtx);

// Write AVFrame data to FILE handle, supports audio PCM and video YUV data, including hardware decoded frames
int writeFrameToFile(AVFrame* frame, FILE* pfile);

void dumpAVCodecParameters(AVCodecParameters* par);

void dumpAVCodecContext(AVCodecContext* ctx);

int64_t getStartTimeStamp(AVFormatContext* fmtCtx);

int64_t getEndTimeStamp(AVFormatContext* fmtCtx);

int64_t readPacketDuration(AVFormatContext* fmtCtx);

AVCodecContext* openDecoder(AVCodecParameters* par);
