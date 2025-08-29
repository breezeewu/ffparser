#include "FFDemuxer.h"

//#include "media_muxer/MediaMuxer.h"//media_bin/interface/../media_bin/
FFDemuxer::FFDemuxer()
{
	av_log_set_level(AV_LOG_INFO);
	av_log_set_callback(av_log_cb);
}
static enum AVPixelFormat get_hw_format(AVCodecContext* ctx,
	const enum AVPixelFormat* pix_fmts)
{
	//FFDemuxer* dec = (FFDemuxer*)ctx->opaque;
	const enum AVPixelFormat* p;
	for (p = pix_fmts; *p != -1; p++) {
		if (*p == AV_PIX_FMT_VAAPI)
		{
			return *p;
		}
	}
	return AV_PIX_FMT_NONE;
}
int getStreamMetadata(AVDictionary* metadata, std::string* title, std::string* language)
{
	AVDictionaryEntry* lang = NULL;
	if (metadata)
	{
		if (title) {
			AVDictionaryEntry* lang = av_dict_get(metadata, "title", NULL, 0);
			if (lang)
			{
				*title = lang->value;
			}
		}

		if (language) {
			//av_dict_get(&metadata, "language", language->c_str(), 0);
			AVDictionaryEntry* lang = av_dict_get(metadata, "language", NULL, 0);
			if (lang)
			{
				*language = lang->value;
			}
			// av_dict_set(&pst->metadata, "language", psti->getLanguage().c_str(), 0);
		}
	}
	return 0;
}

int setStreamMetadata(AVDictionary** ppmetadata, std::string* title, std::string* language)
{
	if (title) {
		av_dict_set(ppmetadata, "title", title->c_str(), 0);
	}

	if (language) {
		av_dict_set(ppmetadata, "language", language->c_str(), 0);
	}

	return 0;
}
void dumpMetadata(AVStream* pst)
{
	const AVDictionaryEntry* entry = NULL;
	while (entry = av_dict_iterate(pst->metadata, entry)) {
		printf("index:%d, id:%d, key:%s, value:%s\n", pst->index, pst->id, entry->key, entry->value);
	};
}

AVSampleFormat* getCodecContextSampleFormat(AVCodecContext* codecCtx)
{
	AVSampleFormat* sample_fmts = NULL;
	int fmt_count = 0;
	//avcodec_get_supported_config(codecCtx, codecCtx->codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, &sample_fmts, &fmt_count);

	return sample_fmts;
}

int FFDemuxer::open(std::string path)
{
    int ret = avformat_open_input(&fmtCtx_, path.c_str(), NULL, NULL);
    if(ret != 0)
    {
        MMLOGE("avformat_open_input(&fmtCtx:%p, path:%s, NULL, NULL) failed, ret:%d", fmtCtx_, path.c_str(), ret);
        return ret;
    }

	int file_size = avio_size(fmtCtx_->pb);
	fmtCtx_->probesize = 15 * 1024 * 1024;
	ret = avformat_find_stream_info(fmtCtx_, NULL);
    if(ret != 0)
    {
        MMLOGE("avformat_find_stream_info(fmtCtx:%p, NULL) failed, ret:%d", fmtCtx_, ret);
        return ret;
    }
    //av_dump_format(fmtCtx_, 0, NULL, 0);
	for (int i = 0; i < fmtCtx_->nb_streams; i++)
	{
        AVStream* pst = fmtCtx_->streams[i];
		AVCodecParameters* par = fmtCtx_->streams[i]->codecpar;
		std::string title, lang;
		getStreamMetadata(fmtCtx_->streams[i]->metadata, &title, &lang);
		printf("index:%d, codec_type:%d id:%d title:%s, lang:%s, start_time:%" PRId64 ", duration:%" PRId64 "\n", 
            i, par->codec_type, par->codec_id, title.c_str(), lang.c_str(), timeScale(fmtCtx_->streams[i]->start_time, NULL, &fmtCtx_->streams[i]->time_base), 
            av_rescale_q(pst->duration, pst->time_base, AV_TIME_BASE_Q));

		dumpMetadata(fmtCtx_->streams[i]);
		if (AVMEDIA_TYPE_VIDEO == par->codec_type && (fmtCtx_->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
		{
            bsf_filt_map_[i] = std::make_shared<UGreenBsfFilter>(par);
            MMLOGI("codec:%s, width:%d, height:%d", avcodec_get_name(par->codec_id), par->width, par->height);
            
		}
		if (AVMEDIA_TYPE_AUDIO == par->codec_type)
		{
            bsf_filt_map_[i] = std::make_shared<UGreenBsfFilter>(par);
			dumpAVCodecParameters(par);
            MMLOGI("codec:%s, channel:%d, samplerate:%d, duration:%" PRId64 "", avcodec_get_name(par->codec_id), par->ch_layout.nb_channels, par->sample_rate, av_rescale_q(pst->duration, pst->time_base, AV_TIME_BASE_Q));
		}
		else if (AVMEDIA_TYPE_SUBTITLE == par->codec_type && i == 7)
		{
		}
	}
    duration_ = fmtCtx_->duration;
    if (duration_ <= 0) {
        duration_ = readPacketDuration(fmtCtx_);
    }

    MMLOGI("open end, duration:%" PRId64 "", duration_);
	
    return 0;
}

void FFDemuxer::setDemuxerFlag(int flag, int dec_flag, int write_flag)
{
	demux_flag_ = flag;
	dec_flag_ = dec_flag;
    write_packet_ = write_flag;
}

int FFDemuxer::threadProc()
{
    int ret = 0;
    while(isAlive())
    {
        if(seekFlag_)
        {
			ret = seekInternal(seekOffset_);
            MMLOGI("ret:%d = seekInternal(seekOffset:%lld)\n", ret, seekOffset_);
            seekFlag_ = false;
        }

        if(endOfStream_ || bPause_)
        {
            threadSleep(10);
            continue;
        }
        
        AVPacket* pkt = av_packet_alloc();
        ret = av_read_frame(fmtCtx_, pkt);
        if(AVERROR_EOF == ret)
        {
           endOfStream_ = true;
           break;
        }
        else if(ret < 0)
        {
            MMLOGI("Invalid error, ret:%d\n", ret);
            break;
        }

		pkt->time_base = fmtCtx_->streams[pkt->stream_index]->time_base;
        if (end_duration_ < av_rescale_q(pkt->pts, pkt->time_base, AV_TIME_BASE_Q))
        {
            break;
        }
        if (write_packet_ > 0) {
            writePacket(pkt);
        }
		if (demux_flag_ > 0)
		{
			dumpPacket(pkt);
		}
        if (dec_flag_ > 0) {
            writeFrame(pkt);
        }
        
        av_packet_free(&pkt);
    }

    for (auto it : file_map_)
    {
        fclose(it.second);
    }
    file_map_.clear();

    for (auto it : dec_map_)
    {
        fclose(it.second);
    }
    dec_map_.clear();
    MMLOGI("after while");
    return ret;
}

int FFDemuxer::seek(int64_t offset)
{
    if(offset >= duration_)
    {
        MMLOGE("Invalid seek offset:%" PRId64 ", should be < duration:%" PRId64 "", offset, duration_);
        return -1;
    }
    seekOffset_ = offset;
    seekFlag_ = true;
    MMLOGI("ffdemux seek(offset:%" PRId64 ")\n", seekOffset_);
	
    if(!isAlive())
    {
        run();
        MMLOGI("run");
    }
    return 0;
}

int FFDemuxer::pause(bool bPause)
{
    bPause_ = bPause;
    MMLOGI("demuxer pause:%d", (int)bPause);
    return 0;
}

void FFDemuxer::close()
{
    MMLOGI("demuxer close begin");
    stop();
    MMLOGI("demuxer stop end");
    if (fmtCtx_)
    {
        avformat_close_input(&fmtCtx_);
        fmtCtx_ = NULL;
    }
    for (auto it : bsf_filt_map_) {
        it.second->deinit();
        it.second.reset();
    }
    bsf_filt_map_.clear();
    MMLOGI("demuxer close end");
}

int64_t FFDemuxer::duration()
{
    return duration_;
}

AVCodecContext* FFDemuxer::openDecCodecContext(AVStream* pst)
{
	auto par = pst->codecpar;
	auto codec = avcodec_find_decoder(par->codec_id);
	if (!codec)
	{
		MMLOGE("open decoder codec:%s context\n", avcodec_get_name(par->codec_id));
		return NULL;
	}
	auto decCtx = avcodec_alloc_context3(codec);
	int ret = avcodec_parameters_to_context(decCtx, par);
	if (ret != 0)
	{
		MMLOGE("Invalid parameter, ret:%d\n", ret);
		return NULL;
	}

	ret = avcodec_open2(decCtx, codec, NULL);
	if (ret != 0)
	{
		MMLOGE("open dec context faild, ret:%d\n", ret);
		avcodec_free_context(&decCtx);
		return NULL;
	}
	return decCtx;
}
 
int FFDemuxer::deliverPacket(AVPacket* pkt)
{
	auto par = fmtCtx_->streams[pkt->stream_index]->codecpar;
	avPacketTimeScale(pkt, NULL, &fmtCtx_->streams[pkt->stream_index]->time_base);
	dumpPacketInfo("demuxer pkt", pkt);

	if (par->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		if (file_map_.find(pkt->stream_index) == file_map_.end())
		{
			static int num = 0;
			char buf[265];
			sprintf(buf, "dec%d_%d_%d_%d.pcm", num++, par->ch_layout.nb_channels, par->sample_rate, par->format);
			file_map_[pkt->stream_index] = fopen(buf, "wb");
		}
	}
	else if (AVMEDIA_TYPE_VIDEO == par->codec_type)
	{
		if (file_map_.find(pkt->stream_index) == file_map_.end())
		{
			static int num = 0;
			char buf[265];
			sprintf(buf, "dec_%d_%d_%d.yuv", par->width, par->height, par->format);
			file_map_[pkt->stream_index] = fopen(buf, "wb");
		}
	}

	if(file_map_.find(pkt->stream_index) != file_map_.end())
	{
		writeFrameFileFromPacket(codecCtxList_[pkt->stream_index], pkt, file_map_[pkt->stream_index]);
	}

	return 0;
	
	if(AVMEDIA_TYPE_VIDEO == par->codec_type)
	{
		if (codecCtxList_.find(pkt->stream_index) == codecCtxList_.end())
		{
			codecCtxList_[pkt->stream_index] = openDecCodecContext(fmtCtx_->streams[pkt->stream_index]);
			char path[256];
			sprintf(path, "dec_%dx%d.yuv", codecCtxList_[pkt->stream_index]->width, codecCtxList_[pkt->stream_index]->height);
			yuv_path_ = path;
		}

		writeFrameFromPacket(codecCtxList_[pkt->stream_index], pkt, yuv_path_.c_str());
	}
	else if (AVMEDIA_TYPE_AUDIO == par->codec_type)
	{
		if (codecCtxList_.find(pkt->stream_index) == codecCtxList_.end())
		{
			codecCtxList_[pkt->stream_index] = openDecCodecContext(fmtCtx_->streams[pkt->stream_index]);
			char path[256];
			sprintf(path, "dec_%d_%d.pcm", codecCtxList_[pkt->stream_index]->ch_layout.nb_channels, codecCtxList_[pkt->stream_index]->sample_rate);
			pcm_path_ = path;
		}

		writeFrameFromPacket(codecCtxList_[pkt->stream_index], pkt, pcm_path_.c_str());
		
	}
	else
	{
		dumpPacketInfo("subtitle", pkt);
	}
	
	return 0;
}

void FFDemuxer::dumpPacket(AVPacket* pkt)
{
	auto par = fmtCtx_->streams[pkt->stream_index]->codecpar;
	if (AVMEDIA_TYPE_VIDEO == par->codec_type && (demux_flag_ & PARSER_FLAG_VIDEO))
	{
		printPacket(pkt);
	}
	else if (AVMEDIA_TYPE_AUDIO == par->codec_type && (demux_flag_ & PARSER_FLAG_AUDIO))
	{
		printPacket(pkt);
	}
	else if (AVMEDIA_TYPE_SUBTITLE == par->codec_type && (demux_flag_ & PARSER_FLAG_SUBTITLE))
	{
		printPacket(pkt);
	}
}

void FFDemuxer::writePacket(AVPacket* pkt)
{
    auto par = fmtCtx_->streams[pkt->stream_index]->codecpar;
    if (AVMEDIA_TYPE_VIDEO == par->codec_type && (write_packet_ & PARSER_FLAG_VIDEO))
    {
        writeStream(pkt);
    }
    else if (AVMEDIA_TYPE_AUDIO == par->codec_type && (write_packet_ & PARSER_FLAG_AUDIO))
    {
        writeStream(pkt);
    }
    else if (AVMEDIA_TYPE_SUBTITLE == par->codec_type && (write_packet_ & PARSER_FLAG_SUBTITLE))
    {
        writeStream(pkt);
    }
}

void FFDemuxer::writeStream(AVPacket* pkt)
{
    if (!fmtCtx_) {
        return;
    }
    auto par = fmtCtx_->streams[pkt->stream_index];
    if (bsf_filt_map_.find(pkt->stream_index) != bsf_filt_map_.end()) {
        bsf_filt_map_[pkt->stream_index]->filter(pkt);
    }
    if (par->codecpar->codec_id == AV_CODEC_ID_H264) {
        if (file_map_.end() == file_map_.find(pkt->stream_index)) {
            file_map_[pkt->stream_index] = fopen("out.h264", "wb");
        }
    }
    else if (par->codecpar->codec_id == AV_CODEC_ID_H265){
        if (file_map_.end() == file_map_.find(pkt->stream_index)) {
            file_map_[pkt->stream_index] = fopen("out.h265", "wb");
        }
    }
    else if (par->codecpar->codec_id == AV_CODEC_ID_AAC) {
        if (file_map_.end() == file_map_.find(pkt->stream_index)) {
            file_map_[pkt->stream_index] = fopen("out.adts", "wb");
        }
    }

    if (file_map_[pkt->stream_index]) {
        fwrite(pkt->data, 1, pkt->size, file_map_[pkt->stream_index]);
    }
}

void FFDemuxer::printPacket(AVPacket* pkt)
{
	auto par = fmtCtx_->streams[pkt->stream_index]->codecpar;
	char buf[1024];
	memset(buf, 0, 1024);
	const char* name = "unknown";
	if (AVMEDIA_TYPE_VIDEO == par->codec_type)
	{
		name = "v";
	}
	else if (AVMEDIA_TYPE_AUDIO == par->codec_type)
	{
		name = "a";
	}
	else if (AVMEDIA_TYPE_SUBTITLE == par->codec_type)
	{
		name = "s";
	}
	sprintf(buf, "%spkt index:%d, pts pts:%" PRId64 ", dts:%" PRId64 ", duration:%" PRId64 ", pos:%" PRId64 ", flags:%d, size:%7d",
		name, pkt->stream_index, pkt->pts, pkt->dts, pkt->duration, pkt->pos, pkt->flags, pkt->size);

	if (pkt->time_base.num != 0 && pkt->time_base.den != 0)
	{
		avPacketTimeScale(pkt, NULL, &fmtCtx_->streams[pkt->stream_index]->time_base);
		sprintf(buf + strlen(buf), ",tb:{%d,%d}, std pts:%" PRId64 ", dts:%" PRId64 "", pkt->time_base.num, pkt->time_base.den, 
			pkt->pts, pkt->dts);
		if (lastPtsMap_.find(pkt->stream_index) != lastPtsMap_.end() && lastPtsMap_[pkt->stream_index] != AV_NOPTS_VALUE)
		{
			sprintf(buf + strlen(buf), ", duration:%" PRId64 "", pkt->dts - lastPtsMap_[pkt->stream_index]);
		}
	}

    if (par->codec_type == AVMEDIA_TYPE_VIDEO && (pkt->flags & AV_PKT_FLAG_KEY)) {
        sprintf(buf + strlen(buf), " gop duration:%" PRId64 "", pkt->pts - last_key_frame_pts);
        last_key_frame_pts = pkt->pts;
    }
	sprintf(buf + strlen(buf), "\n");
	lastPtsMap_[pkt->stream_index] = pkt->dts;
	MMLOGI(buf);
}

int64_t FFDemuxer::parserDuration(AVMediaType mt)
{
	int ret = av_seek_frame(fmtCtx_, -1, 0, AVSEEK_FLAG_BACKWARD);
	AVPacket* pkt = av_packet_alloc();
	int begin_pts = 0;
	int64_t last_pts = AV_NOPTS_VALUE;
	int index = -1;
	do {
		ret = av_read_frame(fmtCtx_, pkt);
		if (ret == 0)
		{
			if (mt == fmtCtx_->streams[pkt->stream_index]->codecpar->codec_type &&  pkt->pts != AV_NOPTS_VALUE)
			{
				index = pkt->stream_index;
				begin_pts = timeScale(pkt->pts, NULL, &fmtCtx_->streams[pkt->stream_index]->time_base);
				av_packet_unref(pkt);
				break;
			}
		}
		av_packet_unref(pkt);
	} while (ret == 0);
	
	int64_t min_pos = avio_size(fmtCtx_->pb) - 2048* 1024;
	ret = avformat_seek_file(fmtCtx_, index, min_pos, min_pos, avio_size(fmtCtx_->pb), AVSEEK_FLAG_BYTE);
	do {
		ret = av_read_frame(fmtCtx_, pkt);
		if (ret == 0)
		{
			if (index == pkt->stream_index && pkt->pts != AV_NOPTS_VALUE)
			{
				last_pts = timeScale(pkt->pts, NULL, &fmtCtx_->streams[pkt->stream_index]->time_base);
			}
			av_packet_unref(pkt);
		}
		else
		{
			break;
		}
	} while (ret == 0);
	int64_t duration = last_pts - begin_pts;
	return duration;
}

int FFDemuxer::OnFrameReady(int index, AVFrame* pframe)
{
	/*auto par = fmtCtx_->streams[index]->codecpar;
	if (par->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		if (audio_resample_list_.find(index) == audio_resample_list_.end())
		{
			audio_resample_list_[index] = std::make_shared<FFmpegAudioFifo>();
			audio_resample_list_[index]->init(pframe->ch_layout, AV_SAMPLE_FMT_FLTP, 44100);
			char name[256];
			sprintf(name, "swr_%d_%d_%d.pcm", pframe->ch_layout.nb_channels, pframe->format, 44100);
			file_map_[index] = fopen(name, "wb");
			sprintf(name, "dec_%d_%d_%d.pcm", pframe->ch_layout.nb_channels, pframe->format, pframe->sample_rate);
			dec_map_[index] = fopen(name, "wb");
		}
		fwrite(pframe->data[0], 1, pframe->linesize[0], dec_map_[index]);
		audio_resample_list_[index]->writeFrame(pframe);
		while (audio_resample_list_[index]->samples() > 1024)
		{
			AVFrame* outframe = audio_resample_list_[index]->readFrame(1024);
			fwrite(outframe->data[0], 1, outframe->linesize[0], file_map_[index]);
			av_frame_free(&outframe);
		}		
	}*/

	return 0;
}

void FFDemuxer::flush(AVCodecContext* decCtx_) {
	if (!decCtx_) {
		return;
	}

	flushCodecContext(decCtx_);
	for (auto it = file_map_.begin(); it != file_map_.end(); it++)
	{
		if (it->second)
		{
			fclose(it->second);
			it->second = NULL;
		}
	}
	file_map_.clear();
	
}

int FFDemuxer::seekInternal(int64_t pos)
{
	int ret = av_seek_frame(fmtCtx_, -1, pos, AVSEEK_FLAG_BACKWARD);
	MMLOGI("ret:%d = av_seek_frame(fmtCtx_, AVMEDIA_TYPE_VIDEO, pos, AVSEEK_FLAG_BACKWARD), reason:%s", ret, ffmpegErrStr(ret));
	int64_t video_pos = avio_tell(fmtCtx_->pb);
	for (auto it = codecCtxList_.begin(); it != codecCtxList_.end(); it++)
	{
		flush(it->second);
	}
	
	seekFlag_ = true;
	return ret;
}

void writeAudioFrame(AVFrame* frame)
{
	if (frame->sample_rate > 0 && frame->ch_layout.nb_channels > 0)
	{
		static FILE* pfile = NULL;
		if (NULL == pfile)
		{
			char buf[256];
			sprintf(buf, "dec_%d_%d_%d.pcm", frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
			pfile = fopen(buf, "wb");
		}

		if (pfile)
		{
			fwrite(frame->data[0], 1, frame->linesize[0], pfile);
		}
	}
}

int FFDemuxer::start(int64_t end_duration)
{
    end_duration_ = end_duration;

    return run();
}

void FFDemuxer::stop()
{
    UGThread::stop();
}

void FFDemuxer::writeFrame(AVPacket* pkt)
{
    auto par = fmtCtx_->streams[pkt->stream_index]->codecpar;
    if ((AVMEDIA_TYPE_VIDEO == par->codec_type && (dec_flag_ & PARSER_FLAG_VIDEO)) || (AVMEDIA_TYPE_AUDIO == par->codec_type && (dec_flag_ & PARSER_FLAG_AUDIO)))
    {
        if (codecCtxList_.find(pkt->stream_index) == codecCtxList_.end()) {
            codecCtxList_[pkt->stream_index] = openDecCodecContext(fmtCtx_->streams[pkt->stream_index]);
        }
        decodePacket(codecCtxList_[pkt->stream_index], pkt);
    }
}

void FFDemuxer::decodePacket(AVCodecContext* decCtx, AVPacket* pkt)
{
    int index = pkt->stream_index;
    int ret = avcodec_send_packet(decCtx, pkt);
    if (ret < 0) {
        MMLOGE("Error sending a packet for decoding, reason:%s\n", ffmpegErrStr(ret));
        return ;
    }
    AVFrame* frame = NULL;
    if (NULL == frame)
    {
        frame = av_frame_alloc();
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(decCtx, frame);
        //MMLOGI("ret:%d = avcodec_receive_frame(decCtx, frame)\n", ret);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            MMLOGE("Error during decoding, reason:%s\n", ffmpegErrStr(ret));
            break;
        }
        else if (ret == 0) {
            //MMLOGI(dumpFrame("dec", frame, &decCtx->time_base).c_str());
            writeFrame(index, frame);
            // return ret;
        }
    }
    av_frame_free(&frame);
}

void FFDemuxer::writeFrame(int index, AVFrame* pframe)
{
    if (dec_map_.find(index) == dec_map_.end())
    {
        char buf[256] = { 0 };
        if (pframe->width > 0 && pframe->height > 0) {
            snprintf(buf, 256, "video_%dx%d.yuv420p", pframe->width, pframe->height);
        }
        else {
            snprintf(buf, 256, "audio_sr%d_chan%d_fmt%d.pcm", pframe->sample_rate, pframe->ch_layout.nb_channels, pframe->format);
        }
        dec_map_[index] = fopen(buf, "wb");
    }
    int y_pitch = pframe->linesize[0];
    int uv_pitch = pframe->linesize[1];
    uint8_t* u_data = pframe->data[1];// +y_pitch * pframe->height;
    uint8_t* v_data = pframe->data[2];// + y_pitch * pframe->height * 5/4;
    if (pframe->width > 0 && pframe->height > 0) {
        for (int i = 0; i < pframe->height; i++) {
            fwrite(pframe->data[0]+i * y_pitch, 1, y_pitch, dec_map_[index]);
        }
        
        for (int i = 0; i < pframe->height/2; i++) {
            fwrite(u_data + i * uv_pitch, 1, uv_pitch, dec_map_[index]);
        }

        for (int i = 0; i < pframe->height / 2; i++) {
            fwrite(v_data + i * uv_pitch, 1, uv_pitch, dec_map_[index]);
        }
    }
    else if (pframe->ch_layout.nb_channels > 0 && pframe->sample_rate > 0) {
        fwrite(pframe->data[0], 1, pframe->linesize[0], dec_map_[index]);
    }
}