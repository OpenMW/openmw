#include "OgreMaterialSerializer.hpp"

#include <OgrePass.h>

namespace sh
{
	void OgreMaterialSerializer::reset()
	{
		mScriptContext.section = Ogre::MSS_NONE;
		mScriptContext.material.setNull();
		mScriptContext.technique = 0;
		mScriptContext.pass = 0;
		mScriptContext.textureUnit = 0;
		mScriptContext.program.setNull();
		mScriptContext.lineNo = 0;
		mScriptContext.filename.clear();
		mScriptContext.techLev = -1;
		mScriptContext.passLev = -1;
		mScriptContext.stateLev = -1;
	}

	bool OgreMaterialSerializer::setPassProperty (const std::string& param, std::string value, Ogre::Pass* pass)
	{
		// workaround https://ogre3d.atlassian.net/browse/OGRE-158
		if (param == "transparent_sorting" && value == "force")
		{
			pass->setTransparentSortingForced(true);
			return true;
		}

		reset();

		mScriptContext.section = Ogre::MSS_PASS;
		mScriptContext.pass = pass;

		if (mPassAttribParsers.find (param) == mPassAttribParsers.end())
			return false;
		else
		{
			mPassAttribParsers.find(param)->second(value, mScriptContext);
			return true;
		}
	}

	bool OgreMaterialSerializer::setTextureUnitProperty (const std::string& param, std::string value, Ogre::TextureUnitState* t)
	{
		reset();

		mScriptContext.section = Ogre::MSS_TEXTUREUNIT;
		mScriptContext.textureUnit = t;

		if (mTextureUnitAttribParsers.find (param) == mTextureUnitAttribParsers.end())
			return false;
		else
		{
			mTextureUnitAttribParsers.find(param)->second(value, mScriptContext);
			return true;
		}
	}

	bool OgreMaterialSerializer::setMaterialProperty (const std::string& param, std::string value, Ogre::MaterialPtr m)
	{
		reset();

		mScriptContext.section = Ogre::MSS_MATERIAL;
		mScriptContext.material = m;

		if (mMaterialAttribParsers.find (param) == mMaterialAttribParsers.end())
			return false;
		else
		{
			mMaterialAttribParsers.find(param)->second(value, mScriptContext);
			return true;
		}
	}
}
