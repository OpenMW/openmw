#include "videoplayer.hpp"

//#ifdef OPENMW_USE_FFMPEG

#include <OgreMaterialManager.h>
#include <OgreSceneManager.h>
#include <OgreMaterial.h>
#include <OgreHardwarePixelBuffer.h>


extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"


namespace MWRender
{

    int OgreResource_Read(void *opaque, uint8_t *buf, int buf_size)
    {
        Ogre::DataStreamPtr stream = *((Ogre::DataStreamPtr*)opaque);

        int num_read = stream->size() - stream->tell();

        if (num_read > buf_size)
            num_read = buf_size;

        stream->read(buf, num_read);
        return num_read;
    }

    int OgreResource_Write(void *opaque, uint8_t *buf, int buf_size)
    {
        Ogre::DataStreamPtr stream = *((Ogre::DataStreamPtr*)opaque);

        int num_write = stream->size() - stream->tell();

        if (num_write > buf_size)
            num_write = buf_size;

        stream->write (buf, num_write);
        return num_write;
    }

    int64_t OgreResource_Seek(void *opaque, int64_t offset, int whence)
    {
        Ogre::DataStreamPtr stream = *((Ogre::DataStreamPtr*)opaque);

        switch (whence)
        {
        case SEEK_SET:
            stream->seek(offset);
        case SEEK_CUR:
            stream->seek(stream->tell() + offset);
        case SEEK_END:
            stream->seek(stream->size() + offset);
        case AVSEEK_SIZE:
            return stream->size();
        default:
            return -1;
        }

        return stream->tell();
    }

    VideoPlayer::VideoPlayer(Ogre::SceneManager *sceneMgr)
        : mAvContext(NULL)
        , mVideoStreamId(-1)
        , mAudioStreamId(-1)
    {
        Ogre::MaterialPtr videoMaterial = Ogre::MaterialManager::getSingleton ().create("VideoMaterial", "General");
        videoMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        videoMaterial->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
        videoMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);
        mTextureUnit = videoMaterial->getTechnique(0)->getPass(0)->createTextureUnitState();

        mRectangle = new Ogre::Rectangle2D(true);
        mRectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
        mRectangle->setMaterial("VideoMaterial");
        mRectangle->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY+1);
        // Use infinite AAB to always stay visible
        Ogre::AxisAlignedBox aabInf;
        aabInf.setInfinite();
        mRectangle->setBoundingBox(aabInf);
        // Attach background to the scene
        Ogre::SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(mRectangle);
        mRectangle->setVisible(false);
        mRectangle->setVisibilityFlags (0x1);
    }

    VideoPlayer::~VideoPlayer()
    {
        if (mAvContext)
            deleteContext();

        delete mRectangle;
    }

    void VideoPlayer::play (const std::string& resourceName)
    {
        mStream = Ogre::ResourceGroupManager::getSingleton ().openResource (resourceName);


        mVideoStreamId = -1;
        mAudioStreamId = -1;
        mEOF = false;
        mDisplayedFrameCount = 0;

        // if something is already playing, close it
        if (mAvContext)
            close();

        mRectangle->setVisible(true);

        MWBase::Environment::get().getWindowManager ()->pushGuiMode (MWGui::GM_Video);


        // BASIC INITIALIZATION

        // Load all the decoders
        av_register_all();

        AVIOContext	 *ioContext = 0;

        int err = 0;

        mAvContext = avformat_alloc_context();
        if (!mAvContext)
            throwError(0);

        ioContext = avio_alloc_context(NULL, 0, 0, &mStream, OgreResource_Read, OgreResource_Write, OgreResource_Seek);
        if (!ioContext)
            throwError(0);

        mAvContext->pb = ioContext;

        err = avformat_open_input(&mAvContext, resourceName.c_str(), NULL, NULL);
        if (err != 0)
            throwError(err);

        err = avformat_find_stream_info(mAvContext, 0);
        if (err < 0)
            throwError(err);




        // Find the video stream among the different streams
        for (unsigned int i = 0; i < mAvContext->nb_streams; i++)
        {
            if (mAvContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                mVideoStreamId = i;
                mVideoStream = mAvContext->streams[i];
                break;
            }
        }

        if (mVideoStreamId < 0)
            throw std::runtime_error("No video stream found in the video");

        // Get the video decoder
        mVideoCodec = avcodec_find_decoder(mVideoStream->codec->codec_id);
        if (NULL == mVideoCodec)
            throw std::runtime_error("No video decoder found");

        // Load the video codec
        err = avcodec_open2(mVideoStream->codec, mVideoCodec, 0);
        if (err < 0)
            throwError (err);



        // Find the audio stream among the different streams
        for (unsigned int i = 0; i < mAvContext->nb_streams; i++)
        {
            if (mAvContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                mAudioStreamId = i;
                mAudioStream = mAvContext->streams[i];
                break;
            }
        }

        if (-1 == mAudioStreamId)
            throw std::runtime_error("No audio stream found in the video");

        // Get the audio decoder
        mAudioCodec = avcodec_find_decoder(mAudioStream->codec->codec_id);
        if (mAudioCodec == NULL)
        {
            throw std::runtime_error("Stream doesn't have an audio codec");
        }

        // Load the audio codec
        err = avcodec_open2(mAudioStream->codec, mAudioCodec, 0);
        if (err < 0)
            throwError (err);



        // Create the frame buffers
        mRawFrame = avcodec_alloc_frame();
        mRGBAFrame = avcodec_alloc_frame();
        if (!mRawFrame || !mRGBAFrame)
        {
            throw std::runtime_error("Can't allocate video frames");
        }


        avpicture_alloc ((AVPicture *)mRGBAFrame, PIX_FMT_RGBA, mVideoStream->codec->width, mVideoStream->codec->height);


        // Setup the image scaler
        // All this does is convert from YUV to RGB - note it would be faster to do this in a shader,
        // but i'm not worried about performance just yet
        mSwsContext = sws_getContext(mVideoStream->codec->width, mVideoStream->codec->height,
            mVideoStream->codec->pix_fmt,
            mVideoStream->codec->width, mVideoStream->codec->height,
            PIX_FMT_RGBA,
            SWS_BICUBIC, NULL, NULL, NULL);
        if (!mSwsContext)
            throw std::runtime_error("Can't create SWS Context");

        // Get the frame time we need for this video
        AVRational r = mVideoStream->avg_frame_rate;
        AVRational r2 = mVideoStream->r_frame_rate;
        if ((!r.num || !r.den) &&
            (!r2.num || !r2.den))
        {
            std::cerr << "Warning - unable to get the video frame rate. Using standard NTSC frame rate : 29.97 fps." << std::endl;
            mWantedFrameTime = 1.f / 29.97f;
        }
        else
        {
            if (r.num && r.den)
                mWantedFrameTime = 1.f/((float)r.num / r.den);
            else
                mWantedFrameTime = 1.f/((float)r2.num / r2.den);
        }


        mTextureUnit->setTextureName ("");

        if (!Ogre::TextureManager::getSingleton ().getByName("VideoTexture").isNull())
            Ogre::TextureManager::getSingleton().remove("VideoTexture");

        mVideoTexture = Ogre::TextureManager::getSingleton().createManual(
                    "VideoTexture",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D,
            mVideoStream->codec->width, mVideoStream->codec->height,
            0,
            Ogre::PF_BYTE_RGBA,
            Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

        // initialize to (0, 0, 0, 1)
        std::vector<Ogre::uint32> buffer;
        buffer.resize(mVideoStream->codec->width * mVideoStream->codec->height);
        for (int p=0; p<mVideoStream->codec->width * mVideoStream->codec->height; ++p)
        {
            buffer[p] = (255 << 24);
        }
        memcpy(mVideoTexture->getBuffer()->lock(Ogre::HardwareBuffer::HBL_DISCARD), &buffer[0], mVideoStream->codec->width*mVideoStream->codec->height*4);
        mVideoTexture->getBuffer()->unlock();

        mTextureUnit->setTextureName ("VideoTexture");

        mTimer.reset();

    }

    void VideoPlayer::throwError(int error)
    {
        char buffer[4096] = {0};

        if (0 == av_strerror(error, buffer, sizeof(buffer)))
        {
            std::stringstream msg;
            msg << "FFMPEG error: ";
            msg << buffer << std::endl;
            throw std::runtime_error(msg.str());
        }
        else
            throw std::runtime_error("Unknown FFMPEG error");
    }

    bool VideoPlayer::readFrameAndQueue ()
    {
        bool ret = true;
        AVPacket *pkt = NULL;

        // check we're not at eof
        if (mEOF)
            ret = false;
        else
        {
            // read frame
            pkt = (AVPacket *)av_malloc(sizeof(*pkt));
            int res = av_read_frame(mAvContext, pkt);

            // check we didn't reach eof right now
            if (res < 0)
            {
                mEOF = true;
                ret = false;
                av_free(pkt);
            }
            else
            {
                // When a frame has been read, save it
                if (!saveFrame(pkt))
                {
                    // we failed to save it, which means it was unknown type of frame - delete it here
                    av_free_packet(pkt);
                    av_free(pkt);
                }
            }
        }

        return ret;
    }

    bool VideoPlayer::saveFrame(AVPacket* frame)
    {
        bool saved = false;

        if (frame->stream_index == mVideoStreamId)
        {
            // If it was a video frame...
            mVideoPacketQueue.push(frame);
            saved = true;
        }

        return saved;

    }

    void VideoPlayer::update()
    {
        if (!mAvContext)
            return;

        // Time elapsed since the video started
        float realTime = mTimer.getMilliseconds ()/1000.f;

        // Here is the time we're at in the video
        float movieTime = mDisplayedFrameCount * mWantedFrameTime;

        if (movieTime >= realTime)
            return;

        if (!mVideoPacketQueue.size() && mEOF)
            close();

        Ogre::Timer timer;

        if (!mVideoPacketQueue.size())
        {
            if (readFrameAndQueue())
                decodeFrontFrame();
        }
        else
            decodeFrontFrame();

        mDecodingTime = timer.getMilliseconds ()/1000.f;
    }

    void VideoPlayer::decodeFrontFrame ()
    {
        int didDecodeFrame = 0;

        // Make sure there is something to decode
        if (!mVideoPacketQueue.size())
            return;

        // Get the front frame and decode it
        AVPacket *videoPacket = mVideoPacketQueue.front();
        int res;
        res = avcodec_decode_video2(mVideoStream->codec, mRawFrame, &didDecodeFrame, videoPacket);

        if (res < 0 || !didDecodeFrame)
            throw std::runtime_error ("an error occured while decoding the video frame");

        // Convert the frame to RGB
        sws_scale(mSwsContext,
            mRawFrame->data, mRawFrame->linesize,
            0, mVideoStream->codec->height,
            mRGBAFrame->data, mRGBAFrame->linesize);


        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = mVideoTexture->getBuffer();
        Ogre::PixelBox pb(mVideoStream->codec->width, mVideoStream->codec->height, 1, Ogre::PF_BYTE_RGBA, mRGBAFrame->data[0]);
        pixelBuffer->blitFromMemory(pb);

        av_free_packet(mVideoPacketQueue.front());
        av_free(mVideoPacketQueue.front());
        mVideoPacketQueue.pop();

        ++mDisplayedFrameCount;
    }

    void VideoPlayer::close ()
    {
        mRectangle->setVisible (false);
        MWBase::Environment::get().getWindowManager ()->removeGuiMode (MWGui::GM_Video);
        deleteContext();
    }

    void VideoPlayer::deleteContext()
    {
        while (mVideoPacketQueue.size())
        {
            av_free_packet(mVideoPacketQueue.front());
            av_free(mVideoPacketQueue.front());
            mVideoPacketQueue.pop();
        }

        if (mVideoStream && mVideoStream->codec != NULL) avcodec_close(mVideoStream->codec);
        if (mAudioStream && mAudioStream->codec != NULL) avcodec_close(mAudioStream->codec);

        avpicture_free((AVPicture *)mRGBAFrame);

        if (mRawFrame)
            av_free(mRawFrame);
        if (mRGBAFrame)
            av_free(mRGBAFrame);

        sws_freeContext(mSwsContext);

        avformat_close_input(&mAvContext);

        mAvContext = NULL;
    }
}

//#endif
