// clang-format off
/* This file is based on OpenSceneGraph's include/osgShadow/ViewDependentShadowMap.
 * Where applicable, any changes made are covered by OpenMW's GPL 3 license, not the OSGPL.
 * The original copyright notice is listed below.
 */

/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2011 Robert Osfield
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

#ifndef COMPONENTS_SCENEUTIL_MWSHADOWTECHNIQUE_H
#define COMPONENTS_SCENEUTIL_MWSHADOWTECHNIQUE_H 1

#include <array>
#include <mutex>
#include <string>

#include <osg/Camera>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/LightSource>
#include <osg/PolygonOffset>

#include <osgShadow/ShadowTechnique>

#include <components/shader/shadermanager.hpp>

// NOLINTBEGIN(readability-identifier-naming)

namespace SceneUtil {

    /** ViewDependentShadowMap provides an base implementation of view dependent shadow mapping techniques.*/
    class MWShadowTechnique : public osgShadow::ShadowTechnique
    {
    public:
        MWShadowTechnique();

        MWShadowTechnique(const MWShadowTechnique& vdsm, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_Object(SceneUtil, MWShadowTechnique)

        /** initialize the ShadowedScene and local cached data structures.*/
        void init() override;

        /** run the update traversal of the ShadowedScene and update any loca chached data structures.*/
        void update(osg::NodeVisitor& nv) override;

        /** run the cull traversal of the ShadowedScene and set up the rendering for this ShadowTechnique.*/
        void cull(osgUtil::CullVisitor& cv) override;

        /** Resize any per context GLObject buffers to specified size. */
        void resizeGLObjectBuffers(unsigned int maxSize) override;

        /** If State is non-zero, this function releases any associated OpenGL objects for
        * the specified graphics context. Otherwise, releases OpenGL objects
        * for all graphics contexts. */
        void releaseGLObjects(osg::State* = 0) const override;

        /** Clean scene graph from any shadow technique specific nodes, state and drawables.*/
        void cleanSceneGraph() override;

        virtual void enableShadows();

        virtual void disableShadows(bool setDummyState = false);

        virtual void enableDebugHUD();

        virtual void disableDebugHUD();

        virtual void setSplitPointUniformLogarithmicRatio(double ratio);

        virtual void setSplitPointDeltaBias(double bias);

        virtual void setPolygonOffset(float factor, float units);

        virtual void setShadowFadeStart(float shadowFadeStart);

        virtual void enableFrontFaceCulling();

        virtual void disableFrontFaceCulling();

        virtual void setupCastingShader(Shader::ShaderManager &shaderManager);

        class ComputeLightSpaceBounds : public osg::NodeVisitor, public osg::CullStack
        {
        public:
            ComputeLightSpaceBounds();

            void apply(osg::Node& node) override final;
            void apply(osg::Group& node) override;

            void apply(osg::Drawable& drawable) override final;
            void apply(osg::Geometry& drawable) override;

            void apply(osg::Billboard&) override;

            void apply(osg::Projection&) override;

            void apply(osg::Transform& transform) override final;
            void apply(osg::MatrixTransform& transform) override;

            void apply(osg::Camera&) override;

            void updateBound(const osg::BoundingBox& bb);

            void update(const osg::Vec3& v);

            void reset() override;

            osg::BoundingBox _bb;
        };

        struct Frustum
        {
            Frustum(osgUtil::CullVisitor* cv, double minZNear, double maxZFar);
            void setCustomClipSpace(const osg::BoundingBoxd& clipCornersOverride);
            void init();

            osg::Matrixd projectionMatrix;
            osg::Matrixd modelViewMatrix;

            bool useCustomClipSpace;
            osg::BoundingBoxd customClipSpace;

            typedef std::vector<osg::Vec3d> Vertices;
            Vertices corners;

            typedef std::vector<unsigned int> Indices;
            typedef std::vector<Indices> Faces;
            Faces faces;

            typedef std::vector<Indices> Edges;
            Edges edges;

            osg::Vec3d eye;
            osg::Vec3d centerNearPlane;
            osg::Vec3d centerFarPlane;
            osg::Vec3d center;
            osg::Vec3d frustumCenterLine;
        };

        /** Custom frustum callback allowing the application to request shadow maps covering a
        * different furstum than the camera normally would cover, by customizing the corners of the clip space. */
        struct CustomFrustumCallback : osg::Referenced
        {
            /** The callback operator.
            * Output the custum frustum to the boundingBox variable.
            * If sharedFrustumHint is set to a valid cull visitor, the shadow maps of that cull visitor will be re-used instead of recomputing new shadow maps 
            * Note that the customClipSpace bounding box will be uninitialized when this operator is called. If it is not initalized, or a valid shared frustum hint set,
            * the resulting shadow map may be invalid. */
            virtual void operator()(osgUtil::CullVisitor& cv, osg::BoundingBoxd& customClipSpace, osgUtil::CullVisitor*& sharedFrustumHint) = 0;
        };
        typedef std::vector< osg::ref_ptr<osg::Uniform> > Uniforms;

        // forward declare
        class ViewDependentData;

        struct LightData : public osg::Referenced
        {
            LightData(ViewDependentData* vdd);

            virtual void setLightData(osg::RefMatrix* lm, const osg::Light* l, const osg::Matrixd& modelViewMatrix);

            ViewDependentData*                  _viewDependentData;

            osg::ref_ptr<osg::RefMatrix>        lightMatrix;
            osg::ref_ptr<const osg::Light>      light;

            osg::Vec4d                          lightPos;
            osg::Vec3d                          lightPos3;
            osg::Vec3d                          lightDir;
            bool                                directionalLight;

            typedef std::vector<unsigned int> ActiveTextureUnits;
            ActiveTextureUnits                   textureUnits;
        };

        typedef std::list< osg::ref_ptr<LightData> > LightDataList;

        struct ShadowData : public osg::Referenced
        {
            ShadowData(ViewDependentData* vdd);

            virtual void releaseGLObjects(osg::State* = 0) const;

            ViewDependentData*                  _viewDependentData;

            unsigned int                        _textureUnit;
            unsigned int                        _sm_i;
            osg::ref_ptr<osg::Texture2D>        _texture;
            osg::ref_ptr<osg::Camera>           _camera;
        };

        typedef std::list< osg::ref_ptr<ShadowData> > ShadowDataList;


        class ViewDependentData : public osg::Referenced
        {
        public:
            ViewDependentData(MWShadowTechnique* vdsm);

            const MWShadowTechnique* getViewDependentShadowMap() const { return _viewDependentShadowMap; }

            LightDataList& getLightDataList() { return _lightDataList; }

            ShadowDataList& getShadowDataList() { return _shadowDataList; }

            osg::StateSet* getStateSet(unsigned int traversalNumber) { return _stateset[traversalNumber % 2].get(); }

            virtual void releaseGLObjects(osg::State* = 0) const;

            unsigned int numValidShadows(void) const { return _numValidShadows; }

            void setNumValidShadows(unsigned int numValidShadows) { _numValidShadows = numValidShadows; }

        protected:
            friend class MWShadowTechnique;
            virtual ~ViewDependentData() {}

            MWShadowTechnique*          _viewDependentShadowMap;

            std::array<osg::ref_ptr<osg::StateSet>, 2> _stateset;

            LightDataList               _lightDataList;
            ShadowDataList              _shadowDataList;
            std::array<Uniforms, 2>     _uniforms;

            unsigned int _numValidShadows;
        };

        virtual ViewDependentData* createViewDependentData(osgUtil::CullVisitor* cv);

        ViewDependentData* getViewDependentData(osgUtil::CullVisitor* cv);

        void copyShadowMap(osgUtil::CullVisitor& cv, ViewDependentData* lhs, ViewDependentData* rhs);

        void setCustomFrustumCallback(CustomFrustumCallback* cfc);

        void copyShadowStateSettings(osgUtil::CullVisitor& cv, ViewDependentData* vdd);

        virtual void createShaders();

        virtual bool selectActiveLights(osgUtil::CullVisitor* cv, ViewDependentData* vdd) const;

        virtual osg::Polytope computeLightViewFrustumPolytope(Frustum& frustum, LightData& positionedLight);

        virtual bool computeShadowCameraSettings(Frustum& frustum, LightData& positionedLight, osg::Matrixd& projectionMatrix, osg::Matrixd& viewMatrix);

        virtual bool cropShadowCameraToMainFrustum(Frustum& frustum, osg::Camera* camera, double viewNear, double viewFar, std::vector<osg::Plane>& planeList);

        virtual bool adjustPerspectiveShadowMapCameraSettings(osgUtil::RenderStage* renderStage, Frustum& frustum, LightData& positionedLight, osg::Camera* camera, double viewNear, double viewFar);
        
        virtual void assignShadowStateSettings(osgUtil::CullVisitor& cv, osg::Camera* camera, unsigned int sm_i, Uniforms& uniforms);
        
        virtual void assignValidRegionSettings(osgUtil::CullVisitor& cv, osg::Camera* camera, unsigned int sm_i, Uniforms& uniforms);

        virtual void cullShadowReceivingScene(osgUtil::CullVisitor* cv) const;

        virtual void cullShadowCastingScene(osgUtil::CullVisitor* cv, osg::Camera* camera) const;

        virtual osg::StateSet* prepareStateSetForRenderingShadow(ViewDependentData& vdd, unsigned int traversalNumber) const;

        void setWorldMask(unsigned int worldMask) { _worldMask = worldMask; }

        osg::ref_ptr<osg::StateSet> getOrCreateShadowsBinStateSet();

    protected:
        virtual ~MWShadowTechnique();

        osg::ref_ptr<ComputeLightSpaceBounds>   _clsb;

        typedef std::map< osgUtil::CullVisitor*, osg::ref_ptr<ViewDependentData> >  ViewDependentDataMap;
        mutable std::mutex                      _viewDependentDataMapMutex;
        ViewDependentDataMap                    _viewDependentDataMap;
        osg::ref_ptr<CustomFrustumCallback>     _customFrustumCallback;
        osg::BoundingBoxd                       _customClipSpace;

        osg::ref_ptr<osg::StateSet>             _shadowRecievingPlaceholderStateSet;

        osg::ref_ptr<osg::StateSet>             _shadowCastingStateSet;
        osg::ref_ptr<osg::PolygonOffset>        _polygonOffset;
        osg::ref_ptr<osg::Texture2D>            _fallbackBaseTexture;
        osg::ref_ptr<osg::Texture2D>            _fallbackShadowMapTexture;

        std::array<Uniforms, 2>                 _uniforms;
        osg::ref_ptr<osg::Program>              _program;

        bool                                    _enableShadows;
        bool                                    mSetDummyStateWhenDisabled;

        double                                  _splitPointUniformLogRatio = 0.5;
        double                                  _splitPointDeltaBias = 0.0;

        float                                   _polygonOffsetFactor = 1.f;
        float                                   _polygonOffsetUnits = 4.0f;

        bool                                    _useFrontFaceCulling = true;

        float                                   _shadowFadeStart = 0.0f;

        unsigned int                            _worldMask = ~0u;

        class DebugHUD final : public osg::Referenced
        {
        public:
            DebugHUD(int numberOfShadowMapsPerLight);

            void draw(osg::ref_ptr<osg::Texture2D> texture, unsigned int shadowMapNumber, const osg::Matrixd &matrix, osgUtil::CullVisitor& cv);

            void releaseGLObjects(osg::State* state = 0) const;

            void setFrustumVertices(osg::ref_ptr<osg::Vec3Array> vertices, unsigned int traversalNumber);
        protected:
            void addAnotherShadowMap();

            static const int sDebugTextureUnit = 0;

            std::vector<osg::ref_ptr<osg::Camera>> mDebugCameras;
            osg::ref_ptr<osg::Program> mDebugProgram;
            std::vector<osg::ref_ptr<osg::Node>> mDebugGeometry;
            std::vector<osg::ref_ptr<osg::Group>> mFrustumTransforms;
            std::array<std::vector<osg::ref_ptr<osg::Uniform>>, 2> mFrustumUniforms;
            std::array<osg::ref_ptr<osg::Geometry>, 2> mFrustumGeometries;
        };

        osg::ref_ptr<DebugHUD>                  _debugHud;
        std::array<osg::ref_ptr<osg::Program>, GL_ALWAYS - GL_NEVER + 1> _castingPrograms;
        const std::string _shadowsBinName = "ShadowsBin_" + std::to_string(reinterpret_cast<std::uint64_t>(this));
        osg::ref_ptr<osgUtil::RenderBin> _shadowsBin;
        osg::ref_ptr<osg::StateSet> _shadowsBinStateSet;
    };

}

// NOLINTEND(readability-identifier-naming)

#endif
// clang-format on
