#ifndef SH_OGREMATERIALSERIALIZER_H
#define SH_OGREMATERIALSERIALIZER_H

#include <OgreMaterialSerializer.h>

namespace Ogre
{
	class Pass;
}

namespace sh
{
	/**
	 * @brief This class allows me to let Ogre handle the pass & texture unit properties
	 */
	class OgreMaterialSerializer : public Ogre::MaterialSerializer
	{
	public:
		bool setPassProperty (const std::string& param, std::string value, Ogre::Pass* pass);
		bool setTextureUnitProperty (const std::string& param, std::string value, Ogre::TextureUnitState* t);
		bool setMaterialProperty (const std::string& param, std::string value, Ogre::MaterialPtr m);

	private:
		void reset();
	};

}

#endif
