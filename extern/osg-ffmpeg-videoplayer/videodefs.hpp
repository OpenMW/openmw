#ifndef VIDEOPLAYER_DEFS_H
#define VIDEOPLAYER_DEFS_H

namespace Video
{

enum {
    AV_SYNC_AUDIO_MASTER, // Play audio with no frame drops, sync video to audio
    AV_SYNC_VIDEO_MASTER, // Play video with no frame drops, sync audio to video
    AV_SYNC_EXTERNAL_MASTER, // Sync audio and video to an external clock

    AV_SYNC_DEFAULT = AV_SYNC_EXTERNAL_MASTER
};

}

#endif
