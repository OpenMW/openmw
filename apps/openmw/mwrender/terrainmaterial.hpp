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

#ifndef __Ogre_TerrainMaterialGeneratorB_H__
#define __Ogre_TerrainMaterialGeneratorB_H__

#include "OgreTerrainPrerequisites.h"
#include "OgreTerrainMaterialGenerator.h"
#include "OgreGpuProgramParams.h"

namespace Ogre
{
	class PSSMShadowCameraSetup;

	/** \addtogroup Optional Components
	*  @{
	*/
	/** \addtogroup Terrain
	*  Some details on the terrain component
	*  @{
	*/


	/** A TerrainMaterialGenerator which can cope with normal mapped, specular mapped
		terrain. 
		@note Requires the Cg plugin to render correctly
	*/
	class _OgreTerrainExport TerrainMaterialGeneratorB : public TerrainMaterialGenerator
	{
	public:
		TerrainMaterialGeneratorB();
		~TerrainMaterialGeneratorB();

		/** Shader model 2 profile target. 
		*/
		class _OgreTerrainExport SM2Profile : public TerrainMaterialGenerator::Profile
		{
		public:
			SM2Profile(TerrainMaterialGenerator* parent, const String& name, const String& desc);
			~SM2Profile();
			MaterialPtr generate(const Terrain* terrain);
			MaterialPtr generateForCompositeMap(const Terrain* terrain);
			uint8 getMaxLayers(const Terrain* terrain) const;
			void updateParams(const MaterialPtr& mat, const Terrain* terrain);
			void updateParamsForCompositeMap(const MaterialPtr& mat, const Terrain* terrain);
			void requestOptions(Terrain* terrain);

			/** Whether to support normal mapping per layer in the shader (default true). 
			*/
			bool isLayerNormalMappingEnabled() const  { return mLayerNormalMappingEnabled; }
			/** Whether to support normal mapping per layer in the shader (default true). 
			*/
			void setLayerNormalMappingEnabled(bool enabled);
			/** Whether to support parallax mapping per layer in the shader (default true). 
			*/
			bool isLayerParallaxMappingEnabled() const  { return mLayerParallaxMappingEnabled; }
			/** Whether to support parallax mapping per layer in the shader (default true). 
			*/
			void setLayerParallaxMappingEnabled(bool enabled);
			/** Whether to support specular mapping per layer in the shader (default true). 
			*/
			bool isLayerSpecularMappingEnabled() const  { return mLayerSpecularMappingEnabled; }
			/** Whether to support specular mapping per layer in the shader (default true). 
			*/
			void setLayerSpecularMappingEnabled(bool enabled);
			/** Whether to support a global colour map over the terrain in the shader,
				if it's present (default true). 
			*/
			bool isGlobalColourMapEnabled() const  { return mGlobalColourMapEnabled; }
			/** Whether to support a global colour map over the terrain in the shader,
			if it's present (default true). 
			*/
			void setGlobalColourMapEnabled(bool enabled);
			/** Whether to support a light map over the terrain in the shader,
			if it's present (default true). 
			*/
			bool isLightmapEnabled() const  { return mLightmapEnabled; }
			/** Whether to support a light map over the terrain in the shader,
			if it's present (default true). 
			*/
			void setLightmapEnabled(bool enabled);
			/** Whether to use the composite map to provide a lower LOD technique
				in the distance (default true). 
			*/
			bool isCompositeMapEnabled() const  { return mCompositeMapEnabled; }
			/** Whether to use the composite map to provide a lower LOD technique
			in the distance (default true). 
			*/
			void setCompositeMapEnabled(bool enabled);
			/** Whether to support dynamic texture shadows received from other 
				objects, on the terrain (default true). 
			*/
			bool getReceiveDynamicShadowsEnabled() const  { return mReceiveDynamicShadows; }
			/** Whether to support dynamic texture shadows received from other 
			objects, on the terrain (default true). 
			*/
			void setReceiveDynamicShadowsEnabled(bool enabled);

			/** Whether to use PSSM support dynamic texture shadows, and if so the 
				settings to use (default 0). 
			*/
			void setReceiveDynamicShadowsPSSM(PSSMShadowCameraSetup* pssmSettings);
			/** Whether to use PSSM support dynamic texture shadows, and if so the 
			settings to use (default 0). 
			*/
			PSSMShadowCameraSetup* getReceiveDynamicShadowsPSSM() const { return mPSSM; }
			/** Whether to use depth shadows (default false). 
			*/
			void setReceiveDynamicShadowsDepth(bool enabled);
			/** Whether to use depth shadows (default false). 
			*/
			bool getReceiveDynamicShadowsDepth() const { return mDepthShadows; }
			/** Whether to use shadows on low LOD material rendering (when using composite map) (default false). 
			*/
			void setReceiveDynamicShadowsLowLod(bool enabled);
			/** Whether to use shadows on low LOD material rendering (when using composite map) (default false). 
			*/
			bool getReceiveDynamicShadowsLowLod() const { return mLowLodShadows; }

                        int getNumberOfLightsSupported() const;

			/// Internal
			bool _isSM3Available() const { return mSM3Available; }
		
		protected:

			enum TechniqueType
			{
				HIGH_LOD, 
				LOW_LOD, 
				RENDER_COMPOSITE_MAP
			};
			void addTechnique(const MaterialPtr& mat, const Terrain* terrain, TechniqueType tt);

			/// Interface definition for helper class to generate shaders
			class _OgreTerrainExport ShaderHelper : public TerrainAlloc
			{
			public:
				ShaderHelper() {}
				virtual ~ShaderHelper() {}
				virtual HighLevelGpuProgramPtr generateVertexProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				virtual HighLevelGpuProgramPtr generateFragmentProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				virtual void updateParams(const SM2Profile* prof, const MaterialPtr& mat, const Terrain* terrain, bool compositeMap);
			protected:
				virtual String getVertexProgramName(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				virtual String getFragmentProgramName(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				virtual HighLevelGpuProgramPtr createVertexProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt) = 0;
				virtual HighLevelGpuProgramPtr createFragmentProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt) = 0;
				virtual void generateVertexProgramSource(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				virtual void generateFragmentProgramSource(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				virtual void generateVpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) = 0;
				virtual void generateFpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) = 0;
				virtual void generateVpLayer(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream) = 0;
				virtual void generateFpLayer(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream) = 0;
				virtual void generateVpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) = 0;
				virtual void generateFpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) = 0;
				virtual void defaultVpParams(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const HighLevelGpuProgramPtr& prog);
				virtual void defaultFpParams(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const HighLevelGpuProgramPtr& prog);
				virtual void updateVpParams(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const GpuProgramParametersSharedPtr& params);
				virtual void updateFpParams(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, const GpuProgramParametersSharedPtr& params);
				static String getChannel(uint idx);

				size_t mShadowSamplerStartHi;
				size_t mShadowSamplerStartLo;

			};

			/// Utility class to help with generating shaders for Cg / HLSL.
			class _OgreTerrainExport ShaderHelperCg : public ShaderHelper
			{
			protected:
				HighLevelGpuProgramPtr createVertexProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				HighLevelGpuProgramPtr createFragmentProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				void generateVpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateFpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateVpLayer(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream);
				void generateFpLayer(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream);
				void generateVpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateFpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				uint generateVpDynamicShadowsParams(uint texCoordStart, const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateVpDynamicShadows(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateFpDynamicShadowsHelpers(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateFpDynamicShadowsParams(uint* texCoord, uint* sampler, const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
				void generateFpDynamicShadows(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream);
			};

			class _OgreTerrainExport ShaderHelperHLSL : public ShaderHelperCg
			{
			protected:
				HighLevelGpuProgramPtr createVertexProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				HighLevelGpuProgramPtr createFragmentProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
			};

			/// Utility class to help with generating shaders for GLSL.
			class _OgreTerrainExport ShaderHelperGLSL : public ShaderHelper
			{
			protected:
				HighLevelGpuProgramPtr createVertexProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				HighLevelGpuProgramPtr createFragmentProgram(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt);
				void generateVpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) {}
				void generateFpHeader(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) {}
				void generateVpLayer(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream) {}
				void generateFpLayer(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, uint layer, StringUtil::StrStreamType& outStream) {}
				void generateVpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) {}
				void generateFpFooter(const SM2Profile* prof, const Terrain* terrain, TechniqueType tt, StringUtil::StrStreamType& outStream) {}
			};

			ShaderHelper* mShaderGen;
			bool mLayerNormalMappingEnabled;
			bool mLayerParallaxMappingEnabled;
			bool mLayerSpecularMappingEnabled;
			bool mGlobalColourMapEnabled;
			bool mLightmapEnabled;
			bool mCompositeMapEnabled;
			bool mReceiveDynamicShadows;
			PSSMShadowCameraSetup* mPSSM;
			bool mDepthShadows;
			bool mLowLodShadows;
			bool mSM3Available;

			bool isShadowingEnabled(TechniqueType tt, const Terrain* terrain) const;

		};




	};



	/** @} */
	/** @} */


}

#endif
