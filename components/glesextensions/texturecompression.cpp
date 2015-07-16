#include <string>
#include "texturecompression.h"

static const char* currentTextureCompression;
static osg::Texture::InternalFormatMode osgTextureCompressionMode;


class TextureCompressionModeSaver {
public:
	static void setTextureCompressionMode(char const *data);
	static char const * getCurrentTextureCompressionMode();
};


JNIEXPORT void JNICALL Java_ui_activity_GameActivity_saveCurrentTextureCompressionMode(
		JNIEnv *env, jobject obj, jstring prompt) {
	jboolean iscopy;
	TextureCompressionModeSaver::setTextureCompressionMode(
			(env)->GetStringUTFChars(prompt, &iscopy));
	GlesTextureCompression::computeCurrentTextureCompression();
	(env)->DeleteLocalRef(prompt);
}


 void TextureCompressionModeSaver::setTextureCompressionMode(char const *data) {
 currentTextureCompression = data;
 }
 char const * TextureCompressionModeSaver::getCurrentTextureCompressionMode() {
 return currentTextureCompression;
 }

 osg::Texture::InternalFormatMode GlesTextureCompression::getGLesOsgTextureCompression() {
 return osgTextureCompressionMode;
 }

 void GlesTextureCompression::computeCurrentTextureCompression() {

 std::string currentTextureCompression = "";
 currentTextureCompression = currentTextureCompression
 + TextureCompressionModeSaver::getCurrentTextureCompressionMode();
 if (currentTextureCompression == "ETC2")
 osgTextureCompressionMode = osg::Texture::USE_ETC2_COMPRESSION;
 else if (currentTextureCompression == "ETC1")//ETC1 does`not support alpha channel
 osgTextureCompressionMode = osg::Texture::USE_ETC_COMPRESSION;
 else if (currentTextureCompression == "PVR")
 osgTextureCompressionMode = osg::Texture::USE_PVRTC_4BPP_COMPRESSION;
 else if (currentTextureCompression == "DXT")
 osgTextureCompressionMode = osg::Texture::USE_IMAGE_DATA_FORMAT;
 else if (currentTextureCompression == "UNKNOWN")
 osgTextureCompressionMode = osg::Texture::USE_IMAGE_DATA_FORMAT;
 }

