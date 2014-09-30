#ifndef SH_OGREMATERIAL_H
#define SH_OGREMATERIAL_H

#include <string>

#include <OgreMaterial.h>

#include "../../Main/Platform.hpp"

namespace sh
{
	class OgreMaterial : public Material
	{
	public:
		OgreMaterial (const std::string& name, const std::string& resourceGroup);
		virtual ~OgreMaterial();

		virtual boost::shared_ptr<Pass> createPass (const std::string& configuration, unsigned short lodIndex);
		virtual bool createConfiguration (const std::string& name, unsigned short lodIndex);

		virtual bool isUnreferenced();
		virtual void unreferenceTextures();
		virtual void ensureLoaded();

		virtual void removeAll ();

		Ogre::MaterialPtr getOgreMaterial();

		virtual void setLodLevels (const std::string& lodLevels);

		Ogre::Technique* getOgreTechniqueForConfiguration (const std::string& configurationName, unsigned short lodIndex = 0);

		virtual void setShadowCasterMaterial (const std::string& name);

	private:
		Ogre::MaterialPtr mMaterial;
		std::string mName;

		std::string mShadowCasterMaterial;
	};
}

#endif
