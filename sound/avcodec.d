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
AVFile cpp_openAVFile(char *fname);

// Close the file handle, invalidating all streams taken from it
void cpp_closeAVFile(AVFile file);

// Get a unique handle to an audio stream in the file. The given number
// is for files that can contain multiple audio streams (generally you
// would pass 0, for the first audio stream)
AVAudio cpp_getAVAudioStream(AVFile file, int streamnum);

// Get audio info representing the current stream. Returns 0 for success
// (not likely to fail)
int cpp_getAVAudioInfo(AVAudio stream, int *rate, int *channels, int *bits);

// Decode the next bit of data for the given audio stream. The function
// must provide no less than the requested number of bytes, except for
// end-of-stream conditions, and is responsible for buffering data. For
// files with multiple streams, it must take care to preserve  data for
// any stream that has had a stream handle returned.
// eg. if a file has one video stream and 2 audio streams and the app
// gets a handle to the video stream and one audio stream, it must
// not destroy video data for subsequent calls to cpp_getAVVideoData if
// it has to read over it while decoding the audio stream. The other
// audio stream's data, however, may be discarded.
// Returns the number of bytes written to the buffer, which will be no
// more than the provided length.
int cpp_getAVAudioData(AVAudio stream, void *data, int length);
