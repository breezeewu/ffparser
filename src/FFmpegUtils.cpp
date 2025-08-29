#include "FFmpegUtils.h"
const char* ffmpegErrStr(int averr) {
  thread_local static char s_estr[AV_ERROR_MAX_STRING_SIZE];
  av_make_error_string(s_estr, AV_ERROR_MAX_STRING_SIZE, averr);
  return s_estr;
}
const AVRational std_time_base = AV_TIME_BASE_Q;
int64_t timeScale(int64_t pts, const AVRational* dst_ral, const AVRational* src_ral)
{
  const AVRational *pdst = dst_ral, *psrc = src_ral;
    if(NULL == pdst)
    {
        pdst = &std_time_base;
    }

    if(NULL == psrc)
    {
        psrc = &std_time_base;
    }

    if(AV_NOPTS_VALUE != pts)
    {
        pts = av_rescale_q(pts, *psrc, *pdst);
    }
    
    return pts;
}

void dumpPacketInfo(std::string name, const AVPacket* pkt)
{
    if(!pkt)
    {
        return;
    }
    char buf[256];
    sprintf(buf, "%s pkt index:%d, pts:%" PRId64 ", dts:%" PRId64 ", duration:%" PRId64 ", pos:%" PRId64 " flag:%d, size:%d", name.c_str(),  pkt->stream_index, pkt->pts, pkt->dts, pkt->duration, pkt->pos, pkt->flags, pkt->size);
    sprintf(buf + strlen(buf), " std pts:%" PRId64 ", dts:%" PRId64 ", duration:%" PRId64 ", time_base{%d, %d}\n", timeScale(pkt->pts, NULL, &pkt->time_base), timeScale(pkt->dts, NULL, &pkt->time_base), timeScale(pkt->duration, NULL, &pkt->time_base), pkt->time_base.num, pkt->time_base.den);
    /*if (pkt->size >= 4)
    {
        sprintf(buf + strlen(buf), "%02x%02x%02x%02x", pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3]);
    }*/
    MMLOGI("%s", buf);
}

void dumpSubtitleInfo(std::string name, AVSubtitle* sub)
{
    char buf[256];
    sprintf(buf, "%s pkt pts:%" PRId64 ", start_display_time:%u, end_display_time:%u, format:%d\n", name.c_str(),  sub->pts, sub->start_display_time, sub->end_display_time, (int)sub->format);
    //sprintf(buf + strlen(buf), " std pts:%" PRId64 ", dts:%" PRId64 ", duration:%" PRId64 "", timeScale(pkt->pts, NULL, &pkt->time_base), timeScale(pkt->dts, NULL, &pkt->time_base), timeScale(pkt->duration, NULL, &pkt->time_base));
    MMLOGI("%s", buf);
}

void avPacketTimeScale(AVPacket* pkt, const AVRational* dst_ral, const AVRational* src_ral)
{
  const AVRational *pdst = dst_ral, *psrc = src_ral;
    if(NULL == pdst)
    {
        pdst = &std_time_base;
    }
    else if(pdst->den == 0)
    {
        return ;
    }

    if(NULL == psrc)
    {
        psrc = &std_time_base;
    }

    else if(psrc->den == 0)
    {
        return ;
    }
    if(AV_NOPTS_VALUE != pkt->pts)
    {
        pkt->pts = av_rescale_q(pkt->pts, *psrc, *pdst);
    }
    else if(AV_NOPTS_VALUE != pkt->dts)
    {
        pkt->pts = av_rescale_q(pkt->dts, *psrc, *pdst);
    }

    if(AV_NOPTS_VALUE != pkt->dts)
    {
        pkt->dts = av_rescale_q(pkt->dts, *psrc, *pdst);
    }
    else if(AV_NOPTS_VALUE != pkt->pts)
    {
        pkt->dts = pkt->pts;
        //MMLOGI("pkt->dts:%" PRId64 " = pkt->pts", pkt->dts);
    }
    
    if(AV_NOPTS_VALUE != pkt->duration)
    {
        pkt->duration = av_rescale_q(pkt->duration, *psrc, *pdst);
    }

    pkt->time_base = *pdst;
}

bool isTextSubtitle(int codec_id)
{
    switch(codec_id)
    {
      case AV_CODEC_ID_TEXT:
      case AV_CODEC_ID_SSA:
      case AV_CODEC_ID_MOV_TEXT:
      case AV_CODEC_ID_DVB_TELETEXT:
      case AV_CODEC_ID_SRT:
      case AV_CODEC_ID_REALTEXT:
      case AV_CODEC_ID_STL:
      case AV_CODEC_ID_SUBRIP:
      case AV_CODEC_ID_WEBVTT:
      case AV_CODEC_ID_ASS:
      case AV_CODEC_ID_PJS:
      case AV_CODEC_ID_JACOSUB:
      case  AV_CODEC_ID_VPLAYER:
      case AV_CODEC_ID_HDMV_TEXT_SUBTITLE:
      case AV_CODEC_ID_EIA_608:
        return true;
      default:
        return false;
    }
}
void av_log_cb(void *ptr, int level, const char *fmt, va_list vl)
{
    if (level > av_log_get_level())
    {
        return;
    }

    va_list vl2;
    char line[4096] = { 0 };
    static int print_prefix = 1;

    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);
    //printf("%s\n", line);
    if(level <= AV_LOG_ERROR)
    {
      MMLOGE("%s\n", line);
    }
    else if(level <= AV_LOG_WARNING)
    {
        MMLOGW("%s\n", line);
    }
    else// if(level <= AV_LOG_INFO)
    {
        MMLOGI("%s\n", line);
    }
}

std::string getCodecNameByCodecID(AVCodecID codec_id)
{
    std::string codecName;
    const AVCodec* codec = avcodec_find_decoder(codec_id);
    if(codec)
    {
        codecName = codec->name;
    }

    return codecName;
}

void avFrameTimeScale(AVFrame* frame, const AVRational* dst_ral, const AVRational* src_ral)
{
	const AVRational* pdst = dst_ral, * psrc = src_ral;
	if (NULL == frame)
	{
		return;
	}

	if (NULL == pdst)
	{
		pdst = &std_time_base;
	}
	else if (pdst->den == 0)
	{
		return;
	}

	if (NULL == psrc)
	{
		psrc = &std_time_base;
	}
	else if (psrc->den == 0)
	{
		return;
	}

	if (AV_NOPTS_VALUE != frame->pts)
	{
		frame->pts = av_rescale_q(frame->pts, *psrc, *pdst);
	}

	if (AV_NOPTS_VALUE != frame->duration)
	{
		frame->duration = av_rescale_q(frame->duration, *psrc, *pdst);
	}

	if (AV_NOPTS_VALUE != frame->pkt_dts)
	{
		frame->pkt_dts = av_rescale_q(frame->pkt_dts, *psrc, *pdst);
	}
}

std::string dumpFrame(const std::string str, AVFrame* frame, const AVRational* ptime_base)
{
	if (!frame)return std::string();
	char buf[1024];
	if (frame->width > 0 && frame->height > 0)
	{
		sprintf(buf, "%s vframe, pts:%" PRId64 ", frame->duration:%" PRId64 ", pkt_dts:%" PRId64 ", width:%d, height:%d, format:%d, linesize[0]:%d", str.c_str(), frame->pts, frame->duration, frame->pkt_dts, frame->width, frame->height, frame->format, frame->linesize[0]);
	}
	else
	{
		sprintf(buf, "%s aframe, pts:%" PRId64 ", frame->duration:%" PRId64 ", pkt_dts:%" PRId64 ", channels:%d, sample_rate:%d, format:%d, nb_samples:%d, linesize[0]:%d", str.c_str(), frame->pts, frame->duration, frame->pkt_dts, frame->ch_layout.nb_channels, frame->sample_rate, frame->format, frame->nb_samples, frame->linesize[0]);
		for (int i = 0; frame->linesize[i] > 0; i++)
		{
			sprintf(buf + strlen(buf), "\ndata[i:%d], data:%p, linesize:%d\n", i, frame->data[i], frame->linesize[i]);
		}
	}

	if (ptime_base)
	{
		auto pts = timeScale(frame->pts, NULL, ptime_base);
		auto duration = timeScale(frame->duration, NULL, ptime_base);
		sprintf(buf + strlen(buf), " std pts:%" PRId64 ", frame->duration:%" PRId64 "", pts, frame->duration);
	}
	return std::string(buf);
}

int writeFrameFromPacket(AVCodecContext* decCtx, AVPacket* pkt, const char* path)
{
	int ret = avcodec_send_packet(decCtx, pkt);
	if (ret < 0) {
		MMLOGE("Error sending a packet for decoding, reason:%s\n", ffmpegErrStr(ret));
		return ret;
	}
	AVFrame* frame = NULL;
	if (NULL == frame)
	{
		frame = av_frame_alloc();
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(decCtx, frame);
		MMLOGI("ret:%d = avcodec_receive_frame(decCtx, frame)\n", ret);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			MMLOGE("Error during decoding, reason:%s\n", ffmpegErrStr(ret));
			break;
		}
		else if (ret == 0) {
			MMLOGI(dumpFrame("dec", frame, &decCtx->time_base).c_str());
			if (path)
			{
				if (frame->ch_layout.nb_channels > 0)
				{
					FILE* pfile = fopen(path, "ab");
					if (pfile)
					{
						int ret = fwrite(frame->data[0], 1, frame->linesize[0], pfile);
						MMLOGI("fwrite pts:%ld, size:%d, ret:%d\n", frame->pts, frame->linesize[0], ret);
						fclose(pfile);
					}
				}
			}
			// return ret;
		}
	}
	return 0;
}

int DecoderPacket(AVCodecContext* decCtx, AVPacket* pkt, IOnFrameReady* frame_ready)
{
	int index = pkt->stream_index;
	int ret = avcodec_send_packet(decCtx, pkt);
	if (ret < 0) {
		MMLOGE("Error sending a packet for decoding, reason:%s\n", ffmpegErrStr(ret));
		return ret;
	}
	AVFrame* frame = NULL;
	if (NULL == frame)
	{
		frame = av_frame_alloc();
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(decCtx, frame);
		MMLOGI("ret:%d = avcodec_receive_frame(decCtx, frame)\n", ret);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			MMLOGE("Error during decoding, reason:%s\n", ffmpegErrStr(ret));
			break;
		}
		else if (ret == 0) {
			MMLOGI(dumpFrame("dec", frame, &decCtx->time_base).c_str());
			if (frame_ready)
			{
				frame_ready->OnFrameReady(index, frame);
			}
			// return ret;
		}
	}
	av_frame_free(&frame);
	return 0;
}

int writeFrameFileFromPacket(AVCodecContext* decCtx, AVPacket* pkt, FILE* pfile)
{
	int ret = avcodec_send_packet(decCtx, pkt);
	if (ret < 0) {
		MMLOGE("Error sending a packet for decoding, reason:%s\n", ffmpegErrStr(ret));
		return ret;
	}
	AVFrame* frame = NULL;
	if (NULL == frame)
	{
		frame = av_frame_alloc();
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(decCtx, frame);
		MMLOGI("ret:%d = avcodec_receive_frame(decCtx, frame)\n", ret);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			MMLOGE("Error during decoding, reason:%s\n", ffmpegErrStr(ret));
			break;
		}
		else if (ret == 0) {
            /*static Resample* resample = NULL;
            static AVFrame* out_frame = NULL;
            if(resample == NULL)
            {
                resample = new Resample();
                ret = av_channel_layout_check(&decCtx->ch_layout);
                MMLOGI("frame->sample_rate:%d, frame->format:%d, frame->ch_layout:%d, ret:%d\n", frame->sample_rate, frame->format, decCtx->ch_layout.nb_channels, ret);
                resample->init(frame->sample_rate, (AVSampleFormat)frame->format, decCtx->ch_layout, 48000, AV_SAMPLE_FMT_FLTP, decCtx->ch_layout);
            }
            if(out_frame == NULL)
            {
                out_frame = av_frame_alloc();
            }
            ret = resample->resample(frame, out_frame);
            MMLOGI("ret:%d = resample->resample(frame, out_frame)\n", ret);*/
			MMLOGI(dumpFrame("dec", frame, &decCtx->time_base).c_str());
#if 1
			int ret = fwrite(frame->data[0], 1, frame->linesize[0], pfile);
			MMLOGI("ret:%d = fwrite(frame->data[0], 1, frame->linesize[0]:%d, pfile:%p)\n", ret, frame->linesize[0], pfile);
			//writeFrameToFile(frame, pfile);
#else
			if (frame->ch_layout.nb_channels > 0 && pfile)
			{
				if (pfile)
				{
					fwrite(frame->data[0], 1, frame->linesize[0], pfile);
					MMLOGI("pts:%ld, size:%d\n", frame->pts, frame->linesize[0]);
				}
			}
#endif
			// return ret;
		}
	}
	return 0;
}

int flushCodecContext(AVCodecContext* codecCtx) {
	if (!codecCtx) {
		return -1;
	}

	if (av_codec_is_decoder(codecCtx->codec)) {
		avcodec_send_packet(codecCtx, nullptr);
		AVFrame* decFrame = av_frame_alloc();
		while (avcodec_receive_frame(codecCtx, decFrame) >= 0) {
			av_frame_unref(decFrame);
		}
		av_frame_free(&decFrame);
		avcodec_flush_buffers(codecCtx);
		return 0;
	}
	else if (codecCtx->codec->capabilities & AV_CODEC_CAP_ENCODER_FLUSH) {
		avcodec_send_frame(codecCtx, NULL);
		AVPacket pkt;
		while (avcodec_receive_packet(codecCtx, &pkt) >= 0) {
			av_packet_unref(&pkt);
		}
		avcodec_flush_buffers(codecCtx);
		return 0;
	}

	return -1;
}

int writeFrameToFile(AVFrame* frame, FILE* pfile) {
    if (!frame || !pfile) {
        MMLOGE("Invalid parameters: frame or file is null\n");
        return -1;
    }

    // Check if this is a hardware frame and copy to system memory if needed
    AVFrame* sw_frame = frame;
    AVFrame* temp_frame = nullptr;
    
    if (frame->hw_frames_ctx) {
        // Hardware frame detected, copy to system memory
        temp_frame = av_frame_alloc();
        if (!temp_frame) {
            MMLOGE("Failed to allocate temporary frame\n");
            return -1;
        }
        
        int ret = av_hwframe_transfer_data(temp_frame, frame, 0);
        if (ret < 0) {
            MMLOGE("Failed to transfer hardware frame to system memory: %s\n", ffmpegErrStr(ret));
            av_frame_free(&temp_frame);
            return ret;
        }
        
        // Copy frame properties
        temp_frame->pts = frame->pts;
        temp_frame->pkt_dts = frame->pkt_dts;
        temp_frame->duration = frame->duration;
        
        sw_frame = temp_frame;
    }

    int ret = 0;
    
    // Audio frame (PCM data)
    if (sw_frame->ch_layout.nb_channels > 0 && sw_frame->sample_rate > 0) {
        MMLOGI("Writing audio frame: channels=%d, sample_rate=%d, format=%d, nb_samples=%d\n", 
               sw_frame->ch_layout.nb_channels, sw_frame->sample_rate, sw_frame->format, sw_frame->nb_samples);
        
        int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)sw_frame->format);
        if (bytes_per_sample <= 0) {
            MMLOGE("Invalid audio format: %d\n", sw_frame->format);
            ret = -1;
            goto cleanup;
        }
        
        // Check if format is planar or packed
        if (av_sample_fmt_is_planar((AVSampleFormat)sw_frame->format)) {
            // Planar format: write each channel's data separately
            for (int ch = 0; ch < sw_frame->ch_layout.nb_channels; ch++) {
                if (sw_frame->data[ch] && sw_frame->linesize[0] > 0) {
                    size_t written = fwrite(sw_frame->data[ch], 1, sw_frame->linesize[0], pfile);
                    if (written != (size_t)sw_frame->linesize[0]) {
                        MMLOGE("Failed to write audio channel %d data\n", ch);
                        ret = -1;
                        goto cleanup;
                    }
                }
            }
        } else {
            // Packed format: all channels interleaved in data[0]
            if (sw_frame->data[0] && sw_frame->linesize[0] > 0) {
                size_t written = fwrite(sw_frame->data[0], 1, sw_frame->linesize[0], pfile);
                if (written != (size_t)sw_frame->linesize[0]) {
                    MMLOGE("Failed to write packed audio data\n");
                    ret = -1;
                    goto cleanup;
                }
            }
        }
        
        MMLOGI("Audio frame written successfully, bytes written per channel/total: %d\n", sw_frame->linesize[0]);
    }
    // Video frame (YUV data)
    else if (sw_frame->width > 0 && sw_frame->height > 0) {
        MMLOGI("Writing video frame: width=%d, height=%d, format=%d\n", 
               sw_frame->width, sw_frame->height, sw_frame->format);
        
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get((AVPixelFormat)sw_frame->format);
        if (!desc) {
            MMLOGE("Invalid pixel format: %d\n", sw_frame->format);
            ret = -1;
            goto cleanup;
        }
        
        // Write each plane (Y, U, V for YUV formats)
        for (int plane = 0; plane < desc->nb_components; plane++) {
            if (sw_frame->data[plane] && sw_frame->linesize[plane] > 0) {
                int plane_height = sw_frame->height;
                
                // For chroma planes in YUV420, height is half
                if (plane > 0 && (desc->log2_chroma_h > 0)) {
                    plane_height = sw_frame->height >> desc->log2_chroma_h;
                }
                
                // Write line by line to handle stride properly
                for (int y = 0; y < plane_height; y++) {
                    int line_width = sw_frame->linesize[plane];
                    
                    // For chroma planes in YUV420, width might be different
                    if (plane > 0 && (desc->log2_chroma_w > 0)) {
                        line_width = sw_frame->width >> desc->log2_chroma_w;
                        if (desc->comp[plane].step > 1) {
                            line_width *= desc->comp[plane].step;
                        }
                    } else if (plane == 0) {
                        line_width = sw_frame->width;
                        if (desc->comp[plane].step > 1) {
                            line_width *= desc->comp[plane].step;
                        }
                    }
                    
                    uint8_t* line_ptr = sw_frame->data[plane] + y * sw_frame->linesize[plane];
                    size_t written = fwrite(line_ptr, 1, line_width, pfile);
                    if (written != (size_t)line_width) {
                        MMLOGE("Failed to write video plane %d, line %d\n", plane, y);
                        ret = -1;
                        goto cleanup;
                    }
                }
                
                MMLOGI("Video plane %d written: %dx%d, linesize=%d\n", 
                       plane, (plane > 0 && desc->log2_chroma_w > 0) ? sw_frame->width >> desc->log2_chroma_w : sw_frame->width,
                       plane_height, sw_frame->linesize[plane]);
            }
        }
        
        MMLOGI("Video frame written successfully\n");
    }
    else {
        MMLOGE("Invalid frame: neither audio nor video data detected\n");
        ret = -1;
    }

cleanup:
    if (temp_frame) {
        av_frame_free(&temp_frame);
    }
    
    return ret;
}

void dumpAVCodecParameters(AVCodecParameters* par)
{
    if (!par) {
        MMLOGI("AVCodecParameters is NULL\n");
        return;
    }
    
    MMLOGI("=== AVCodecParameters Info ===\n");
    
    // Basic codec information
    MMLOGI("Codec Type: %s (%d)\n", av_get_media_type_string(par->codec_type), par->codec_type);
    MMLOGI("Codec ID: %s (%d)\n", avcodec_get_name(par->codec_id), par->codec_id);
    MMLOGI("Codec Tag: 0x%08x\n", par->codec_tag);
    MMLOGI("Format: %d\n", par->format);
    MMLOGI("Bit Rate: %ld bps\n", par->bit_rate);
    MMLOGI("Bits per Coded Sample: %d\n", par->bits_per_coded_sample);
    MMLOGI("Bits per Raw Sample: %d\n", par->bits_per_raw_sample);
    MMLOGI("Profile: %d\n", par->profile);
    MMLOGI("Level: %d\n", par->level);
    
    // Extra data
    if (par->extradata && par->extradata_size > 0) {
        MMLOGI("Extra Data Size: %d bytes\n", par->extradata_size);
        MMLOGI("Extra Data (first 32 bytes): ");
        int print_size = par->extradata_size > 32 ? 32 : par->extradata_size;
        for (int i = 0; i < print_size; i++) {
            printf("%02x ", par->extradata[i]);
        }
        if (par->extradata_size > 32) {
            printf("...");
        }
        printf("\n");
    } else {
        MMLOGI("Extra Data: None\n");
    }
    
    // Side data
    MMLOGI("Number of Coded Side Data: %d\n", par->nb_coded_side_data);
    for (int i = 0; i < par->nb_coded_side_data; i++) {
        MMLOGI("  Side Data [%d]: Type=%d, Size=%zu\n", 
               i, par->coded_side_data[i].type, par->coded_side_data[i].size);
    }
    
    // Video specific parameters
    if (par->codec_type == AVMEDIA_TYPE_VIDEO) {
        MMLOGI("--- Video Parameters ---\n");
        MMLOGI("Width: %d\n", par->width);
        MMLOGI("Height: %d\n", par->height);
        MMLOGI("Sample Aspect Ratio: %d:%d\n", par->sample_aspect_ratio.num, par->sample_aspect_ratio.den);
        MMLOGI("Field Order: %d\n", par->field_order);
        MMLOGI("Color Range: %d\n", par->color_range);
        MMLOGI("Color Primaries: %d\n", par->color_primaries);
        MMLOGI("Color Transfer: %d\n", par->color_trc);
        MMLOGI("Color Space: %d\n", par->color_space);
        MMLOGI("Chroma Location: %d\n", par->chroma_location);
        MMLOGI("Number of delayed frames: %d\n", par->video_delay);
        MMLOGI("framerate:{%d, %d}\n", par->framerate.num, par->framerate.den);
        
        // Video format name
        if (par->format != -1) {
            const char* pix_fmt_name = av_get_pix_fmt_name((AVPixelFormat)par->format);
            MMLOGI("Pixel Format: %s (%d)\n", pix_fmt_name ? pix_fmt_name : "Unknown", par->format);
        }
    }
    
    // Audio specific parameters
    if (par->codec_type == AVMEDIA_TYPE_AUDIO) {
        MMLOGI("--- Audio Parameters ---\n");
        MMLOGI("Sample Rate: %d Hz\n", par->sample_rate);
        MMLOGI("Channels: %d\n", par->ch_layout.nb_channels);
        MMLOGI("Channel Layout: ");
        char ch_layout_str[256];
        av_channel_layout_describe(&par->ch_layout, ch_layout_str, sizeof(ch_layout_str));
        MMLOGI("%s\n", ch_layout_str);
        MMLOGI("Block Align: %d\n", par->block_align);
        MMLOGI("Frame Size: %d\n", par->frame_size);
        MMLOGI("Initial Padding: %d\n", par->initial_padding);
        MMLOGI("Trailing Padding: %d\n", par->trailing_padding);
        MMLOGI("Seek Preroll: %d\n", par->seek_preroll);
        
        // Audio format name
        if (par->format != -1) {
            const char* sample_fmt_name = av_get_sample_fmt_name((AVSampleFormat)par->format);
            MMLOGI("Sample Format: %s (%d)\n", sample_fmt_name ? sample_fmt_name : "Unknown", par->format);
        }
    }
    
    // Subtitle specific parameters
    if (par->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        MMLOGI("--- Subtitle Parameters ---\n");
        MMLOGI("Width: %d\n", par->width);
        MMLOGI("Height: %d\n", par->height);
    }
    
    MMLOGI("===============================\n");
}

void dumpAVCodecContext(AVCodecContext* ctx)
{
    if (!ctx) {
        MMLOGI("AVCodecContext is NULL\n");
        return;
    }
    
    MMLOGI("=== AVCodecContext Info ===\n");
    
    // Basic codec information
    MMLOGI("Codec Type: %s (%d)\n", av_get_media_type_string(ctx->codec_type), ctx->codec_type);
    if (ctx->codec) {
        MMLOGI("Codec Name: %s\n", ctx->codec->name);
        MMLOGI("Codec Long Name: %s\n", ctx->codec->long_name ? ctx->codec->long_name : "N/A");
        MMLOGI("Codec ID: %s (%d)\n", avcodec_get_name(ctx->codec_id), ctx->codec_id);
    } else {
        MMLOGI("Codec: NULL\n");
        MMLOGI("Codec ID: %s (%d)\n", avcodec_get_name(ctx->codec_id), ctx->codec_id);
    }
    
    MMLOGI("Codec Tag: 0x%08x\n", ctx->codec_tag);
    MMLOGI("Bit Rate: %ld bps\n", ctx->bit_rate);
    MMLOGI("Bit Rate Tolerance: %d\n", ctx->bit_rate_tolerance);
    MMLOGI("Global Quality: %d\n", ctx->global_quality);
    MMLOGI("Compression Level: %d\n", ctx->compression_level);
    MMLOGI("Flags: 0x%08x\n", ctx->flags);
    MMLOGI("Flags2: 0x%08x\n", ctx->flags2);
    
    // Threading information
    MMLOGI("Thread Count: %d\n", ctx->thread_count);
    MMLOGI("Thread Type: %d\n", ctx->thread_type);
    
    // Time base information
    MMLOGI("Time Base: %d/%d\n", ctx->time_base.num, ctx->time_base.den);
    //MMLOGI("Ticks Per Frame: %d\n", ctx->ticks_per_frame);
    
    // Delay and frame information
    MMLOGI("Delay: %d\n", ctx->delay);
    MMLOGI("Frame Number: %d\n", ctx->frame_num);
    
    // Extra data
    if (ctx->extradata && ctx->extradata_size > 0) {
        MMLOGI("Extra Data Size: %d bytes\n", ctx->extradata_size);
        MMLOGI("Extra Data (first 32 bytes): ");
        int print_size = ctx->extradata_size > 32 ? 32 : ctx->extradata_size;
        for (int i = 0; i < print_size; i++) {
            printf("%02x ", ctx->extradata[i]);
        }
        if (ctx->extradata_size > 32) {
            printf("...");
        }
        printf("\n");
    } else {
        MMLOGI("Extra Data: None\n");
    }
    
    // Video specific parameters
    if (ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        MMLOGI("--- Video Parameters ---\n");
        MMLOGI("Width: %d\n", ctx->width);
        MMLOGI("Height: %d\n", ctx->height);
        MMLOGI("Coded Width: %d\n", ctx->coded_width);
        MMLOGI("Coded Height: %d\n", ctx->coded_height);
        MMLOGI("Pixel Format: %s (%d)\n", av_get_pix_fmt_name(ctx->pix_fmt), ctx->pix_fmt);
        MMLOGI("Sample Aspect Ratio: %d:%d\n", ctx->sample_aspect_ratio.num, ctx->sample_aspect_ratio.den);
        MMLOGI("Framerate: %d/%d (%.2f fps)\n", ctx->framerate.num, ctx->framerate.den, 
               ctx->framerate.den > 0 ? (double)ctx->framerate.num / ctx->framerate.den : 0.0);
        MMLOGI("GOP Size: %d\n", ctx->gop_size);
        MMLOGI("Max B Frames: %d\n", ctx->max_b_frames);
        MMLOGI("Has B Frames: %d\n", ctx->has_b_frames);
        //MMLOGI("B Frame Strategy: %d\n", ctx->b_frame_strategy);
        MMLOGI("B Quant Factor: %.2f\n", ctx->b_quant_factor);
        MMLOGI("B Quant Offset: %.2f\n", ctx->b_quant_offset);
        MMLOGI("I Quant Factor: %.2f\n", ctx->i_quant_factor);
        MMLOGI("I Quant Offset: %.2f\n", ctx->i_quant_offset);
        MMLOGI("Lumi Masking: %.2f\n", ctx->lumi_masking);
        MMLOGI("Temporal Complexity Masking: %.2f\n", ctx->temporal_cplx_masking);
        MMLOGI("Spatial Complexity Masking: %.2f\n", ctx->spatial_cplx_masking);
        MMLOGI("P Masking: %.2f\n", ctx->p_masking);
        MMLOGI("Dark Masking: %.2f\n", ctx->dark_masking);
        MMLOGI("motion estimation comparison: %d\n", ctx->me_cmp);
        MMLOGI("me subpel_quality: %d\n", ctx->me_subpel_quality);
        MMLOGI("me_range: %d\n", ctx->me_range);
        MMLOGI("me_pre_cmp: %d\n", ctx->me_pre_cmp);

        MMLOGI("Field Order: %d\n", ctx->field_order);
        MMLOGI("Color Range: %d\n", ctx->color_range);
        MMLOGI("Color Primaries: %d\n", ctx->color_primaries);
        MMLOGI("Color Transfer: %d\n", ctx->color_trc);
        MMLOGI("Colorspace: %d\n", ctx->colorspace);
        MMLOGI("Chroma Location: %d\n", ctx->chroma_sample_location);
        MMLOGI("Refs: %d\n", ctx->refs);
        MMLOGI("Skip Loop Filter: %d\n", ctx->skip_loop_filter);
        MMLOGI("Skip IDCT: %d\n", ctx->skip_idct);
        MMLOGI("Skip Frame: %d\n", ctx->skip_frame);
    }
    
    // Audio specific parameters
    if (ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        MMLOGI("--- Audio Parameters ---\n");
        MMLOGI("Sample Rate: %d Hz\n", ctx->sample_rate);
        MMLOGI("Channels: %d\n", ctx->ch_layout.nb_channels);
        MMLOGI("Channel Layout: ");
        char ch_layout_str[256];
        av_channel_layout_describe(&ctx->ch_layout, ch_layout_str, sizeof(ch_layout_str));
        MMLOGI("%s\n", ch_layout_str);
        MMLOGI("Sample Format: %s (%d)\n", av_get_sample_fmt_name(ctx->sample_fmt), ctx->sample_fmt);
        MMLOGI("Frame Size: %d\n", ctx->frame_size);
        MMLOGI("Block Align: %d\n", ctx->block_align);
        MMLOGI("Cutoff: %d\n", ctx->cutoff);
        MMLOGI("Audio Service Type: %d\n", ctx->audio_service_type);
        MMLOGI("Request Sample Format: %s (%d)\n", av_get_sample_fmt_name(ctx->request_sample_fmt), ctx->request_sample_fmt);
        MMLOGI("Initial Padding: %d\n", ctx->initial_padding);
        MMLOGI("Trailing Padding: %d\n", ctx->trailing_padding);
        MMLOGI("Seek Preroll: %d\n", ctx->seek_preroll);
    }
    
    // Subtitle specific parameters
    if (ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        MMLOGI("--- Subtitle Parameters ---\n");
        MMLOGI("Width: %d\n", ctx->width);
        MMLOGI("Height: %d\n", ctx->height);
    }
    
    // Quality and rate control parameters
    MMLOGI("--- Quality & Rate Control ---\n");
    MMLOGI("QMin: %d\n", ctx->qmin);
    MMLOGI("QMax: %d\n", ctx->qmax);
    MMLOGI("Max QDiff: %d\n", ctx->max_qdiff);
    MMLOGI("QBlur: %.2f\n", ctx->qblur);
    MMLOGI("QCompress: %.2f\n", ctx->qcompress);
    MMLOGI("RC Max Rate: %ld\n", ctx->rc_max_rate);
    MMLOGI("RC Min Rate: %ld\n", ctx->rc_min_rate);
    MMLOGI("RC Buffer Size: %d\n", ctx->rc_buffer_size);
    MMLOGI("RC Initial Buffer Occupancy: %d\n", ctx->rc_initial_buffer_occupancy);
    MMLOGI("RC Max Available VBV Use: %.2f\n", ctx->rc_max_available_vbv_use);
    MMLOGI("RC Min VBV Overflow Use: %.2f\n", ctx->rc_min_vbv_overflow_use);
    
    // Profile and level
    MMLOGI("--- Profile & Level ---\n");
    MMLOGI("Profile: %d\n", ctx->profile);
    MMLOGI("Level: %d\n", ctx->level);
    
    // Hardware acceleration
    MMLOGI("--- Hardware Acceleration ---\n");
    if (ctx->hw_device_ctx) {
        MMLOGI("Hardware Device Context: Present\n");
    } else {
        MMLOGI("Hardware Device Context: None\n");
    }
    
    if (ctx->hw_frames_ctx) {
        MMLOGI("Hardware Frames Context: Present\n");
    } else {
        MMLOGI("Hardware Frames Context: None\n");
    }
    
    //MMLOGI("Hardware Accelerated Pixel Format: %s (%d)\n", 
    //       av_get_pix_fmt_name(ctx->hw_pix_fmt), ctx->hw_pix_fmt);
    
    // Error handling and debugging
    MMLOGI("--- Error & Debug ---\n");
    MMLOGI("Error Concealment: %d\n", ctx->error_concealment);
    MMLOGI("Debug: %d\n", ctx->debug);
    MMLOGI("Error Recognition: %d\n", ctx->err_recognition);
    
    // Packet and frame statistics (if available)
    if (ctx->codec_type == AVMEDIA_TYPE_VIDEO || ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        MMLOGI("--- Statistics ---\n");
        MMLOGI("Frame Number: %d\n", ctx->frame_num);
        MMLOGI("Properties: 0x%08x\n", ctx->properties);
    }
    
    MMLOGI("===============================\n");
}

int64_t readPacketDuration(AVFormatContext* fmtCtx)
{
    if (!fmtCtx) {
        return 0;
    }
    int64_t start_timestamp = INT64_MAX;
    int64_t latest_timestamp = INT64_MIN;
    AVPacket* pkt = av_packet_alloc();
    int ret = av_seek_frame(fmtCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    if (0 != ret) {
        MMLOGE("ret:%d = av_seek_frame(fmtCtx, -1, 0, AVSEEK_FLAG_BACKWARD) failed:%s", ret, ffmpegErrStr(ret));
        return 0;
    }
    for (int i = 0; i < fmtCtx->nb_streams; i++) {
        int64_t start_time = timeScale(fmtCtx->streams[i]->start_time, NULL, &fmtCtx->streams[i]->time_base);
        if (start_time != AV_NOPTS_VALUE && start_timestamp > start_time) {
            start_timestamp = start_time;
        }
    }

    do {
        ret = av_read_frame(fmtCtx, pkt);
        if (AVERROR_EOF == ret) {
            MMLOGI("ret:%d = av_read_frame(fmtCtx, pkt)\n", ret);
            break;
        }
        else if (0 != ret) {
            MMLOGE("ret:%d = av_read_frame(fmtCtx, pkt)failed, %s\n", ret, ffmpegErrStr(ret));
            break;
        }

        avPacketTimeScale(pkt, NULL, &fmtCtx->streams[pkt->stream_index]->time_base);
        if (AV_NOPTS_VALUE != pkt->pts) {
            if (start_timestamp > pkt->pts) {
                start_timestamp = pkt->pts;
                // 如何是mpegts/mpegps 根据文件位置seek到文件末尾
                if (memcmp(fmtCtx->iformat->name, "mpegts", 6) == 0 ||
                    memcmp(fmtCtx->iformat->name, "mpegps", 6) == 0) {
                    int64_t file_size = avio_size(fmtCtx->pb);
                    //最多解析后面5M数据
                    int max_read_size = 1024 * 1024 * 5;
                    int64_t seek_size = file_size - (file_size / 10 < max_read_size ? file_size / 10 : max_read_size);
                    ret = av_seek_frame(fmtCtx, -1, seek_size, AVSEEK_FLAG_BYTE);
                    MMLOGI("ret:%d = av_seek_frame(fmtCtx, -1, seek_size, AVSEEK_FLAG_BYTE)", ret);
                    // 没取到有效值就默认为0
                    if (start_timestamp == INT64_MAX) {
                        start_timestamp = 0;
                    }
                }
            }
            if (pkt->pts > latest_timestamp) {
                latest_timestamp = pkt->pts;
            }
        }
        av_packet_unref(pkt);
    } while (0 == ret);
    av_packet_free(&pkt);
    ret = av_seek_frame(fmtCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    if (0 != ret) {
        MMLOGE("ret:%d = av_seek_frame(fmtCtx, -1, 0, AVSEEK_FLAG_BACKWARD) failed:%s", ret, ffmpegErrStr(ret));
        return 0;
    }
    MMLOGI("duration:%" PRId64 " = latest_timestamp:%" PRId64 " - start_timestamp:%" PRId64 "", latest_timestamp - start_timestamp, latest_timestamp, start_timestamp);
    return latest_timestamp - start_timestamp;
}