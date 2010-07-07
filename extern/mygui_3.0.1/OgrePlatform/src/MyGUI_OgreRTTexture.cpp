/*!
	@file
	@author		Albert Semenov
	@date		10/2009
	@module
*/

#include <cstring>
#include "MyGUI_OgreRTTexture.h"
#include "MyGUI_OgreRenderManager.h"

namespace MyGUI
{

	OgreRTTexture::OgreRTTexture(Ogre::TexturePtr _texture) :
		mViewport(nullptr),
		mSaveViewport(nullptr),
		mTexture(_texture)
	{
		mProjectMatrix = Ogre::Matrix4::IDENTITY;
		Ogre::Root * root = Ogre::Root::getSingletonPtr();
		if (root != nullptr)
		{
			Ogre::RenderSystem* system = root->getRenderSystem();
			if (system != nullptr)
			{
				int width = mTexture->getWidth();
				int height = mTexture->getHeight();

				mRenderTargetInfo.maximumDepth = system->getMaximumDepthInputValue();
				mRenderTargetInfo.hOffset = system->getHorizontalTexelOffset() / float(width);
				mRenderTargetInfo.vOffset = system->getVerticalTexelOffset() / float(height);
				mRenderTargetInfo.aspectCoef = float(height) / float(width);
				mRenderTargetInfo.pixScaleX = 1.0 / float(width);
				mRenderTargetInfo.pixScaleY = 1.0 / float(height);
			}

			if (mTexture->getBuffer()->getRenderTarget()->requiresTextureFlipping())
			{
				mProjectMatrix[1][0] = -mProjectMatrix[1][0];
				mProjectMatrix[1][1] = -mProjectMatrix[1][1];
				mProjectMatrix[1][2] = -mProjectMatrix[1][2];
				mProjectMatrix[1][3] = -mProjectMatrix[1][3];
			}
		}
	}

	OgreRTTexture::~OgreRTTexture()
	{
	}

	void OgreRTTexture::begin()
	{
		Ogre::RenderTexture* rtt = mTexture->getBuffer()->getRenderTarget();

		if (mViewport == nullptr)
		{
			mViewport = rtt->addViewport(nullptr);
			mViewport->setClearEveryFrame(false);
			mViewport->setOverlaysEnabled(false);
		}

		Ogre::RenderSystem* system = Ogre::Root::getSingleton().getRenderSystem();
		system->_setProjectionMatrix(mProjectMatrix);
		mSaveViewport = system->_getViewport();
		system->_setViewport(mViewport);
		system->clearFrameBuffer(Ogre::FBT_COLOUR, Ogre::ColourValue::ZERO);
	}

	void OgreRTTexture::end()
	{
		Ogre::RenderSystem* system = Ogre::Root::getSingleton().getRenderSystem();
		system->_setViewport(mSaveViewport);
		system->_setProjectionMatrix(Ogre::Matrix4::IDENTITY);
	}

	void OgreRTTexture::doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count)
	{
		OgreRenderManager::getInstance().doRender(_buffer, _texture, _count);
	}

} // namespace MyGUI
