#include "terrainmaterial.hpp"

#include <OgreTerrain.h>

#include <extern/shiny/Main/Factory.hpp>


namespace MWRender
{

    TerrainMaterial::TerrainMaterial()
    {
        mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("albedo_specular", Ogre::PF_BYTE_RGBA));
        //mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("normal_height", Ogre::PF_BYTE_RGBA));

        mLayerDecl.elements.push_back(
            Ogre::TerrainLayerSamplerElement(0, Ogre::TLSS_ALBEDO, 0, 3));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(0, Ogre::TLSS_SPECULAR, 3, 1));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(1, Ogre::TLSS_NORMAL, 0, 3));
        //mLayerDecl.elements.push_back(
        //	Ogre::TerrainLayerSamplerElement(1, Ogre::TLSS_HEIGHT, 3, 1));


        mProfiles.push_back(OGRE_NEW Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards"));
        setActiveProfile("SM2");
    }

    // -----------------------------------------------------------------------------------------------------------------------

    TerrainMaterial::Profile::Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc)
        : Ogre::TerrainMaterialGenerator::Profile(parent, name, desc)
    {
    }

    TerrainMaterial::Profile::~Profile()
    {
    }


    Ogre::MaterialPtr TerrainMaterial::Profile::generate(const Ogre::Terrain* terrain)
    {
        const Ogre::String& matName = terrain->getMaterialName();

        sh::Factory::getInstance().destroyMaterialInstance (matName);

        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(matName);
        if (!mat.isNull())
            Ogre::MaterialManager::getSingleton().remove(matName);

        mMaterial = sh::Factory::getInstance().createMaterialInstance (matName);

        mMaterial->setProperty ("allow_fixed_function", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(false)));

        createPass();

        return Ogre::MaterialManager::getSingleton().getByName(matName);

        /*
        Ogre::MaterialPtr m = Ogre::MaterialManager::getSingleton().create(matName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        return m;
        */
    }

    int TerrainMaterial::Profile::getLayersPerPass () const
    {
        return 10;
    }

    void TerrainMaterial::Profile::createPass (int index)
    {
        int layerOffset = index * getLayersPerPass();

        sh::MaterialInstancePass* p = mMaterial->createPass ();

        p->setProperty ("vertex_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_vertex")));
        p->setProperty ("fragment_program", sh::makeProperty<sh::StringValue>(new sh::StringValue("terrain_fragment")));
    }

    Ogre::MaterialPtr TerrainMaterial::Profile::generateForCompositeMap(const Ogre::Terrain* terrain)
    {
        throw std::runtime_error ("composite map not supported");
    }

    Ogre::uint8 TerrainMaterial::Profile::getMaxLayers(const Ogre::Terrain* terrain) const
    {
        return 32;
    }

    void TerrainMaterial::Profile::updateParams(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain)
    {
    }

    void TerrainMaterial::Profile::updateParamsForCompositeMap(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain)
    {
    }

    void TerrainMaterial::Profile::requestOptions(Ogre::Terrain* terrain)
    {
        terrain->_setMorphRequired(true);
        terrain->_setNormalMapRequired(false);
        terrain->_setLightMapRequired(false);
        terrain->_setCompositeMapRequired(false);
    }

}
