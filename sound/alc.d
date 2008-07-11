extern(System):

//Definitions
const int ALC_FALSE                             =  0;
const int ALC_TRUE                              =  1;

const int ALC_FREQUENCY                         =  0x1007;
const int ALC_REFRESH                           =  0x1008;
const int ALC_SYNC                              =  0x1009;
const int ALC_MONO_SOURCES                      =  0x1010;
const int ALC_STEREO_SOURCES                    =  0x1011;
const int ALC_NO_ERROR                          =  ALC_FALSE;
const int ALC_INVALID_DEVICE                    =  0xA001;
const int ALC_INVALID_CONTEXT                   =  0xA002;
const int ALC_INVALID_ENUM                      =  0xA003;
const int ALC_INVALID_VALUE                     =  0xA004;
const int ALC_OUT_OF_MEMORY                     =  0xA005;
const int ALC_DEFAULT_DEVICE_SPECIFIER          =  0x1004;
const int ALC_DEVICE_SPECIFIER                  =  0x1005;
const int ALC_EXTENSIONS                        =  0x1006;
const int ALC_MAJOR_VERSION                     =  0x1000;
const int ALC_MINOR_VERSION                     =  0x1001;
const int ALC_ATTRIBUTES_SIZE                   =  0x1002;
const int ALC_ALL_ATTRIBUTES                    =  0x1003;

const int ALC_CAPTURE_DEVICE_SPECIFIER          =  0x310;
const int ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER  =  0x311;
const int ALC_CAPTURE_SAMPLES                   =  0x312;

//Device and context structures
alias void ALCdevice;
alias void ALCcontext;

//Typedefs
alias char ALCboolean;
alias char ALCchar;
alias char ALCbyte;
alias ubyte ALCubyte;
alias short ALCshort;
alias ushort ALCushort;
alias int ALCint;
alias uint ALCuint;
alias int ALCsizei;
alias int ALCenum;
alias float ALCfloat;
alias double ALCdouble;
alias void ALCvoid;

//Context functions
ALCcontext* alcCreateContext( ALCdevice *device, ALCint* attrlist );
ALCboolean alcMakeContextCurrent( ALCcontext *context );
void alcProcessContext( ALCcontext *context );
void alcSuspendContext( ALCcontext *context );
void alcDestroyContext( ALCcontext *context );
ALCcontext* alcGetCurrentContext();
ALCdevice* alcGetContextsDevice( ALCcontext *context );
ALCdevice * alcOpenDevice( ALCchar *devicename );
ALCboolean alcCloseDevice( ALCdevice *device );
ALCenum alcGetError( ALCdevice *device );
ALCboolean alcIsExtensionPresent( ALCdevice *device, ALCchar *extname );
void* alcGetProcAddress( ALCdevice *device, ALCchar *funcname );
ALCenum alcGetEnumValue( ALCdevice *device, ALCchar *enumname );
ALCchar* alcGetString( ALCdevice *device, ALCenum param );
void alcGetIntegerv( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data );
ALCdevice* alcCaptureOpenDevice( ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
ALCboolean alcCaptureCloseDevice( ALCdevice *device );
void alcCaptureStart( ALCdevice *device );
void alcCaptureStop( ALCdevice *device );
void alcCaptureSamples( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );

typedef ALCcontext * ( *LPALCCREATECONTEXT) (ALCdevice *device, ALCint *attrlist);
typedef ALCboolean ( *LPALCMAKECONTEXTCURRENT)( ALCcontext *context );
typedef void ( *LPALCPROCESSCONTEXT)( ALCcontext *context );
typedef void ( *LPALCSUSPENDCONTEXT)( ALCcontext *context );
typedef void ( *LPALCDESTROYCONTEXT)( ALCcontext *context );
typedef ALCcontext * ( *LPALCGETCURRENTCONTEXT)( );
typedef ALCdevice * ( *LPALCGETCONTEXTSDEVICE)( ALCcontext *context );
typedef ALCdevice * ( *LPALCOPENDEVICE)( ALCchar *devicename );
typedef ALCboolean ( *LPALCCLOSEDEVICE)( ALCdevice *device );
typedef ALCenum ( *LPALCGETERROR)( ALCdevice *device );
typedef ALCboolean ( *LPALCISEXTENSIONPRESENT)( ALCdevice *device, ALCchar *extname );
typedef void * ( *LPALCGETPROCADDRESS)(ALCdevice *device, ALCchar *funcname );
typedef ALCenum ( *LPALCGETENUMVALUE)(ALCdevice *device, ALCchar *enumname );
typedef const ALCchar* ( *LPALCGETSTRING)( ALCdevice *device, ALCenum param );
typedef void ( *LPALCGETINTEGERV)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *dest );
typedef ALCdevice * ( *LPALCCAPTUREOPENDEVICE)( ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
typedef ALCboolean ( *LPALCCAPTURECLOSEDEVICE)( ALCdevice *device );
typedef void ( *LPALCCAPTURESTART)( ALCdevice *device );
typedef void ( *LPALCCAPTURESTOP)( ALCdevice *device );
typedef void ( *LPALCCAPTURESAMPLES)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );
