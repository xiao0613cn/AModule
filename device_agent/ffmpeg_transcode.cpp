#include "../stdafx.h"
#include "audio_transcode.h"


static int open_decoder(FFmpegAudioTranscode *tx, AVCodecParameters *src)
{
	AVCodec *codec = avcodec_find_decoder(src->codec_id);
	if (codec == NULL) {
		TRACE("no found decoder: %d.\n", src->codec_id);
		return -ENOSYS;
	}

	tx->_araw_ctx = avcodec_alloc_context3(codec);
	if (tx->_araw_ctx == NULL)
		return -ENOMEM;

	int result = avcodec_parameters_to_context(tx->_araw_ctx, src);
	if (result < 0) {
		TRACE("avcodec_parameters_to_context(%s) = %d.\n", codec->name, result);
		return result;
	}

	result = avcodec_open2(tx->_araw_ctx, codec, NULL);
	if (result < 0) {
		TRACE("avcodec_open2(%s) = %d.\n", codec->name, result);
		return result;
	}
	return result;
}

static int open_encoder(FFmpegAudioTranscode *tx, AVCodecParameters *dest, AVCodecParameters *src)
{
	AVCodec *codec = avcodec_find_decoder(dest->codec_id);
	if (codec == NULL) {
		TRACE("no found encoder: %d.\n", dest->codec_id);
		return -ENOSYS;
	}

	tx->_aac_ctx = avcodec_alloc_context3(codec);
	if (tx->_aac_ctx == NULL)
		return -ENOMEM;

	uint64_t channel_layout = src->channel_layout;
	if (channel_layout == 0)
		channel_layout = av_get_default_channel_layout(src->channels);

	//int bits_per_raw_sample = src->bits_per_raw_sample;
	//if (bits_per_raw_sample == 0)
	//	bits_per_raw_sample = 8*av_get_bytes_per_sample((AVSampleFormat)src->format);

	BOOL need_swr = FALSE;
	int ix = find_ix0<const int>(codec->supported_samplerates, 0, src->sample_rate);
	if (ix != -1) {
		tx->_aac_ctx->sample_rate = src->sample_rate;
	} else {
		need_swr = TRUE;
		tx->_aac_ctx->sample_rate = codec->supported_samplerates[0];
	}

	tx->_aac_ctx->channels = src->channels;
	ix = find_ix0<const uint64_t>(codec->channel_layouts, 0, channel_layout);
	if (ix != -1) {
		tx->_aac_ctx->channel_layout = channel_layout;
	} else {
		need_swr = TRUE;
		tx->_aac_ctx->channel_layout = codec->channel_layouts[0];
	}

	ix = find_ix0<const AVSampleFormat>(codec->sample_fmts, AV_SAMPLE_FMT_NONE, (AVSampleFormat)src->format);
	if (ix != -1) {
		tx->_aac_ctx->sample_fmt = (AVSampleFormat)src->format;
	} else {
		need_swr = TRUE;
		tx->_aac_ctx->sample_fmt = codec->sample_fmts[0];
	}

	if (need_swr) {
		tx->_swr_ctx = swr_alloc_set_opts(NULL,
			tx->_aac_ctx->channel_layout, tx->_aac_ctx->sample_fmt, tx->_aac_ctx->sample_rate,
			channel_layout, (AVSampleFormat)src->format, src->sample_rate,
			0, NULL);
		if (tx->_swr_ctx == NULL)
			return -ENOMEM;

		int result = swr_init(tx->_swr_ctx);
		if (result < 0) {
			TRACE("swr_init(%s, %lld, %d, %d) = %d.\n",
				codec->name, tx->_aac_ctx->channel_layout,
				tx->_aac_ctx->sample_fmt, tx->_aac_ctx->sample_rate, result);
			return result;
		}
	}

	int result = avcodec_open2(tx->_aac_ctx, codec, NULL);
	if (result < 0) {
		TRACE("avcodec_open2(%s) = %d.\n", codec->name, result);
		return result;
	}

	result = av_samples_get_buffer_size(&tx->_araw_linesize,
		src->channels, tx->_aac_ctx->frame_size, (AVSampleFormat)src->format, 0);
	if (result < 0) {
		TRACE("av_samples_get_buffer_size(%s, %d, %d, %d) = %d.\n",
			tx->_araw_ctx->codec->name,
			src->channels, tx->_aac_ctx->frame_size, src->format, result);
		return result;
	}
	tx->_araw_planar_count = (av_sample_fmt_is_planar((AVSampleFormat)src->format) ? src->channels : 1);

	result = av_samples_get_buffer_size(&tx->_aac_linesize,
		tx->_aac_ctx->channels, tx->_aac_ctx->frame_size, tx->_aac_ctx->sample_fmt, 0);
	if (result < 0) {
		TRACE("av_samples_get_buffer_size(%s, %d, %d, %d) = %d.\n", codec->name,
			tx->_aac_ctx->channels, tx->_aac_ctx->frame_size, tx->_aac_ctx->sample_fmt, result);
		return result;
	}
	tx->_aac_planar_count = (av_sample_fmt_is_planar(tx->_aac_ctx->sample_fmt) ? tx->_aac_ctx->channels : 1);

	if (tx->_swr_ctx != NULL) {
		int linesize[AV_NUM_DATA_POINTERS];
		result = av_samples_alloc(tx->_swr_buf, linesize, tx->_aac_ctx->channels,
			tx->_aac_ctx->frame_size, tx->_aac_ctx->sample_fmt, 0);
		if (result < 0) {
			TRACE("av_samples_alloc(%s, %d, %d, %d) = %d.\n",
				codec->name, tx->_aac_ctx->channels,
				tx->_aac_ctx->frame_size, tx->_aac_ctx->sample_fmt, result);
			return result;
		}
		assert(linesize[0] == tx->_aac_linesize);
	}

	//avcodec_parameters_from_context(&tx->_aac_info.param, tx->_aac_ctx);
	//if (tx->_aac_info.param.bits_per_raw_sample == 0)
	//	tx->_aac_info.param.bits_per_raw_sample = 8*av_get_bytes_per_sample(tx->_aac_ctx->sample_fmt);
	return result;
}

// avcodec_parameters_from_context()
static int get_sinfo_from_context(AStreamInfo **pinfo, AVCodecContext *codec)
{
	int result = 1;
	if ((*pinfo)->extra_bufsiz < codec->extradata_size) {
		// sinfo_realloc()
		result = AStreamComponentModule::get()->sinfo_clone(pinfo, NULL, codec->extradata_size);
		if (result < 0)
			return result;
	}
	AVCodecParameters *par = &((*pinfo)->param);

	par->codec_type = codec->codec_type;
	par->codec_id   = codec->codec_id;
	par->codec_tag  = codec->codec_tag;

	par->bit_rate              = codec->bit_rate;
	par->bits_per_coded_sample = codec->bits_per_coded_sample;
	par->bits_per_raw_sample   = codec->bits_per_raw_sample;
	par->profile               = codec->profile;
	par->level                 = codec->level;

	switch (par->codec_type) {
	case AVMEDIA_TYPE_VIDEO:
		par->format              = codec->pix_fmt;
		par->width               = codec->width;
		par->height              = codec->height;
		par->field_order         = codec->field_order;
		par->color_range         = codec->color_range;
		par->color_primaries     = codec->color_primaries;
		par->color_trc           = codec->color_trc;
		par->color_space         = codec->colorspace;
		par->chroma_location     = codec->chroma_sample_location;
		par->sample_aspect_ratio = codec->sample_aspect_ratio;
		par->video_delay         = codec->has_b_frames;
		//par->sample_rate         = codec->framerate; // re-mark as fps (frame rate)
		break;
	case AVMEDIA_TYPE_AUDIO:
		par->format           = codec->sample_fmt;
		par->channel_layout   = codec->channel_layout;
		par->channels         = codec->channels;
		par->sample_rate      = codec->sample_rate;
		par->block_align      = codec->block_align;
		par->frame_size       = codec->frame_size;
		par->initial_padding  = codec->initial_padding;
		par->trailing_padding = codec->trailing_padding;
		par->seek_preroll     = codec->seek_preroll;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		par->width  = codec->width;
		par->height = codec->height;
		break;
	}

	if (codec->extradata) {
		memcpy(par->extradata, codec->extradata, codec->extradata_size);
	}
	par->extradata_size = codec->extradata_size;
	return result;
}

static int ffmpeg_tx_create(AObject **object, AObject *parent, AOption *options)
{
	FFmpegAudioTranscode *tx = (FFmpegAudioTranscode*)object;
	tx->_araw_ctx            = tx->_aac_ctx = NULL;
	memzero(tx->_araw_frame);
	memzero(tx->_araw_buf);    tx->_aac_buf = NULL;
	tx->_araw_linesize       = tx->_aac_linesize = 0;
	tx->_araw_planar_count   = tx->_aac_planar_count = 0;

	tx->_aac_pts = AV_NOPTS_VALUE;
	tx->_swr_ctx = NULL;       memzero(tx->_swr_buf);
	return 1;
}

static void ffmpeg_tx_release(AObject *object)
{
	FFmpegAudioTranscode *tx = (FFmpegAudioTranscode*)object;

	reset_nif(tx->_araw_ctx, NULL, avcodec_free_context(&tx->_araw_ctx));
	for (int ix = 0; ix < AV_NUM_DATA_POINTERS; ++ix)
		release_s(tx->_araw_buf[ix]);

	reset_nif(tx->_aac_ctx, NULL, avcodec_free_context(&tx->_aac_ctx));
	release_s(tx->_aac_buf);
	reset_nif(tx->_swr_ctx, NULL, swr_free(&tx->_swr_ctx));
	reset_nif(tx->_swr_buf[0], NULL, av_freep(tx->_swr_buf));
}

static int ffmpeg_tx_open(AComponent *c, AStreamInfo **pinfo, AVCodecParameters *src)
{
	FFmpegAudioTranscode *tx = (FFmpegAudioTranscode*)c;
	assert(strcmp(c->_name, FFmpegAudioTranscode::name()) == 0);

	int result = open_decoder(tx, src);
	if (result < 0)
		return result;

	result = open_encoder(tx, &((*pinfo)->param), src);
	if (result < 0)
		return result;

	result = get_sinfo_from_context(pinfo, tx->_aac_ctx);
	if (result < 0)
		return result;
	return result;
}

static int ffmpeg_tx_transcode(AComponent *c, AVPacket *dest, AVPacket *src)
{
	FFmpegAudioTranscode *tx = (FFmpegAudioTranscode*)c;
	assert(strcmp(c->_name, FFmpegAudioTranscode::name()) == 0);

	reset_nif(dest->buf, NULL, ((ARefsBuf*)dest->buf)->release());
	ARefsBuf *src_buf = (ARefsBuf*)src->buf; src->buf = NULL;

	//int got = 0;
	//int result = avcodec_decode_audio4(tx->_araw_ctx, &tx->_araw_frame, &got, src);
	int result = avcodec_send_packet(tx->_araw_ctx, src);
	if (result >= 0) {
		result = avcodec_receive_frame(tx->_araw_ctx, &tx->_araw_frame);
	}
	src->buf = (AVBufferRef*)src_buf;

	if ((result < 0) /*|| !got*/) {
		TRACE("avcodec_decode_audio4(%s, %d) = %d.\n",
			tx->_araw_ctx->codec->name, src->size, result);
		av_frame_unref(&tx->_araw_frame);
		return (result < 0) ? result : 0;
	}

	int araw_framesize = tx->_araw_frame.linesize[0];
	for (int ix = 0; ix < tx->_araw_planar_count; ++ix) {
		result = ARefsBuf::reserve(tx->_araw_buf[ix], araw_framesize, 2*1024);
		if (result < 0) {
			av_frame_unref(&tx->_araw_frame);
			return result;
		}
		tx->_araw_buf[ix]->mempush(tx->_araw_frame.data[ix], araw_framesize);
	}
	av_frame_unref(&tx->_araw_frame);

	int got_count = 0;
	while (tx->_araw_buf[0]->len() >= tx->_araw_linesize) {
		if (ARefsBuf::reserve(tx->_aac_buf, tx->_aac_linesize, 2*1024) < 0) {
			result = -ENOMEM;
			break;
		}

		for (int ix = 0; ix < tx->_araw_planar_count; ++ix) {
			tx->_araw_frame.data[ix] = (uint8_t*)tx->_araw_buf[ix]->ptr();
			tx->_araw_frame.linesize[ix] = tx->_araw_linesize;
		}

		if (tx->_swr_ctx != NULL) {
			result = swr_convert(tx->_swr_ctx, tx->_swr_buf, tx->_aac_ctx->frame_size,
				(const uint8_t**)tx->_araw_frame.data, tx->_aac_ctx->frame_size);
			if (result < 0) {
				TRACE("swr_convert(%s, %d, %d, %d, %d) = %d.\n",
					tx->_aac_ctx->codec->name,
					tx->_aac_ctx->channels, tx->_aac_ctx->sample_fmt,
					tx->_aac_ctx->sample_rate, tx->_aac_ctx->frame_size, result);
				break;
			}
			for (int ix = 0; ix < tx->_aac_planar_count; ++ix) {
				tx->_araw_frame.data[ix] = tx->_swr_buf[ix];
				tx->_araw_frame.linesize[ix] = tx->_aac_linesize;
			}
		}

		tx->_araw_frame.extended_data  = tx->_araw_frame.data;
		tx->_araw_frame.nb_samples     = tx->_aac_ctx->frame_size;
		tx->_araw_frame.format         = tx->_aac_ctx->sample_fmt;
		tx->_araw_frame.sample_rate    = tx->_aac_ctx->sample_rate;
		tx->_araw_frame.channel_layout = tx->_aac_ctx->channel_layout;

		dest->data = (uint8_t*)tx->_aac_buf->next();
		dest->size = tx->_aac_buf->left();
		//got = 0;
		//result = avcodec_encode_audio2(tx->_aac_ctx, dest, &tx->_araw_frame, &got);
		result = avcodec_send_frame(tx->_aac_ctx, &tx->_araw_frame);
		for (int ix = 0; ix < tx->_araw_planar_count; ++ix)
			tx->_araw_buf[ix]->pop(tx->_araw_linesize);

		if (result >= 0)
			result = avcodec_receive_packet(tx->_aac_ctx, dest);
		if (result < 0) {
			TRACE("avcodec_encode_audio2(%s, %d) = %d.\n",
				tx->_aac_ctx->codec->name, tx->_araw_frame.linesize[0], result);
		} else /*if (got)*/ {
			tx->_aac_buf->push(dest->size);
			++got_count;
			// TODO ??
			//pkt.data = NULL;
			//pkt.size = 0;
			av_packet_unref(dest);
		}
	}
	if (result < 0) {
		for (int ix = 0; ix < tx->_araw_planar_count; ++ix) {
			tx->_araw_buf[ix]->reset();
		}
	} else if (tx->_araw_buf[0]->left() < araw_framesize) {
		for (int ix = 0; ix < tx->_araw_planar_count; ++ix) {
			tx->_araw_buf[ix]->rewind();
		}
	}

	if ((tx->_aac_buf != NULL) && (tx->_aac_buf->len() > 0)) {
		dest->buf = (AVBufferRef*)tx->_aac_buf; tx->_aac_buf->addref();
		dest->data = (uint8_t*)tx->_aac_buf->ptr();
		dest->size = tx->_aac_buf->len(); tx->_aac_buf->pop(tx->_aac_buf->len());
		dest->duration = (tx->_aac_ctx->frame_size * got_count * (int64_t)AV_TIME_BASE)/tx->_aac_ctx->sample_rate;

		if (tx->_aac_pts == AV_NOPTS_VALUE)
			tx->_aac_pts = src->pts;
		dest->pts = tx->_aac_pts;
		tx->_aac_pts += dest->duration;
	}
	return result;
}

static int64_t ffmpeg_tx_reset_pts(AComponent *c, int64_t pts)
{
	FFmpegAudioTranscode *tx = (FFmpegAudioTranscode*)c;
	int64_t last_pts = tx->_aac_pts;
	tx->_aac_pts = pts;
	return last_pts;
}

TranscodeModule FFmpegTxModule = { {
	FFmpegAudioTranscode::name(),
	FFmpegAudioTranscode::name(),
	sizeof(FFmpegAudioTranscode),
	NULL, NULL,
	&ffmpeg_tx_create, &ffmpeg_tx_release,
},
	&ffmpeg_tx_open,
	&ffmpeg_tx_transcode,
	&ffmpeg_tx_reset_pts,
};
static int reg_ffmpegtx = AModuleRegister(&FFmpegTxModule.module);
