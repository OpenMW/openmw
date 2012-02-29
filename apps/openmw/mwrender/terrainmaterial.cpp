/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2011 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "terrainmaterial.hpp"
#include "OgreTerrain.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreTextureUnitState.h"
#include "OgreGpuProgramManager.h"
#include "OgreHighLevelGpuProgramManager.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreShadowCameraSetupPSSM.h"

namespace Ogre
{
	//---------------------------------------------------------------------
	TerrainMaterialGeneratorB::TerrainMaterialGeneratorB()
	{
		// define the layers
		// We expect terrain textures to have no alpha, so we use the alpha channel
		// in the albedo texture to store specular reflection
		// similarly we double-up the normal and height (for parallax)
		mLayerDecl.samplers.push_back(TerrainLayerSampler("albedo_specular", PF_BYTE_RGBA));
		//mLayerDecl.samplers.push_back(TerrainLayerSampler("normal_height", PF_BYTE_RGBA));
		
		mLayerDecl.elements.push_back(
			TerrainLayerSamplerElement(0, TLSS_ALBEDO, 0, 3));
		//mLayerDecl.elements.push_back(
		//	TerrainLayerSamplerElement(0, TLSS_SPECULAR, 3, 1));
		//mLayerDecl.elements.push_back(
		//	TerrainLayerSamplerElement(1, TLSS_NORMAL, 0, 3));
		//mLayerDecl.elements.push_back(
		//	TerrainLayerSamplerElement(1, TLSS_HEIGHT, 3, 1));


		mProfiles.push_back(OGRE_NEW SM2Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards"));
		// TODO - check hardware capabilities & use fallbacks if required (more profiles needed)
		setActiveProfile("SM2");

	}
	//---------------------------------------------------------------------
	TerrainMaterialGeneratorB::~TerrainMaterialGeneratorB()
	{

	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	TerrainMaterialGeneratorB::SM2Profile::SM2Profile(TerrainMaterialGenerator* parent, const String& name, const String& desc)
		: Profile(parent, name, desc)
		, mShaderGen(0)
		, mLayerNormalMappingEnabled(true)
		, mLayerParallaxMappingEnabled(true)
		, mLayerSpecularMappingEnabled(true)
		, mGlobalColourMapEnabled(true)
		, mLightmapEnabled(true)
		, mCompositeMapEnabled(true)
		, mReceiveDynamicShadows(true)
		, mPSSM(0)
		, mDepthShadows(false)
		, mLowLodShadows(false)
	{

	}
	//---------------------------------------------------------------------
	TerrainMaterialGeneratorB::SM2Profile::~SM2Profile()
	{
		OGRE_DELETE mShaderGen;
	}	
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::requestOptions(Terrain* terrain)
	{
		terrain->_setMorphRequired(true);
		terrain->_setNormalMapRequired(true);
		terrain->_setLightMapRequired(mLightmapEnabled, true);
		terrain->_setCompositeMapRequired(mCompositeMapEnabled);
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::setLayerNormalMappingEnabled(bool enabled)
	{
		if (enabled != mLayerNormalMappingEnabled)
		{
			mLayerNormalMappingEnabled = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::setLayerParallaxMappingEnabled(bool enabled)
	{
		if (enabled != mLayerParallaxMappingEnabled)
		{
			mLayerParallaxMappingEnabled = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::setLayerSpecularMappingEnabled(bool enabled)
	{
		if (enabled != mLayerSpecularMappingEnabled)
		{
			mLayerSpecularMappingEnabled = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void  TerrainMaterialGeneratorB::SM2Profile::setGlobalColourMapEnabled(bool enabled)
	{
		if (enabled != mGlobalColourMapEnabled)
		{
			mGlobalColourMapEnabled = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void  TerrainMaterialGeneratorB::SM2Profile::setLightmapEnabled(bool enabled)
	{
		if (enabled != mLightmapEnabled)
		{
			mLightmapEnabled = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void  TerrainMaterialGeneratorB::SM2Profile::setCompositeMapEnabled(bool enabled)
	{
		if (enabled != mCompositeMapEnabled)
		{
			mCompositeMapEnabled = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void  TerrainMaterialGeneratorB::SM2Profile::setReceiveDynamicShadowsEnabled(bool enabled)
	{
		if (enabled != mReceiveDynamicShadows)
		{
			mReceiveDynamicShadows = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::setReceiveDynamicShadowsPSSM(PSSMShadowCameraSetup* pssmSettings)
	{
		if (pssmSettings != mPSSM)
		{
			mPSSM = pssmSettings;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::setReceiveDynamicShadowsDepth(bool enabled)
	{
		if (enabled != mDepthShadows)
		{
			mDepthShadows = enabled;
			mParent->_markChanged();
		}

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::setReceiveDynamicShadowsLowLod(bool enabled)
	{
		if (enabled != mLowLodShadows)
		{
			mLowLodShadows = enabled;
			mParent->_markChanged();
		}
	}
	//---------------------------------------------------------------------
	uint8 TerrainMaterialGeneratorB::SM2Profile::getMaxLayers(const Terrain* terrain) const
	{
		// count the texture units free
		uint8 freeTextureUnits = 16;
		// lightmap
                if (mLightmapEnabled)
                        --freeTextureUnits;
		// normalmap
		--freeTextureUnits;
		// colourmap
		if (terrain->getGlobalColourMapEnabled())
			--freeTextureUnits;
		if (isShadowingEnabled(HIGH_LOD, terrain))
		{
			uint numShadowTextures = 1;
			if (getReceiveDynamicShadowsPSSM())
			{
				numShadowTextures = getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			freeTextureUnits -= numShadowTextures;
		}

		// each layer needs 2.25 units (1xdiffusespec, (1xnormalheight), 0.25xblend)
		return static_cast<uint8>(freeTextureUnits / (1.25f + (mLayerNormalMappingEnabled||mLayerParallaxMappingEnabled)));
		

	}
	//---------------------------------------------------------------------
	MaterialPtr TerrainMaterialGeneratorB::SM2Profile::generate(const Terrain* terrain)
	{
		// re-use old material if exists
		MaterialPtr mat = terrain->_getMaterial();
		if (mat.isNull())
		{
			MaterialManager& matMgr = MaterialManager::getSingleton();

			// it's important that the names are deterministic for a given terrain, so
			// use the terrain pointer as an ID
			const String& matName = terrain->getMaterialName();
			mat = matMgr.getByName(matName);
			if (mat.isNull())
			{
				mat = matMgr.create(matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
		}
		// clear everything
		mat->removeAllTechniques();
		
		// Automatically disable normal & parallax mapping if card cannot handle it
		// We do this rather than having a specific technique for it since it's simpler
		GpuProgramManager& gmgr = GpuProgramManager::getSingleton();
		if (!gmgr.isSyntaxSupported("ps_3_0") && !gmgr.isSyntaxSupported("ps_2_x")
			&& !gmgr.isSyntaxSupported("fp40") && !gmgr.isSyntaxSupported("arbfp1"))
		{
			setLayerNormalMappingEnabled(false);
			setLayerParallaxMappingEnabled(false);
		}

		addTechnique(mat, terrain, HIGH_LOD);

		// LOD
		if(mCompositeMapEnabled)
		{
			addTechnique(mat, terrain, LOW_LOD);
			Material::LodValueList lodValues;
			lodValues.push_back(TerrainGlobalOptions::getSingleton().getCompositeMapDistance());
			mat->setLodLevels(lodValues);
			Technique* lowLodTechnique = mat->getTechnique(1);
			lowLodTechnique->setLodIndex(1);
		}

		updateParams(mat, terrain);

		return mat;

	}
	//---------------------------------------------------------------------
	MaterialPtr TerrainMaterialGeneratorB::SM2Profile::generateForCompositeMap(const Terrain* terrain)
	{
		// re-use old material if exists
		MaterialPtr mat = terrain->_getCompositeMapMaterial();
		if (mat.isNull())
		{
			MaterialManager& matMgr = MaterialManager::getSingleton();

			// it's important that the names are deterministic for a given terrain, so
			// use the terrain pointer as an ID
			const String& matName = terrain->getMaterialName() + "/comp";
			mat = matMgr.getByName(matName);
			if (mat.isNull())
			{
				mat = matMgr.create(matName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
		}
		// clear everything
		mat->removeAllTechniques();

		addTechnique(mat, terrain, RENDER_COMPOSITE_MAP);

		updateParamsForCompositeMap(mat, terrain);

		return mat;

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::addTechnique(
		const MaterialPtr& mat, const Terrain* terrain, TechniqueType tt)
	{

		Technique* tech = mat->createTechnique();

		// Only supporting one pass
		Pass* pass = tech->createPass();

		GpuProgramManager& gmgr = GpuProgramManager::getSingleton();
		HighLevelGpuProgramManager& hmgr = HighLevelGpuProgramManager::getSingleton();
		if (!mShaderGen)
		{
			bool check2x = mLayerNormalMappingEnabled || mLayerParallaxMappingEnabled;
			if (hmgr.isLanguageSupported("cg"))
				mShaderGen = OGRE_NEW ShaderHelperCg();
			else if (hmgr.isLanguageSupported("hlsl") &&
				((check2x && gmgr.isSyntaxSupported("ps_2_x")) ||
				(!check2x && gmgr.isSyntaxSupported("ps_2_0"))))
				mShaderGen = OGRE_NEW ShaderHelperHLSL();
			else if (hmgr.isLanguageSupported("glsl"))
				mShaderGen = OGRE_NEW ShaderHelperGLSL();
			else
			{
				// todo
			}

			// check SM3 features
			mSM3Available = GpuProgramManager::getSingleton().isSyntaxSupported("ps_3_0");

		}
		HighLevelGpuProgramPtr vprog = mShaderGen->generateVertexProgram(this, terrain, tt);
		HighLevelGpuProgramPtr fprog = mShaderGen->generateFragmentProgram(this, terrain, tt);

		pass->setVertexProgram(vprog->getName());
		pass->setFragmentProgram(fprog->getName());

		if (tt == HIGH_LOD || tt == RENDER_COMPOSITE_MAP)
		{
			// global normal map
			TextureUnitState* tu = pass->createTextureUnitState();
			tu->setTextureName(terrain->getTerrainNormalMap()->getName());
			tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

			// global colour map
			if (terrain->getGlobalColourMapEnabled() && isGlobalColourMapEnabled())
			{
				tu = pass->createTextureUnitState(terrain->getGlobalColourMap()->getName());
				tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// light map
			if (isLightmapEnabled())
			{
				tu = pass->createTextureUnitState(terrain->getLightmap()->getName());
				tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// blend maps
			uint maxLayers = getMaxLayers(terrain);
			uint numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
			uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));
			for (uint i = 0; i < numBlendTextures; ++i)
			{
				tu = pass->createTextureUnitState(terrain->getBlendTextureName(i));
				tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// layer textures
			for (uint i = 0; i < numLayers; ++i)
			{
				// diffuse / specular
				tu = pass->createTextureUnitState(terrain->getLayerTextureName(i, 0));
                                                                
				// normal / height
                                if (mLayerNormalMappingEnabled || mLayerParallaxMappingEnabled)
                                        tu = pass->createTextureUnitState(terrain->getLayerTextureName(i, 1));
			}

		}
		else
		{
			// LOW_LOD textures
			// composite map
			TextureUnitState* tu = pass->createTextureUnitState();
			tu->setTextureName(terrain->getCompositeMap()->getName());
			tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

			// That's it!

		}

		// Add shadow textures (always at the end)
		if (isShadowingEnabled(tt, terrain))
		{
			uint numTextures = 1;
			if (getReceiveDynamicShadowsPSSM())
			{
				numTextures = getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			for (uint i = 0; i < numTextures; ++i)
			{
				TextureUnitState* tu = pass->createTextureUnitState();
				tu->setContentType(TextureUnitState::CONTENT_SHADOW);
				tu->setTextureAddressingMode(TextureUnitState::TAM_BORDER);
				tu->setTextureBorderColour(ColourValue::White);
			}
		}

	}
	//---------------------------------------------------------------------
	bool TerrainMaterialGeneratorB::SM2Profile::isShadowingEnabled(TechniqueType tt, const Terrain* terrain) const
	{
		return getReceiveDynamicShadowsEnabled() && tt != RENDER_COMPOSITE_MAP && 
			(tt != LOW_LOD || mLowLodShadows) &&
			terrain->getSceneManager()->isShadowTechniqueTextureBased();

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::updateParams(const MaterialPtr& mat, const Terrain* terrain)
	{
		mShaderGen->updateParams(this, mat, terrain, false);

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::updateParamsForCompositeMap(const MaterialPtr& mat, const Terrain* terrain)
	{
		mShaderGen->updateParams(this, mat, terrain, true);
	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr 
		TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::generateVertexProgram(
			const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramPtr ret = createVertexProgram(prof, terrain, tt);

		StringUtil::StrStreamType sourceStr;
		generateVertexProgramSource(prof, terrain, tt, sourceStr);
		ret->setSource(sourceStr.str());
		ret->load();
		defaultVpParams(prof, terrain, tt, ret);
#if OGRE_DEBUG_MODE
		LogManager::getSingleton().stream(LML_TRIVIAL) << "*** Terrain Vertex Program: " 
			<< ret->getName() << " ***\n" << ret->getSource() << "\n***   ***";
#endif

		return ret;

	}
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr 
	TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::generateFragmentProgram(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramPtr ret = createFragmentProgram(prof, terrain, tt);

		StringUtil::StrStreamType sourceStr;
		generateFragmentProgramSource(prof, terrain, tt, sourceStr);
		ret->setSource(sourceStr.str());
		ret->load();
		defaultFpParams(prof, terrain, tt, ret);

#if OGRE_DEBUG_MODE
		LogManager::getSingleton().stream(LML_TRIVIAL) << "*** Terrain Fragment Program: " 
			<< ret->getName() << " ***\n" << ret->getSource() << "\n***   ***";
#endif

		return ret;
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::generateVertexProgramSource(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		generateVpHeader(prof, terrain, tt, outStream);

		if (tt != LOW_LOD)
		{
			uint maxLayers = prof->getMaxLayers(terrain);
			uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));

			for (uint i = 0; i < numLayers; ++i)
				generateVpLayer(prof, terrain, tt, i, outStream);
		}

		generateVpFooter(prof, terrain, tt, outStream);

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::generateFragmentProgramSource(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		generateFpHeader(prof, terrain, tt, outStream);

		if (tt != LOW_LOD)
		{
			uint maxLayers = prof->getMaxLayers(terrain);
			uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));

			for (uint i = 0; i < numLayers; ++i)
				generateFpLayer(prof, terrain, tt, i, outStream);
		}

		generateFpFooter(prof, terrain, tt, outStream);
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::defaultVpParams(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const HighLevelGpuProgramPtr& prog)
	{
		GpuProgramParametersSharedPtr params = prog->getDefaultParameters();
		params->setIgnoreMissingParams(true);
		params->setNamedAutoConstant("worldMatrix", GpuProgramParameters::ACT_WORLD_MATRIX);
		params->setNamedAutoConstant("viewProjMatrix", GpuProgramParameters::ACT_VIEWPROJ_MATRIX);
		params->setNamedAutoConstant("lodMorph", GpuProgramParameters::ACT_CUSTOM, 
			Terrain::LOD_MORPH_CUSTOM_PARAM);
		params->setNamedAutoConstant("fogParams", GpuProgramParameters::ACT_FOG_PARAMS);

		if (prof->isShadowingEnabled(tt, terrain))
		{
			uint numTextures = 1;
			if (prof->getReceiveDynamicShadowsPSSM())
			{
				numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			for (uint i = 0; i < numTextures; ++i)
			{
				params->setNamedAutoConstant("texViewProjMatrix" + StringConverter::toString(i), 
					GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX, i);
				if (prof->getReceiveDynamicShadowsDepth())
				{
					params->setNamedAutoConstant("depthRange" + StringConverter::toString(i), 
						GpuProgramParameters::ACT_SHADOW_SCENE_DEPTH_RANGE, i);
				}
			}
		}


	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::defaultFpParams(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const HighLevelGpuProgramPtr& prog)
	{
		GpuProgramParametersSharedPtr params = prog->getDefaultParameters();
		params->setIgnoreMissingParams(true);

		params->setNamedAutoConstant("ambient", GpuProgramParameters::ACT_AMBIENT_LIGHT_COLOUR);
		params->setNamedAutoConstant("lightPosObjSpace", GpuProgramParameters::ACT_LIGHT_POSITION_OBJECT_SPACE, 0);
		params->setNamedAutoConstant("lightDiffuseColour", GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR, 0);
		//params->setNamedAutoConstant("lightSpecularColour", GpuProgramParameters::ACT_LIGHT_SPECULAR_COLOUR, 0);
		params->setNamedAutoConstant("eyePosObjSpace", GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);
		params->setNamedAutoConstant("fogColour", GpuProgramParameters::ACT_FOG_COLOUR);

		if (prof->isShadowingEnabled(tt, terrain))
		{
			uint numTextures = 1;
			if (prof->getReceiveDynamicShadowsPSSM())
			{
				PSSMShadowCameraSetup* pssm = prof->getReceiveDynamicShadowsPSSM();
				numTextures = pssm->getSplitCount();
				Vector4 splitPoints;
				const PSSMShadowCameraSetup::SplitPointList& splitPointList = pssm->getSplitPoints();
				// Populate from split point 1, not 0, since split 0 isn't useful (usually 0)
				for (uint i = 1; i < numTextures; ++i)
				{
					splitPoints[i-1] = splitPointList[i];
				}
				params->setNamedConstant("pssmSplitPoints", splitPoints);
			}

			if (prof->getReceiveDynamicShadowsDepth())
			{
				size_t samplerOffset = (tt == HIGH_LOD) ? mShadowSamplerStartHi : mShadowSamplerStartLo;
				for (uint i = 0; i < numTextures; ++i)
				{
					params->setNamedAutoConstant("inverseShadowmapSize" + StringConverter::toString(i), 
						GpuProgramParameters::ACT_INVERSE_TEXTURE_SIZE, i + samplerOffset);
				}
			}
		}


	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::updateParams(
		const SM2Profile* prof, const MaterialPtr& mat, const Terrain* terrain, bool compositeMap)
	{
		Pass* p = mat->getTechnique(0)->getPass(0);
		if (compositeMap)
		{
			updateVpParams(prof, terrain, RENDER_COMPOSITE_MAP, p->getVertexProgramParameters());
			updateFpParams(prof, terrain, RENDER_COMPOSITE_MAP, p->getFragmentProgramParameters());
		}
		else
		{
			// high lod
			updateVpParams(prof, terrain, HIGH_LOD, p->getVertexProgramParameters());
			updateFpParams(prof, terrain, HIGH_LOD, p->getFragmentProgramParameters());

			if(prof->isCompositeMapEnabled())
			{
				// low lod
				p = mat->getTechnique(1)->getPass(0);
				updateVpParams(prof, terrain, LOW_LOD, p->getVertexProgramParameters());
				updateFpParams(prof, terrain, LOW_LOD, p->getFragmentProgramParameters());
			}
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::updateVpParams(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const GpuProgramParametersSharedPtr& params)
	{
		params->setIgnoreMissingParams(true);
		uint maxLayers = prof->getMaxLayers(terrain);
		uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));
		uint numUVMul = numLayers / 4;
		if (numLayers % 4)
			++numUVMul;
		for (uint i = 0; i < numUVMul; ++i)
		{
			Vector4 uvMul(
				terrain->getLayerUVMultiplier(i * 4), 
				terrain->getLayerUVMultiplier(i * 4 + 1), 
				terrain->getLayerUVMultiplier(i * 4 + 2), 
				terrain->getLayerUVMultiplier(i * 4 + 3) 
				);
			params->setNamedConstant("uvMul" + StringConverter::toString(i), uvMul);
		}

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::updateFpParams(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const GpuProgramParametersSharedPtr& params)
	{
		params->setIgnoreMissingParams(true);
		// TODO - parameterise this?
		Vector4 scaleBiasSpecular(0.03, -0.04, 32, 1);
		params->setNamedConstant("scaleBiasSpecular", scaleBiasSpecular);

	}
	//---------------------------------------------------------------------
	String TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::getChannel(uint idx)
	{
		uint rem = idx % 4;
		switch(rem)
		{
		case 0:
		default:
			return "r";
		case 1:
			return "g";
		case 2:
			return "b";
		case 3:
			return "a";
		};
	}
	//---------------------------------------------------------------------
	String TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::getVertexProgramName(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		String progName = terrain->getMaterialName() + "/sm2/vp";

		switch(tt)
		{
		case HIGH_LOD:
			progName += "/hlod";
			break;
		case LOW_LOD:
			progName += "/llod";
			break;
		case RENDER_COMPOSITE_MAP:
			progName += "/comp";
			break;
		}

		return progName;

	}
	//---------------------------------------------------------------------
	String TerrainMaterialGeneratorB::SM2Profile::ShaderHelper::getFragmentProgramName(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{

		String progName = terrain->getMaterialName() + "/sm2/fp";

		switch(tt)
		{
		case HIGH_LOD:
			progName += "/hlod";
			break;
		case LOW_LOD:
			progName += "/llod";
			break;
		case RENDER_COMPOSITE_MAP:
			progName += "/comp";
			break;
		}

		return progName;
	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
	TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::createVertexProgram(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
		String progName = getVertexProgramName(prof, terrain, tt);
		HighLevelGpuProgramPtr ret = mgr.getByName(progName);
		if (ret.isNull())
		{
			ret = mgr.createProgram(progName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
				"cg", GPT_VERTEX_PROGRAM);
		}
		else
		{
			ret->unload();
		}

		ret->setParameter("profiles", "vs_3_0 vs_2_0 arbvp1");
		ret->setParameter("entry_point", "main_vp");

		return ret;

	}
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
		TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::createFragmentProgram(
			const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
		String progName = getFragmentProgramName(prof, terrain, tt);

		HighLevelGpuProgramPtr ret = mgr.getByName(progName);
		if (ret.isNull())
		{
			ret = mgr.createProgram(progName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
				"cg", GPT_FRAGMENT_PROGRAM);
		}
		else
		{
			ret->unload();
		}
		
		if(prof->isLayerNormalMappingEnabled() || prof->isLayerParallaxMappingEnabled())
			ret->setParameter("profiles", "ps_3_0 ps_2_x fp40 arbfp1");
		//else
			//ret->setParameter("profiles", "ps_3_0 ps_2_0 fp30 arbfp1");
		else // fp30 doesn't work (black terrain)
			ret->setParameter("profiles", "ps_3_0 ps_2_x fp40 arbfp1");
		ret->setParameter("entry_point", "main_fp");

		return ret;

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateVpHeader(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		outStream << 
			"void main_vp(\n"
			"float4 pos : POSITION,\n"
			"float2 uv  : TEXCOORD0,\n";
		if (tt != RENDER_COMPOSITE_MAP)
			outStream << "float2 delta  : TEXCOORD1,\n"; // lodDelta, lodThreshold

		outStream << 
			"uniform float4x4 worldMatrix,\n"
			"uniform float4x4 viewProjMatrix,\n"
			"uniform float2   lodMorph,\n"; // morph amount, morph LOD target

		// uv multipliers
		uint maxLayers = prof->getMaxLayers(terrain);
		uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));
		uint numUVMultipliers = (numLayers / 4);
		if (numLayers % 4)
			++numUVMultipliers;
		for (uint i = 0; i < numUVMultipliers; ++i)
			outStream << "uniform float4 uvMul" << i << ", \n";

		outStream <<
			"out float4 oPos : POSITION,\n"
			"out float4 oPosObj : TEXCOORD0 \n";

		uint texCoordSet = 1;
		outStream <<
			", out float4 oUVMisc : TEXCOORD" << texCoordSet++ <<" // xy = uv, z = camDepth\n";

		// layer UV's premultiplied, packed as xy/zw
		uint numUVSets = numLayers / 2;
		if (numLayers % 2)
			++numUVSets;
		if (tt != LOW_LOD)
		{
			for (uint i = 0; i < numUVSets; ++i)
			{
				outStream <<
					", out float4 oUV" << i << " : TEXCOORD" << texCoordSet++ << "\n";
			}
		}

		if (prof->getParent()->getDebugLevel() && tt != RENDER_COMPOSITE_MAP)
		{
			outStream << ", out float2 lodInfo : TEXCOORD" << texCoordSet++ << "\n";
		}

		bool fog = terrain->getSceneManager()->getFogMode() != FOG_NONE && tt != RENDER_COMPOSITE_MAP;
		if (fog)
		{
			outStream <<
				", uniform float4 fogParams\n"
				", out float fogVal : COLOR\n";
		}

		if (prof->isShadowingEnabled(tt, terrain))
		{
			texCoordSet = generateVpDynamicShadowsParams(texCoordSet, prof, terrain, tt, outStream);
		}

		// check we haven't exceeded texture coordinates
		if (texCoordSet > 8)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
				"Requested options require too many texture coordinate sets! Try reducing the number of layers.",
				__FUNCTION__);
		}

		outStream <<
			")\n"
			"{\n"
			"	float4 worldPos = mul(worldMatrix, pos);\n"
			"	oPosObj = pos;\n";

		if (tt != RENDER_COMPOSITE_MAP)
		{
			// determine whether to apply the LOD morph to this vertex
			// we store the deltas against all vertices so we only want to apply 
			// the morph to the ones which would disappear. The target LOD which is
			// being morphed to is stored in lodMorph.y, and the LOD at which 
			// the vertex should be morphed is stored in uv.w. If we subtract
			// the former from the latter, and arrange to only morph if the
			// result is negative (it will only be -1 in fact, since after that
			// the vertex will never be indexed), we will achieve our aim.
			// sign(vertexLOD - targetLOD) == -1 is to morph
			outStream << 
				"	float toMorph = -min(0, sign(delta.y - lodMorph.y));\n";
			// this will either be 1 (morph) or 0 (don't morph)
			if (prof->getParent()->getDebugLevel())
			{
				// x == LOD level (-1 since value is target level, we want to display actual)
				outStream << "lodInfo.x = (lodMorph.y - 1) / " << terrain->getNumLodLevels() << ";\n";
				// y == LOD morph
				outStream << "lodInfo.y = toMorph * lodMorph.x;\n";
			}

			// morph
			switch (terrain->getAlignment())
			{
			case Terrain::ALIGN_X_Y:
				outStream << "	worldPos.z += delta.x * toMorph * lodMorph.x;\n";
				break;
			case Terrain::ALIGN_X_Z:
				outStream << "	worldPos.y += delta.x * toMorph * lodMorph.x;\n";
				break;
			case Terrain::ALIGN_Y_Z:
				outStream << "	worldPos.x += delta.x * toMorph * lodMorph.x;\n";
				break;
			};
		}


		// generate UVs
		if (tt != LOW_LOD)
		{
			for (uint i = 0; i < numUVSets; ++i)
			{
				uint layer  =  i * 2;
				uint uvMulIdx = layer / 4;

				outStream <<
					"	oUV" << i << ".xy = " << " uv.xy * uvMul" << uvMulIdx << "." << getChannel(layer) << ";\n";
				outStream <<
					"	oUV" << i << ".zw = " << " uv.xy * uvMul" << uvMulIdx << "." << getChannel(layer+1) << ";\n";
				
			}

		}	


	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateFpHeader(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{

		// Main header
		outStream << 
			// helpers
			"float4 expand(float4 v)\n"
			"{ \n"
			"	return v * 2 - 1;\n"
			"}\n\n\n";

		if (prof->isShadowingEnabled(tt, terrain))
			generateFpDynamicShadowsHelpers(prof, terrain, tt, outStream);


		outStream << 
			"float4 main_fp(\n"
			"float4 position : TEXCOORD0,\n";

		uint texCoordSet = 1;
		outStream <<
			"float4 uvMisc : TEXCOORD" << texCoordSet++ << ",\n";

		// UV's premultiplied, packed as xy/zw
		uint maxLayers = prof->getMaxLayers(terrain);
		uint numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
		uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));
		uint numUVSets = numLayers / 2;
		if (numLayers % 2)
			++numUVSets;
		if (tt != LOW_LOD)
		{
			for (uint i = 0; i < numUVSets; ++i)
			{
				outStream <<
					"float4 layerUV" << i << " : TEXCOORD" << texCoordSet++ << ", \n";
			}

		}
		if (prof->getParent()->getDebugLevel() && tt != RENDER_COMPOSITE_MAP)
		{
			outStream << "float2 lodInfo : TEXCOORD" << texCoordSet++ << ", \n";
		}

		bool fog = terrain->getSceneManager()->getFogMode() != FOG_NONE && tt != RENDER_COMPOSITE_MAP;
		if (fog)
		{
			outStream <<
				"uniform float3 fogColour, \n"
				"float fogVal : COLOR,\n";
		}

		uint currentSamplerIdx = 0;

		outStream <<
			// Only 1 light supported in this version
			// deferred shading profile / generator later, ok? :)
			"uniform float3 ambient,\n"
			"uniform float4 lightPosObjSpace,\n"
			"uniform float3 lightDiffuseColour,\n"
			//"uniform float3 lightSpecularColour,\n"
			"uniform float3 eyePosObjSpace,\n"
			// pack scale, bias and specular
			"uniform float4 scaleBiasSpecular,\n";

		if (tt == LOW_LOD)
		{
			// single composite map covers all the others below
			outStream << 
				"uniform sampler2D compositeMap : register(s" << currentSamplerIdx++ << ")\n";
		}
		else
		{
			outStream << 
				"uniform sampler2D globalNormal : register(s" << currentSamplerIdx++ << ")\n";


			if (terrain->getGlobalColourMapEnabled() && prof->isGlobalColourMapEnabled())
			{
				outStream << ", uniform sampler2D globalColourMap : register(s" 
					<< currentSamplerIdx++ << ")\n";
			}
			if (prof->isLightmapEnabled())
			{
				outStream << ", uniform sampler2D lightMap : register(s" 
					<< currentSamplerIdx++ << ")\n";
			}
			// Blend textures - sampler definitions
			for (uint i = 0; i < numBlendTextures; ++i)
			{
				outStream << ", uniform sampler2D blendTex" << i 
					<< " : register(s" << currentSamplerIdx++ << ")\n";
			}

			// Layer textures - sampler definitions & UV multipliers
			for (uint i = 0; i < numLayers; ++i)
			{
				outStream << ", uniform sampler2D difftex" << i 
					<< " : register(s" << currentSamplerIdx++ << ")\n";
                                
                                if (prof->mLayerNormalMappingEnabled || prof->mLayerParallaxMappingEnabled)
                                        outStream << ", uniform sampler2D normtex" << i 
					<< " : register(s" << currentSamplerIdx++ << ")\n";
			}
		}

		if (prof->isShadowingEnabled(tt, terrain))
		{
			generateFpDynamicShadowsParams(&texCoordSet, &currentSamplerIdx, prof, terrain, tt, outStream);
		}

		// check we haven't exceeded samplers
		if (currentSamplerIdx > 16)
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, 
				"Requested options require too many texture samplers! Try reducing the number of layers.",
				__FUNCTION__);
		}

		outStream << 
			") : COLOR\n"
			"{\n"
			"	float4 outputCol;\n"
			"	float shadow = 1.0;\n"
			"	float2 uv = uvMisc.xy;\n"
			// base colour
			"	outputCol = float4(0,0,0,1);\n";

		if (tt != LOW_LOD)
		{
			outStream << 
				// global normal
				"	float3 normal = expand(tex2D(globalNormal, uv)).rgb;\n";
                                
                                // not needed anymore apparently
                                //"       normal = float3(normal.x, normal.z, -normal.y); \n"; // convert Ogre to MW coordinate system

		}

		outStream <<
			"	float3 lightDir = \n"
			"		lightPosObjSpace.xyz -  (position.xyz * lightPosObjSpace.w);\n"
			"	float3 eyeDir = eyePosObjSpace - position.xyz;\n"

			// set up accumulation areas
			"	float3 diffuse = float3(0,0,0);\n"
			"	float specular = 0;\n";


		if (tt == LOW_LOD)
		{
			// we just do a single calculation from composite map
			outStream <<
				"	float4 composite = tex2D(compositeMap, uv);\n"
				"	diffuse = composite.rgb;\n";
			// TODO - specular; we'll need normals for this!
		}
		else
		{
			// set up the blend values
			for (uint i = 0; i < numBlendTextures; ++i)
			{
				outStream << "	float4 blendTexVal" << i << " = tex2D(blendTex" << i << ", uv);\n";
			}

			if (prof->isLayerNormalMappingEnabled())
			{
				// derive the tangent space basis
				// we do this in the pixel shader because we don't have per-vertex normals
				// because of the LOD, we use a normal map
				// tangent is always +x or -z in object space depending on alignment
				switch(terrain->getAlignment())
				{
				case Terrain::ALIGN_X_Y:
				case Terrain::ALIGN_X_Z:
					outStream << "	float3 tangent = float3(1, 0, 0);\n";
					break;
				case Terrain::ALIGN_Y_Z:
					outStream << "	float3 tangent = float3(0, 0, -1);\n";
					break;
				};

				outStream << "	float3 binormal = normalize(cross(tangent, normal));\n";
				// note, now we need to re-cross to derive tangent again because it wasn't orthonormal
				outStream << "	tangent = normalize(cross(normal, binormal));\n";
				// derive final matrix
				outStream << "	float3x3 TBN = float3x3(tangent, binormal, normal);\n";

				// set up lighting result placeholders for interpolation
				outStream <<  "	float4 litRes, litResLayer;\n";
				outStream << "	float3 TSlightDir, TSeyeDir, TShalfAngle, TSnormal;\n";
				if (prof->isLayerParallaxMappingEnabled())
					outStream << "	float displacement;\n";
				// move 
				outStream << "	TSlightDir = normalize(mul(TBN, lightDir));\n";
				outStream << "	TSeyeDir = normalize(mul(TBN, eyeDir));\n";

			}
			else
			{
				// simple per-pixel lighting with no normal mapping
				outStream << "	lightDir = normalize(lightDir);\n";
				outStream << "	eyeDir = normalize(eyeDir);\n";
				outStream << "	float3 halfAngle = normalize(lightDir + eyeDir);\n";
				outStream << "	float4 litRes = lit(dot(lightDir, normal), dot(halfAngle, normal), scaleBiasSpecular.z);\n";

			}
		}


	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateVpLayer(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream)
	{
		// nothing to do
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateFpLayer(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream)
	{
		uint uvIdx = layer / 2;
		String uvChannels = layer % 2 ? ".zw" : ".xy";
		uint blendIdx = (layer-1) / 4;
		String blendChannel = getChannel(layer-1);
		String blendWeightStr = String("blendTexVal") + StringConverter::toString(blendIdx) + 
			"." + blendChannel;

		// generate early-out conditional
		// Disable - causing some issues even when trying to force the use of texldd
                
                // comment by scrawl:
                // on a NVIDIA card in opengl mode, didn't produce any problems,
                // while increasing FPS from 170 to 185 (!!!) in the same area
                // so let's try this out - if something does cause problems for
                // someone else (with a different card / renderer) we can just
                // add a vendor-specific check here
		if (layer && prof->_isSM3Available())
			outStream << "  if (" << blendWeightStr << " > 0.0003)\n  { \n";
		

		// generate UV
		outStream << "	float2 uv" << layer << " = layerUV" << uvIdx << uvChannels << ";\n";

		// calculate lighting here if normal mapping
		if (prof->isLayerNormalMappingEnabled())
		{
			if (prof->isLayerParallaxMappingEnabled() && tt != RENDER_COMPOSITE_MAP)
			{
				// modify UV - note we have to sample an extra time
				outStream << "	displacement = tex2D(normtex" << layer << ", uv" << layer << ").a\n"
					"		* scaleBiasSpecular.x + scaleBiasSpecular.y;\n";
				outStream << "	uv" << layer << " += TSeyeDir.xy * displacement;\n";
			}

			// access TS normal map
			outStream << "	TSnormal = expand(tex2D(normtex" << layer << ", uv" << layer << ")).rgb;\n";
			outStream << "	TShalfAngle = normalize(TSlightDir + TSeyeDir);\n";
			outStream << "	litResLayer = lit(dot(TSlightDir, TSnormal), dot(TShalfAngle, TSnormal), scaleBiasSpecular.z);\n";
			if (!layer)
				outStream << "	litRes = litResLayer;\n";
			else
				outStream << "	litRes = lerp(litRes, litResLayer, " << blendWeightStr << ");\n";

		}

		// sample diffuse texture
		outStream << "	float4 diffuseSpecTex" << layer 
			<< " = tex2D(difftex" << layer << ", uv" << layer << ");\n";

		// apply to common
		if (!layer)
		{
			outStream << "	diffuse = diffuseSpecTex0.rgb;\n";
			if (prof->isLayerSpecularMappingEnabled())
				outStream << "	specular = diffuseSpecTex0.a;\n";
		}
		else
		{
			outStream << "	diffuse = lerp(diffuse, diffuseSpecTex" << layer 
				<< ".rgb, " << blendWeightStr << ");\n";
			if (prof->isLayerSpecularMappingEnabled())
				outStream << "	specular = lerp(specular, diffuseSpecTex" << layer 
					<< ".a, " << blendWeightStr << ");\n";

		}

		// End early-out
		// Disable - causing some issues even when trying to force the use of texldd
                
                // comment by scrawl:
                // on a NVIDIA card in opengl mode, didn't produce any problems,
                // while increasing FPS from 170 to 185 (!!!) in the same area
                // so let's try this out - if something does cause problems for
                // someone else (with a different card / renderer) we can just
                // add a vendor-specific check here
		if (layer && prof->_isSM3Available())
			outStream << "  } // early-out blend value\n";
		
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateVpFooter(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{

		outStream << 
			"	oPos = mul(viewProjMatrix, worldPos);\n"
			"	oUVMisc.xy = uv.xy;\n";

		bool fog = terrain->getSceneManager()->getFogMode() != FOG_NONE && tt != RENDER_COMPOSITE_MAP;
		if (fog)
		{
			if (terrain->getSceneManager()->getFogMode() == FOG_LINEAR)
			{
				outStream <<
					"	fogVal = saturate((oPos.z - fogParams.y) * fogParams.w);\n";
			}
			else
			{
				outStream <<
					"	fogVal = saturate(1 / (exp(oPos.z * fogParams.x)));\n";
			}
		}
		
		if (prof->isShadowingEnabled(tt, terrain))
			generateVpDynamicShadows(prof, terrain, tt, outStream);

		outStream << 
			"}\n";


	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateFpFooter(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{

		if (tt == LOW_LOD)
		{
			if (prof->isShadowingEnabled(tt, terrain))
			{
				generateFpDynamicShadows(prof, terrain, tt, outStream);
				outStream << 
					"	outputCol.rgb = diffuse * rtshadow;\n";
			}
			else
			{
				outStream << 
					"	outputCol.rgb = diffuse;\n";
			}
		}
		else
		{
			if (terrain->getGlobalColourMapEnabled() && prof->isGlobalColourMapEnabled())
			{
				// sample colour map and apply to diffuse
				outStream << "	diffuse *= tex2D(globalColourMap, uv).rgb;\n";
			}
			if (prof->isLightmapEnabled())
			{
				// sample lightmap
				outStream << "	shadow = tex2D(lightMap, uv).r;\n";
			}

			if (prof->isShadowingEnabled(tt, terrain))
			{
				generateFpDynamicShadows(prof, terrain, tt, outStream);
			}

			// diffuse lighting
			outStream << "	outputCol.rgb += ambient * diffuse + litRes.y * lightDiffuseColour * diffuse * shadow;\n";

			// specular default
			if (!prof->isLayerSpecularMappingEnabled())
				outStream << "	specular = 0.0;\n";

			if (tt == RENDER_COMPOSITE_MAP)
			{
				// Lighting embedded in alpha
				outStream <<
					"	outputCol.a = shadow;\n";

			}
			else
			{
				// Apply specular
				//outStream << "	outputCol.rgb += litRes.z * lightSpecularColour * specular * shadow;\n";

				if (prof->getParent()->getDebugLevel())
				{
					outStream << "	outputCol.rg += lodInfo.xy;\n";
				}
			}
		}

		bool fog = terrain->getSceneManager()->getFogMode() != FOG_NONE && tt != RENDER_COMPOSITE_MAP;
		if (fog)
		{
			outStream << "	outputCol.rgb = lerp(outputCol.rgb, fogColour, fogVal);\n";
		}

		// Final return
		outStream << "  return outputCol;\n"
			<< "}\n";

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateFpDynamicShadowsHelpers(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		// TODO make filtering configurable
		outStream <<
			"// Simple PCF \n"
			"// Number of samples in one dimension (square for total samples) \n"
			"#define NUM_SHADOW_SAMPLES_1D 2.0 \n"
			"#define SHADOW_FILTER_SCALE 1 \n"

			"#define SHADOW_SAMPLES NUM_SHADOW_SAMPLES_1D*NUM_SHADOW_SAMPLES_1D \n"

			"float4 offsetSample(float4 uv, float2 offset, float invMapSize) \n"
			"{ \n"
			"	return float4(uv.xy + offset * invMapSize * uv.w, uv.z, uv.w); \n"
			"} \n";

		if (prof->getReceiveDynamicShadowsDepth())
		{
			outStream << 
				"float calcDepthShadow(sampler2D shadowMap, float4 uv, float invShadowMapSize) \n"
				"{ \n"
				"	// 4-sample PCF \n"
					
				"	float shadow = 0.0; \n"
				"	float offset = (NUM_SHADOW_SAMPLES_1D/2 - 0.5) * SHADOW_FILTER_SCALE; \n"
				"	for (float y = -offset; y <= offset; y += SHADOW_FILTER_SCALE) \n"
				"		for (float x = -offset; x <= offset; x += SHADOW_FILTER_SCALE) \n"
				"		{ \n"
				"			float4 newUV = offsetSample(uv, float2(x, y), invShadowMapSize);\n"
				"			// manually project and assign derivatives \n"
				"			// to avoid gradient issues inside loops \n"
				"			newUV = newUV / newUV.w; \n"
				"			float depth = tex2D(shadowMap, newUV.xy, 1, 1).x; \n"
				"			if (depth >= 1 || depth >= uv.z)\n"
				"				shadow += 1.0;\n"
				"		} \n"

				"	shadow /= SHADOW_SAMPLES; \n"

				"	return shadow; \n"
				"} \n";
		}
		else
		{
			outStream <<
				"float calcSimpleShadow(sampler2D shadowMap, float4 shadowMapPos) \n"
				"{ \n"
				"	return tex2Dproj(shadowMap, shadowMapPos).x; \n"
				"} \n";

		}

		if (prof->getReceiveDynamicShadowsPSSM())
		{
			uint numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();


			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream <<
					"float calcPSSMDepthShadow(";
			}
			else
			{
				outStream <<
					"float calcPSSMSimpleShadow(";
			}

			outStream << "\n	";
			for (uint i = 0; i < numTextures; ++i)
				outStream << "sampler2D shadowMap" << i << ", ";
			outStream << "\n	";
			for (uint i = 0; i < numTextures; ++i)
				outStream << "float4 lsPos" << i << ", ";
			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream << "\n	";
				for (uint i = 0; i < numTextures; ++i)
					outStream << "float invShadowmapSize" << i << ", ";
			}
			outStream << "\n"
				"	float4 pssmSplitPoints, float camDepth) \n"
				"{ \n"
				"	float shadow; \n"
				"	// calculate shadow \n";
			
			for (uint i = 0; i < numTextures; ++i)
			{
				if (!i)
					outStream << "	if (camDepth <= pssmSplitPoints." << ShaderHelper::getChannel(i) << ") \n";
				else if (i < numTextures - 1)
					outStream << "	else if (camDepth <= pssmSplitPoints." << ShaderHelper::getChannel(i) << ") \n";
				else
					outStream << "	else \n";

				outStream <<
					"	{ \n";
				if (prof->getReceiveDynamicShadowsDepth())
				{
					outStream <<
						"		shadow = calcDepthShadow(shadowMap" << i << ", lsPos" << i << ", invShadowmapSize" << i << "); \n";
				}
				else
				{
					outStream <<
						"		shadow = calcSimpleShadow(shadowMap" << i << ", lsPos" << i << "); \n";
				}
				outStream <<
					"	} \n";

			}

			outStream <<
				"	return shadow; \n"
				"} \n\n\n";
		}


	}
	//---------------------------------------------------------------------
	uint TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateVpDynamicShadowsParams(
		uint texCoord, const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		// out semantics & params
		uint numTextures = 1;
		if (prof->getReceiveDynamicShadowsPSSM())
		{
			numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
		}
		for (uint i = 0; i < numTextures; ++i)
		{
			outStream <<
				", out float4 oLightSpacePos" << i << " : TEXCOORD" << texCoord++ << " \n" <<
				", uniform float4x4 texViewProjMatrix" << i << " \n";
			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream <<
					", uniform float4 depthRange" << i << " // x = min, y = max, z = range, w = 1/range \n";
			}
		}

		return texCoord;

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateVpDynamicShadows(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		uint numTextures = 1;
		if (prof->getReceiveDynamicShadowsPSSM())
		{
			numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
		}

		// Calculate the position of vertex in light space
		for (uint i = 0; i < numTextures; ++i)
		{
			outStream <<
				"	oLightSpacePos" << i << " = mul(texViewProjMatrix" << i << ", worldPos); \n";
			if (prof->getReceiveDynamicShadowsDepth())
			{
				// make linear
				outStream <<
					"oLightSpacePos" << i << ".z = (oLightSpacePos" << i << ".z - depthRange" << i << ".x) * depthRange" << i << ".w;\n";

			}
		}


		if (prof->getReceiveDynamicShadowsPSSM())
		{
			outStream <<
				"	// pass cam depth\n"
				"	oUVMisc.z = oPos.z;\n";
		}

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateFpDynamicShadowsParams(
		uint* texCoord, uint* sampler, const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		if (tt == HIGH_LOD)
			mShadowSamplerStartHi = *sampler;
		else if (tt == LOW_LOD)
			mShadowSamplerStartLo = *sampler;

		// in semantics & params
		uint numTextures = 1;
		if (prof->getReceiveDynamicShadowsPSSM())
		{
			numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
			outStream <<
				", uniform float4 pssmSplitPoints \n";
		}
		for (uint i = 0; i < numTextures; ++i)
		{
			outStream <<
				", float4 lightSpacePos" << i << " : TEXCOORD" << *texCoord << " \n" <<
				", uniform sampler2D shadowMap" << i << " : register(s" << *sampler << ") \n";
			*sampler = *sampler + 1;
			*texCoord = *texCoord + 1;
			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream <<
					", uniform float inverseShadowmapSize" << i << " \n";
			}
		}

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorB::SM2Profile::ShaderHelperCg::generateFpDynamicShadows(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream)
	{
		if (prof->getReceiveDynamicShadowsPSSM())
		{
			uint numTextures = prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
			outStream << 
				"	float camDepth = uvMisc.z;\n";

			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream << 
					"	float rtshadow = calcPSSMDepthShadow(";
			}
			else
			{
				outStream << 
					"	float rtshadow = calcPSSMSimpleShadow(";
			}
			for (uint i = 0; i < numTextures; ++i)
				outStream << "shadowMap" << i << ", ";
			outStream << "\n		";

			for (uint i = 0; i < numTextures; ++i)
				outStream << "lightSpacePos" << i << ", ";
			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream << "\n		";
				for (uint i = 0; i < numTextures; ++i)
					outStream << "inverseShadowmapSize" << i << ", ";
			}
			outStream << "\n" <<
				"		pssmSplitPoints, camDepth);\n";

		}
		else
		{
			if (prof->getReceiveDynamicShadowsDepth())
			{
				outStream <<
					"	float rtshadow = calcDepthShadow(shadowMap0, lightSpacePos0, inverseShadowmapSize0);";
			}
			else
			{
				outStream <<
					"	float rtshadow = calcSimpleShadow(shadowMap0, lightSpacePos0);";
			}
		}

		outStream << 
			"	shadow = min(shadow, rtshadow);\n";
		
	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
	TerrainMaterialGeneratorB::SM2Profile::ShaderHelperHLSL::createVertexProgram(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
		String progName = getVertexProgramName(prof, terrain, tt);

		HighLevelGpuProgramPtr ret = mgr.getByName(progName);
		if (ret.isNull())
		{
			ret = mgr.createProgram(progName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
				"hlsl", GPT_VERTEX_PROGRAM);
		}
		else
		{
			ret->unload();
		}

		if (prof->_isSM3Available())
			ret->setParameter("target", "vs_3_0");
		else
			ret->setParameter("target", "vs_2_0");
		ret->setParameter("entry_point", "main_vp");

		return ret;

	}
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
	TerrainMaterialGeneratorB::SM2Profile::ShaderHelperHLSL::createFragmentProgram(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
		String progName = getFragmentProgramName(prof, terrain, tt);


		HighLevelGpuProgramPtr ret = mgr.getByName(progName);
		if (ret.isNull())
		{
			ret = mgr.createProgram(progName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
				"hlsl", GPT_FRAGMENT_PROGRAM);
		}
		else
		{
			ret->unload();
		}

		if (prof->_isSM3Available())
			ret->setParameter("target", "ps_3_0");
		else
			ret->setParameter("target", "ps_2_x");
		ret->setParameter("entry_point", "main_fp");

		return ret;

	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
	TerrainMaterialGeneratorB::SM2Profile::ShaderHelperGLSL::createVertexProgram(
		const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
		String progName = getVertexProgramName(prof, terrain, tt);

		switch(tt)
		{
		case HIGH_LOD:
			progName += "/hlod";
			break;
		case LOW_LOD:
			progName += "/llod";
			break;
		case RENDER_COMPOSITE_MAP:
			progName += "/comp";
			break;
		}

		HighLevelGpuProgramPtr ret = mgr.getByName(progName);
		if (ret.isNull())
		{
			ret = mgr.createProgram(progName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
				"glsl", GPT_VERTEX_PROGRAM);
		}
		else
		{
			ret->unload();
		}

		return ret;

	}
	//---------------------------------------------------------------------
	HighLevelGpuProgramPtr
		TerrainMaterialGeneratorB::SM2Profile::ShaderHelperGLSL::createFragmentProgram(
			const SM2Profile* prof, const Terrain* terrain, TechniqueType tt)
	{
		HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
		String progName = getFragmentProgramName(prof, terrain, tt);

		HighLevelGpuProgramPtr ret = mgr.getByName(progName);
		if (ret.isNull())
		{
			ret = mgr.createProgram(progName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
				"glsl", GPT_FRAGMENT_PROGRAM);
		}
		else
		{
			ret->unload();
		}

		return ret;

	}


}
