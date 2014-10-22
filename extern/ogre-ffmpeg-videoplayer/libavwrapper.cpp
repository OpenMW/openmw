// This file is a wrapper around the libavresample library (the API-incompatible swresample replacement in the libav fork of ffmpeg), to make it look like swresample to the user.

#ifndef HAVE_LIBSWRESAMPLE
extern "C"
{
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <stdint.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
// From libavutil version 52.2.0 and onward the declaration of
// AV_CH_LAYOUT_* is removed from libavcodec/avcodec.h and moved to
// libavutil/channel_layout.h
#if AV_VERSION_INT(52, 2, 0) <= AV_VERSION_INT(LIBAVUTIL_VERSION_MAJOR, \
    LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO)
    #include <libavutil/channel_layout.h>
#endif
#include <libavresample/avresample.h>
#include <libavutil/opt.h>

/* FIXME: delete this file once libswresample is packaged for Debian */

int  swr_init(AVAudioResampleContext *avr) { return 1; }

void  swr_free(AVAudioResampleContext **avr) { avresample_free(avr); }

int swr_convert(
        AVAudioResampleContext *avr,
        uint8_t** output,
        int out_samples,
        const uint8_t** input,
        int in_samples)
{
    // FIXME: potential performance hit
    int out_plane_size = 0;
    int in_plane_size = 0;
    return avresample_convert(avr, output, out_plane_size, out_samples,
                              (uint8_t **)input, in_plane_size, in_samples);
}

AVAudioResampleContext * swr_alloc_set_opts(
        AVAudioResampleContext *avr,
        int64_t out_ch_layout,
        AVSampleFormat out_fmt,
        int out_rate,
        int64_t in_ch_layout,
        AVSampleFormat in_fmt,
        int in_rate,
        int o,
        void* l)
{
    avr = avresample_alloc_context();
    if(!avr)
        return 0;

    int res;
    res = av_opt_set_int(avr, "out_channel_layout", out_ch_layout, 0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: out_ch_layout = %d\n", res);
        return 0;
    }
    res = av_opt_set_int(avr, "out_sample_fmt",     out_fmt,       0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: out_fmt = %d\n", res);
        return 0;
    }
    res = av_opt_set_int(avr, "out_sample_rate",    out_rate,      0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: out_rate = %d\n", res);
        return 0;
    }
    res = av_opt_set_int(avr, "in_channel_layout",  in_ch_layout,  0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: in_ch_layout = %d\n", res);
        return 0;
    }
    res = av_opt_set_int(avr, "in_sample_fmt",      in_fmt,        0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: in_fmt = %d\n", res);
        return 0;
    }
    res = av_opt_set_int(avr, "in_sample_rate",     in_rate,       0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: in_rate = %d\n", res);
        return 0;
    }
    res = av_opt_set_int(avr, "internal_sample_fmt",     AV_SAMPLE_FMT_FLTP,       0);
    if(res < 0)
    {
        av_log(avr, AV_LOG_ERROR, "av_opt_set_int: internal_sample_fmt\n");
        return 0;
    }


    if(avresample_open(avr) < 0)
    {
        av_log(avr, AV_LOG_ERROR, "Error opening context\n");
        return 0;
    }
    else
        return avr;
}

}
#endif
