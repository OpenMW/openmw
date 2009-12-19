/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (avcodec.d) is part of the OpenMW package.

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

module sound.avcodec;

extern (C):

// A unique handle that represents an AV file
typedef void* AVFile;

// A unique handle representing an audio stream
typedef void* AVAudio;

// In case we ever decide to implement more codec backends, here's what
// these functions need to do...

// Open the named file, and return a unique handle representing it.
// Returns NULL on error
AVFile avc_openAVFile(char *fname);

// Close the file handle, invalidating all streams taken from it
void avc_closeAVFile(AVFile file);

// Get a unique handle to an audio stream in the file. The given number
// is for files that can contain multiple audio streams (generally you
// would pass 0, for the first audio stream)
AVAudio avc_getAVAudioStream(AVFile file, int streamnum);

// Get audio info representing the current stream. Returns 0 for success
// (not likely to fail)
int avc_getAVAudioInfo(AVAudio stream, int *rate, int *channels, int *bits);

// Decode the next bit of data for the given audio stream. The function
// must provide no less than the requested number of bytes, except for
// end-of-stream conditions, and is responsible for buffering data. For
// files with multiple streams, it must take care to preserve  data for
// any stream that has had a stream handle returned.
// eg. if a file has one video stream and 2 audio streams and the app
// gets a handle to the video stream and one audio stream, it must
// not destroy video data for subsequent calls to avc_getAVVideoData if
// it has to read over it while decoding the audio stream. The other
// audio stream's data, however, may be discarded.
// Returns the number of bytes written to the buffer, which will be no
// more than the provided length.
int avc_getAVAudioData(AVAudio stream, void *data, int length);
