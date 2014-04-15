#include "OgreTextureUnitState.hpp"

#include <iomanip>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "OgrePass.hpp"
#include "OgrePlatform.hpp"
#include "OgreMaterialSerializer.hpp"

namespace sh
{
	OgreTextureUnitState::OgreTextureUnitState (OgrePass* parent, const std::string& name)
		: TextureUnitState()
	{
		mTextureUnitState = parent->getOgrePass()->createTextureUnitState("");
		mTextureUnitState->setName(name);
	}

	bool OgreTextureUnitState::setPropertyOverride (const std::string &name, PropertyValuePtr& value, PropertySetGet* context)
	{
		OgreMaterialSerializer& s = OgrePlatform::getSerializer();

		if (name == "texture_alias")
		{
			// texture alias in this library refers to something else than in ogre
			// delegate up
			return TextureUnitState::setPropertyOverride (name, value, context);
		}
		else if (name == "direct_texture")
		{
			setTextureName (retrieveValue<StringValue>(value, context).get());
			return true;
		}
        else if (name == "anim_texture2")
        {
            std::string val = retrieveValue<StringValue>(value, context).get();
            std::vector<std::string> tokens;
            boost::split(tokens, val, boost::is_any_of(" "));
            assert(tokens.size() == 3);
            std::string texture = tokens[0];
            int frames = boost::lexical_cast<int>(tokens[1]);
            float duration = boost::lexical_cast<float>(tokens[2]);

            std::vector<Ogre::String> frameTextures;
            for (int i=0; i<frames; ++i)
            {
                std::stringstream stream;
                stream << std::setw(2);
                stream << std::setfill('0');
                stream << i;
                stream << '.';
                std::string tex = texture;
                boost::replace_last(tex, ".", stream.str());
                frameTextures.push_back(tex);
            }

            mTextureUnitState->setAnimatedTextureName(&frameTextures[0], frames, duration);
            return true;
        }
		else if (name == "create_in_ffp")
			return true; // handled elsewhere

		return s.setTextureUnitProperty (name, retrieveValue<StringValue>(value, context).get(), mTextureUnitState);
	}

	void OgreTextureUnitState::setTextureName (const std::string& textureName)
	{
		mTextureUnitState->setTextureName(textureName);
	}
}
