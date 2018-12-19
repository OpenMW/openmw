/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

/* ParallelSplitShadowMap written by Adrian Egli
 *
 * this version has still a bug in mutli-thread application (flickering problem)
 * to avoid the flickering problem try osgShadow --pssm --SingleThreaded your_scene.ive
 *
 * The Parallel Split Shadow Map only supports directional light for simulating the shadow.
 * It's one of the most robust algorithm for huge terrain sun light's shadow simulation, if
 * you need to shadow a terrain, or another huge scene, you should use Parallel Split Shadow Map
 * or at least test it against your scene. Have fun.
 *
 */

#ifndef OSGMW_ParallelSplitShadowMap
#define OSGMW_ParallelSplitShadowMap 1

#include <osg/Camera>
#include <osg/Material>
#include <osg/Depth>
#include <osg/ClipPlane>

#include <osgShadow/ShadowTechnique>

namespace MWRender
{
class  ParallelSplitShadowMap :  public osgShadow::ShadowTechnique
{
    public:
        ParallelSplitShadowMap(osg::Geode** debugGroup=NULL, int icountplanes=3);

        ParallelSplitShadowMap(const ParallelSplitShadowMap& es, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Object(MWRender, ParallelSplitShadowMap);


        /** Initialize the ShadowedScene and local cached data structures.*/
        virtual void init();

        /** Run the update traversal of the ShadowedScene and update any loca chached data structures.*/
        virtual void update(osg::NodeVisitor& nv);

        /** Run the cull traversal of the ShadowedScene and set up the rendering for this ShadowTechnique.*/
        virtual void cull(osgUtil::CullVisitor& cv);

        /** Clean scene graph from any shadow technique specific nodes, state and drawables.*/
        virtual void cleanSceneGraph();

        /** Switch on the debug coloring in GLSL (only the first 3 texture/splits showed for visualisation */
        inline void setDebugColorOn() { _debug_color_in_GLSL = true; }

        /** Set the polygon offset osg::Vec2f(factor,unit) */
        inline void setPolygonOffset(const osg::Vec2f& p) { _polgyonOffset = p;_user_polgyonOffset_set=true;}

        /** Get the polygon offset osg::Vec2f(factor,unit) */
        inline const osg::Vec2f& getPolygonOffset() const { return _polgyonOffset;}

        /** Set the texture resolution */
        inline void setTextureResolution(unsigned int resolution) { _resolution = resolution; }

        /** Get the texture resolution */
        inline unsigned int getTextureResolution() const { return _resolution; }
        /** Set the texture resolution */
        inline void setSplitCount(unsigned int i) { _number_of_splits = i; }

        /** Get the texture resolution */
        inline unsigned int getSplitCount() const { return _number_of_splits; }

        /** Set shadow textures offset */
        inline void setTextureOffset(unsigned int i) { _textureUnitOffset = i; }

        /** Get shadow textures offset */
        inline unsigned int getTextureOffset() const { return _textureUnitOffset; }

        /** Set the max far distance */
        inline void setMaxFarDistance(double farDist) { _setMaxFarDistance = farDist; _isSetMaxFarDistance = true; }

        /** Get the max far distance */
        inline double getMaxFarDistance() const { return _setMaxFarDistance; }

        /** Set the factor for moving the virtual camera behind the real camera*/
        inline void setMoveVCamBehindRCamFactor(double distFactor ) { _move_vcam_behind_rcam_factor = distFactor; }

        /** Get the factor for moving the virtual camera behind the real camera*/
        inline double getMoveVCamBehindRCamFactor() const { return _move_vcam_behind_rcam_factor; }

        /** Set min near distance for splits */
        inline void setMinNearDistanceForSplits(double nd){ _split_min_near_dist=nd; }

        /** Get min near distance for splits */
        inline double getMinNearDistanceForSplits() const { return _split_min_near_dist; }

        /** set a user defined light for shadow simulation (sun light, ... )
         *    when this light get passed to pssm, the scene's light are no longer collected
         *    and simulated. just this user passed light, it needs to be a directional light.
         */
        inline void setUserLight(osg::Light* light) { _userLight = light; }

        /** get the user defined light for shadow simulation */
        inline const osg::Light* getUserLight() const { return _userLight.get(); }

        /** Set the values for the ambient bias the shader will use.*/
        void setAmbientBias(const osg::Vec2& ambientBias );

        /** Get the values for the ambient bias the shader will use.*/
        const osg::Vec2& getAmbientBias() const { return _ambientBias; }

        /**
         * you can overwrite the fragment shader if you like to modify it yourself, own fragment shader can be used
         */
        class OSGSHADOW_EXPORT FragmentShaderGenerator : public osg::Referenced {
            public:
                /**
                 * generate the GLSL fragment shader
                 */
                virtual std::string generateGLSL_FragmentShader_BaseTex(bool debug, unsigned int splitCount,double textureRes, bool filtered, unsigned int nbrSplits,unsigned int textureOffset);
        };

        /** set fragment shader generator */
        inline void setFragmentShaderGenerator(FragmentShaderGenerator* fsw) { _FragmentShaderGenerator = fsw;}

        /** enable / disable shadow filtering */
        inline void enableShadowGLSLFiltering(bool filtering = true) {  _GLSL_shadow_filtered = filtering; }

        enum SplitCalcMode {
            SPLIT_LINEAR,
            SPLIT_EXP
        };

        /** set split calculation mode */
        inline void setSplitCalculationMode(SplitCalcMode scm=SPLIT_EXP) { _SplitCalcMode = scm; }

        /** get split calculation mode */
        inline SplitCalcMode getSplitCalculationMode() const { return _SplitCalcMode; }


        /** Resize any per context GLObject buffers to specified size. */
        virtual void resizeGLObjectBuffers(unsigned int maxSize);

        /** If State is non-zero, this function releases any associated OpenGL objects for
           * the specified graphics context. Otherwise, releases OpenGL objects
           * for all graphics contexts. */
        virtual void releaseGLObjects(osg::State* = 0) const;

    protected :

        virtual ~ParallelSplitShadowMap() {}

        osg::ref_ptr<osg::Uniform>       _iMVPT;
          std::vector< osg::ref_ptr<osg::Camera>   > _cams;//redondant
         osg::ref_ptr<osg::Camera>  lightcam;
         osg::ref_ptr<osgUtil::CullVisitor> _shadowCullHelper;
        struct PSSMShadowSplitTexture {
            // RTT
            osg::ref_ptr<osg::Camera>       _camera;
            osg::ref_ptr<osg::TexGen>       _texgen;
         //   osg::ref_ptr<osg::Uniform>       _iMVPT;
            osg::ref_ptr<osg::Texture2D>    _texture;
            osg::ref_ptr<osg::StateSet>     _stateset;
            unsigned int                    _textureUnit;


            double                            _split_far;

            osg::ref_ptr<osg::Camera>       _debug_camera;
            osg::ref_ptr<osg::Texture2D>    _debug_texture;
            osg::ref_ptr<osg::StateSet>     _debug_stateset;
            unsigned int                    _debug_textureUnit;

            // Light (SUN)
            osg::Vec3d                        _lightCameraSource;
            osg::Vec3d                        _lightCameraTarget;
            osg::Vec3d                        _frustumSplitCenter;
            osg::Vec3d                        _lightDirection;
            double                            _lightNear;
            double                            _lightFar;

            osg::Matrix                       _cameraView;
            osg::Matrix                       _cameraProj;

            unsigned int                      _splitID;
            unsigned int                      _resolution;

            osg::ref_ptr<osg::Uniform>        _farDistanceSplit;

            void resizeGLObjectBuffers(unsigned int maxSize);
            void releaseGLObjects(osg::State* = 0) const;

        };

        typedef std::map<unsigned int,PSSMShadowSplitTexture> PSSMShadowSplitTextureMap;
        PSSMShadowSplitTextureMap _PSSMShadowSplitTextureMap;

    private:
        void calculateFrustumCorners(PSSMShadowSplitTexture &pssmShadowSplitTexture,osg::Vec3d *frustumCorners);
        void calculateLightInitialPosition(PSSMShadowSplitTexture &pssmShadowSplitTexture,osg::Vec3d *frustumCorners);
        void calculateLightNearFarFormFrustum(PSSMShadowSplitTexture &pssmShadowSplitTexture,osg::Vec3d *frustumCorners);
        void calculateLightViewProjectionFormFrustum(PSSMShadowSplitTexture &pssmShadowSplitTexture,osg::Vec3d *frustumCorners);

        osg::Geode** _displayTexturesGroupingNode;

        unsigned int _textureUnitOffset;

        unsigned int _number_of_splits;

        bool _debug_color_in_GLSL;

        osg::Vec2 _polgyonOffset;
        bool _user_polgyonOffset_set;

        unsigned int _resolution;

        double _setMaxFarDistance;
        bool _isSetMaxFarDistance;

        double _split_min_near_dist;

        double _move_vcam_behind_rcam_factor;

        osg::ref_ptr<osg::Light> _userLight;
        osg::ref_ptr<FragmentShaderGenerator> _FragmentShaderGenerator;

        bool            _GLSL_shadow_filtered;
        SplitCalcMode   _SplitCalcMode;

        osg::Uniform*   _ambientBiasUniform;
        osg::Vec2       _ambientBias;

};
}
#endif
