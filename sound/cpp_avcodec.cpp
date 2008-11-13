/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_avcodec.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#include <stdio.h>
#include <string.h>

extern "C" { // the headers don't do this..
#ifdef WIN32
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#else
// FIXME: This works on Ubuntu (given the switches from pkg-config),
// but apparently there are some problems on other systems (eg
// Archlinux).
#include <avcodec.h>
#include <avformat.h>
#endif
}

#include <vector>

using std::vector;

struct MyFile {
    AVFormatContext *FmtCtx;
    struct MyStream {
        MyFile *parent;

        AVCodecContext *CodecCtx;
        int StreamNum;

        vector<uint8_t> Data;
        vector<char> DecodedData;
    };
    vector<MyStream*> Streams;
};

// TODO:
// extern "C" MyFile::MyStream *avc_getAVVideoStream(MyFile *file, int streamnum);
// extern "C" int avc_getAVVideoInfo(MyFile::MyStream *stream, float *fps, int *width, int * height);
// extern "C" int avc_getAVVideoData(MyFile::MyStream *stream, char *data, int length);

extern "C" MyFile *avc_openAVFile(char *fname)
{
    static bool done = false;
    if(!done) { av_register_all();
    av_log_set_level(AV_LOG_ERROR);}
    done = true;

    MyFile *file = new MyFile;
    if(av_open_input_file(&file->FmtCtx, fname, NULL, 0, NULL) == 0)
    {
        if(av_find_stream_info(file->FmtCtx) >= 0)
            return file;
        av_close_input_file(file->FmtCtx);
    }
    delete file;
    return NULL;
}

extern "C" void avc_closeAVFile(MyFile *file)
{
    if(!file) return;

    for(size_t i = 0;i < file->Streams.size();i++)
    {
        avcodec_close(file->Streams[i]->CodecCtx);
        file->Streams[i]->Data.clear();
        file->Streams[i]->DecodedData.clear();
        delete file->Streams[i];
    }
    file->Streams.clear();

    av_close_input_file(file->FmtCtx);
    delete file;
}

extern "C" MyFile::MyStream *avc_getAVAudioStream(MyFile *file, int streamnum)
{
    if(!file) return NULL;
    for(unsigned int i = 0;i < file->FmtCtx->nb_streams;i++)
    {
        if(file->FmtCtx->streams[i]->codec->codec_type != CODEC_TYPE_AUDIO)
            continue;

        if(streamnum == 0)
        {
            MyFile::MyStream *stream = new MyFile::MyStream;
            stream->parent = file;
            stream->CodecCtx = file->FmtCtx->streams[i]->codec;
            stream->StreamNum = i;

            AVCodec *codec = avcodec_find_decoder(stream->CodecCtx->codec_id);
            if(!codec || avcodec_open(stream->CodecCtx, codec) < 0)
            {
                delete stream;
                return NULL;
            }

            file->Streams.push_back(stream);
            return stream;
        }
        streamnum--;
    }
    return NULL;
}

extern "C" int avc_getAVAudioInfo(MyFile::MyStream *stream,
                                  int *rate, int *channels, int *bits)
{
    if(!stream) return 1;

    if(rate) *rate = stream->CodecCtx->sample_rate;
    if(channels) *channels = stream->CodecCtx->channels;
    if(bits) *bits = 16;

    return 0;
}

static void getNextPacket(MyFile *file, int streamidx)
{
    AVPacket packet;
    while(av_read_frame(file->FmtCtx, &packet) >= 0)
    {
        for(vector<MyFile::MyStream*>::iterator i = file->Streams.begin();
            i != file->Streams.end();i++)
        {
            if((*i)->StreamNum == packet.stream_index)
            {
                size_t idx = (*i)->Data.size();
                (*i)->Data.resize(idx + packet.size);
                memcpy(&(*i)->Data[idx], packet.data, packet.size);
                if(streamidx == packet.stream_index)
                {
                    av_free_packet(&packet);
                    return;
                }
                break;
            }
        }
        av_free_packet(&packet);
    }
}

extern "C" int avc_getAVAudioData(MyFile::MyStream *stream, char *data, int length)
{
    if(!stream) return 0;

    int dec = 0;
    while(dec < length)
    {
        if(stream->DecodedData.size() > 0)
        {
            size_t rem = length-dec;
            if(rem > stream->DecodedData.size())
                rem = stream->DecodedData.size();

            memcpy(data, &stream->DecodedData[0], rem);
            data += rem;
            dec += rem;
            if(rem < stream->DecodedData.size())
                memmove(&stream->DecodedData[0], &stream->DecodedData[rem],
                        stream->DecodedData.size() - rem);
            stream->DecodedData.resize(stream->DecodedData.size()-rem);
        }
        if(stream->DecodedData.size() == 0)
        {
            // Must always get at least one more packet if possible, in case
            // the previous one wasn't enough
            getNextPacket(stream->parent, stream->StreamNum);
            int insize = stream->Data.size();
            if(insize == 0)
                break;

            // Temporarilly add padding to the input data since some
            // codecs read in larger chunks and may accidently read
            // past the end of the allocated buffer
            stream->Data.resize(insize + FF_INPUT_BUFFER_PADDING_SIZE);
            memset(&stream->Data[insize], 0, FF_INPUT_BUFFER_PADDING_SIZE);
            stream->DecodedData.resize(AVCODEC_MAX_AUDIO_FRAME_SIZE);

            int16_t *ptr = (int16_t*)&stream->DecodedData[0];
            int size = stream->DecodedData.size();
            int len = avcodec_decode_audio2(stream->CodecCtx, ptr, &size,
                                            &stream->Data[0], insize);
            if(len < 0)
            {
                stream->Data.resize(insize);
                break;
            }
            if(len > 0)
            {
                int datarem = insize-len;
                if(datarem)
                    memmove(&stream->Data[0], &stream->Data[len], datarem);
                stream->Data.resize(datarem);
            }

            stream->DecodedData.resize(size);
        }
    }
    return dec;
}
