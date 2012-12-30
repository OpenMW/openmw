#include "OgreMaterial.hpp"

#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <stdexcept>

#include "OgrePass.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgrePlatform.hpp"

namespace sh
{
	static const std::string sDefaultTechniqueName = "SH_DefaultTechnique";

	OgreMaterial::OgreMaterial (const std::string& name, const std::string& resourceGroup)
		: Material()
	{
		assert (Ogre::MaterialManager::getSingleton().getByName(name).isNull() && "Material already exists");
		mMaterial = Ogre::MaterialManager::getSingleton().create (name, resourceGroup);
		mMaterial->removeAllTechniques();
		mMaterial->createTechnique()->setSchemeName (sDefaultTechniqueName);
		mMaterial->compile();
	}

	OgreMaterial::~OgreMaterial()
	{
		Ogre::MaterialManager::getSingleton().remove(mMaterial->getName());
	}

	boost::shared_ptr<Pass> OgreMaterial::createPass (const std::string& configuration, unsigned short lodIndex)
	{
		return boost::shared_ptr<Pass> (new OgrePass (this, configuration, lodIndex));
	}

	void OgreMaterial::removeAll ()
	{
		mMaterial->removeAllTechniques();
		mMaterial->createTechnique()->setSchemeName (sDefaultTechniqueName);
		mMaterial->compile();
	}

	void OgreMaterial::setLodLevels (const std::string& lodLevels)
	{
		OgreMaterialSerializer& s = OgrePlatform::getSerializer();

		s.setMaterialProperty ("lod_values", lodLevels, mMaterial);
	}

	bool OgreMaterial::createConfiguration (const std::string& name, unsigned short lodIndex)
	{
		for (int i=0; i<mMaterial->getNumTechniques(); ++i)
		{
			if (mMaterial->getTechnique(i)->getSchemeName() == name && mMaterial->getTechnique(i)->getLodIndex() == lodIndex)
				return false;
		}

		Ogre::Technique* t = mMaterial->createTechnique();
		t->setSchemeName (name);
		t->setLodIndex (lodIndex);
		if (mShadowCasterMaterial != "")
			t->setShadowCasterMaterial(mShadowCasterMaterial);

		mMaterial->compile();

		return true;
	}

	Ogre::MaterialPtr OgreMaterial::getOgreMaterial ()
	{
		return mMaterial;
	}

	Ogre::Technique* OgreMaterial::getOgreTechniqueForConfiguration (const std::string& configurationName, unsigned short lodIndex)
	{
		for (int i=0; i<mMaterial->getNumTechniques(); ++i)
		{
			if (mMaterial->getTechnique(i)->getSchemeName() == configurationName && mMaterial->getTechnique(i)->getLodIndex() == lodIndex)
			{
				return mMaterial->getTechnique(i);
			}
		}

		// Prepare and throw error message
		std::stringstream message;
		message << "Could not find configurationName '" << configurationName
				<< "' and lodIndex " << lodIndex;

		throw std::runtime_error(message.str());
	}

	void OgreMaterial::setShadowCasterMaterial (const std::string& name)
	{
		mShadowCasterMaterial = name;
		for (int i=0; i<mMaterial->getNumTechniques(); ++i)
		{
			mMaterial->getTechnique(i)->setShadowCasterMaterial(mShadowCasterMaterial);
		}
	}
}
