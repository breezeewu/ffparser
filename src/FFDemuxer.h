#pragma once
#include <string>
#include <memory>
#include <vector>
#include <stdio.h>
#include <map>
#include "UGThread.h"
#include "FFmpegUtils.h"
#include "UGBsfFilter.h"
//#include "DashSink.h"

#define ENABLE_VIDEO 1
#define ENABLE_AUDIO 2
#define ENABLE_SUBTITLE 4

#define PARSER_FLAG_VIDEO 1
#define PARSER_FLAG_AUDIO 2
#define PARSER_FLAG_SUBTITLE 4

#define DECODER_FLAG_VIDEO 1
#define DECODER_FLAG_AUDIO 2
#define DECODER_FLAG_SUBTITLE 1
  class FFDemuxer:public UGThread, public IOnFrameReady
  {
  public:
    FFDemuxer();
    ~FFDemuxer() = default;

    int open(std::string path);

	void setDemuxerFlag(int flag, int dec_flag, int write_flag);

    int threadProc();

    int seek(int64_t offset);

    int pause(bool bpause);
  
    void close();
  
    int64_t duration();
  
    int deliverPacket(AVPacket* pkt);

	

	int64_t parserDuration(AVMediaType mt);

	int OnFrameReady(int index, AVFrame* pframe);

    int start(int64_t end_duration = INT64_MAX);

    void stop();

    // 获取格式上下文
    AVFormatContext* getFormatContext() const { return fmtCtx_; }

private:
	void flush(AVCodecContext* decCtx);

	int seekInternal(int64_t pos);

    void dumpPacket(AVPacket* pkt);
	void printPacket(AVPacket* pkt);

	void writePacket(AVPacket* pkt);
    void writeStream(AVPacket* pkt);

    void writeFrame(AVPacket* pkt);
    void decodePacket(AVCodecContext* decCtx, AVPacket* pkt);
    void writeFrame(int index, AVFrame* pframe);

	AVCodecContext* openDecCodecContext(AVStream* pst);

private:
    AVFormatContext* fmtCtx_ = NULL;
    int64_t seekOffset_ = 0;
    bool    seekFlag_ = false;
    bool    endOfStream_ = false;
    int64_t duration_ = 0;

    bool    bPause_ = false;
	std::string pcm_path_;
	std::string yuv_path_;
	int demux_flag_ = 0;
	int dec_flag_ = 0;
    int write_packet_ = 0;
	int64_t start_time = AV_NOPTS_VALUE;
    int64_t last_key_frame_pts = 0;
    int64_t end_duration_ = INT64_MAX;

	bool needSeek_ = true;
	FILE* pcm_file_ = NULL;
	std::map<int, FILE*> file_map_;
	std::map<int, FILE*> dec_map_;
	std::map<int, AVCodecContext*>	codecCtxList_;
	std::map<int, int64_t> lastPtsMap_;
    std::map<int, std::shared_ptr<IBsfFilter>> bsf_filt_map_;
    //std::shared_ptr<DashSink> sink_;
  };

