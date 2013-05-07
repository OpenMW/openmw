#ifndef SH_OGRETEXTUREUNITSTATE_H
#define SH_OGRETEXTUREUNITSTATE_H

#include <OgreTextureUnitState.h>

#include "../../Main/Platform.hpp"

namespace sh
{
	class OgrePass;

	class OgreTextureUnitState : public TextureUnitState
	{
	public:
		OgreTextureUnitState (OgrePass* parent, const std::string& name);

		virtual void setTextureName (const std::string& textureName);

	private:
		Ogre::TextureUnitState* mTextureUnitState;

	protected:
		virtual bool setPropertyOverride (const std::string &name, PropertyValuePtr& value, PropertySetGet* context);
	};
}

#endif
