#include "shadow.hpp"

#include <osgShadow/ShadowedScene>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/io_utils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

namespace MWRender
{
    using namespace osgShadow;

    std::string debugVertexShaderSource = "void main(void){gl_Position = gl_Vertex; gl_TexCoord[0]=gl_MultiTexCoord0;}";
    std::string debugFragmentShaderSource =
        "uniform sampler2D texture;                                              \n"
        "                                                                        \n"
        "void main(void)                                                         \n"
        "{                                                                       \n"
#if 1
        "    float f = texture2D( texture, gl_TexCoord[0] ).r;                   \n"
        "                                                                        \n"
        "    f = 256.0 * f;                                                      \n"
        "    float fC = floor( f ) / 256.0;                                      \n"
        "                                                                        \n"
        "    f = 256.0 * fract( f );                                             \n"
        "    float fS = floor( f ) / 256.0;                                      \n"
        "                                                                        \n"
        "    f = 256.0 * fract( f );                                             \n"
        "    float fH = floor( f ) / 256.0;                                      \n"
        "                                                                        \n"
        "    fS *= 0.5;                                                          \n"
        "    fH = ( fH  * 0.34 + 0.66 ) * ( 1.0 - fS );                          \n"
        "                                                                        \n"
        "    vec3 rgb = vec3( ( fC > 0.5 ? ( 1.0 - fC ) : fC ),                  \n"
        "                     abs( fC - 0.333333 ),                              \n"
        "                     abs( fC - 0.666667 ) );                            \n"
        "                                                                        \n"
        "    rgb = min( vec3( 1.0, 1.0, 1.0 ), 3.0 * rgb );                      \n"
        "                                                                        \n"
        "    float fMax = max( max( rgb.r, rgb.g ), rgb.b );                     \n"
        "    fMax = 1.0 / fMax;                                                  \n"
        "                                                                        \n"
        "    vec3 color = fMax * rgb;                                            \n"
        "                                                                        \n"
        "    gl_FragColor =  vec4( fS + fH * color, 1 );                         \n"
#else
        "    gl_FragColor = texture2D(texture, gl_TexCoord[0]);                  \n"
        "    //gl_FragColor = vec4(1.0, 0.5, 0.5, 1.0);                            \n"
#endif
        "}                                                                       \n";


    MWShadow::MWShadow() : debugCamera(new osg::Camera), debugProgram(new osg::Program), testTex(new osg::Texture2D)
    {
        debugCamera->setViewport(0, 0, 200, 200);
        debugCamera->setRenderOrder(osg::Camera::POST_RENDER);
        debugCamera->setClearColor(osg::Vec4(1.0, 1.0, 0.0, 1.0));

        osg::ref_ptr<osg::Shader> vertexShader = new osg::Shader(osg::Shader::VERTEX, debugVertexShaderSource);
        debugProgram->addShader(vertexShader);
        osg::ref_ptr<osg::Shader> fragmentShader = new osg::Shader(osg::Shader::FRAGMENT, debugFragmentShaderSource);
        debugProgram->addShader(fragmentShader);

        debugGeometry = osg::createTexturedQuadGeometry(osg::Vec3(-1, -1, 0), osg::Vec3(2, 0, 0), osg::Vec3(0, 2, 0));
        debugCamera->addChild(debugGeometry);
        osg::ref_ptr<osg::StateSet> stateSet = debugGeometry->getOrCreateStateSet();
        stateSet->setAttributeAndModes(debugProgram, osg::StateAttribute::ON);
        osg::ref_ptr<osg::Uniform> textureUniform = new osg::Uniform("texture", 0);
        //textureUniform->setType(osg::Uniform::SAMPLER_2D);
        stateSet->addUniform(textureUniform.get());

        testTex->setDataVariance(osg::Object::DYNAMIC);
        osg::ref_ptr<osg::Image> testImage = osgDB::readRefImageFile("resources/mygui/openmw.png");
        testTex->setImage(testImage);
    }
    
    class VDSMCameraCullCallback : public osg::NodeCallback
    {
    public:

        VDSMCameraCullCallback(ViewDependentShadowMap* vdsm, osg::Polytope& polytope);

        virtual void operator()(osg::Node*, osg::NodeVisitor* nv);

        osg::RefMatrix* getProjectionMatrix() { return _projectionMatrix.get(); }
        osgUtil::RenderStage* getRenderStage() { return _renderStage.get(); }

    protected:

        ViewDependentShadowMap*                 _vdsm;
        osg::ref_ptr<osg::RefMatrix>            _projectionMatrix;
        osg::ref_ptr<osgUtil::RenderStage>      _renderStage;
        osg::Polytope                           _polytope;
    };

    VDSMCameraCullCallback::VDSMCameraCullCallback(ViewDependentShadowMap* vdsm, osg::Polytope& polytope) :
        _vdsm(vdsm),
        _polytope(polytope)
    {
    }

    void VDSMCameraCullCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        osg::Camera* camera = dynamic_cast<osg::Camera*>(node);
        OSG_INFO << "VDSMCameraCullCallback::operator()(osg::Node* " << camera << ", osg::NodeVisitor* " << cv << ")" << std::endl;

#if 1
        if (!_polytope.empty())
        {
            OSG_INFO << "Pushing custom Polytope" << std::endl;

            osg::CullingSet& cs = cv->getProjectionCullingStack().back();

            cs.setFrustum(_polytope);

            cv->pushCullingSet();
        }
#endif
        if (_vdsm->getShadowedScene())
        {
            _vdsm->getShadowedScene()->osg::Group::traverse(*nv);
        }
#if 1
        if (!_polytope.empty())
        {
            OSG_INFO << "Popping custom Polytope" << std::endl;
            cv->popCullingSet();
        }
#endif

        _renderStage = cv->getCurrentRenderBin()->getStage();

        OSG_INFO << "VDSM second : _renderStage = " << _renderStage << std::endl;

        if (cv->getComputeNearFarMode() != osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR)
        {
            // make sure that the near plane is computed correctly.
            cv->computeNearPlane();

            osg::Matrixd projection = *(cv->getProjectionMatrix());

            OSG_INFO << "RTT Projection matrix " << projection << std::endl;

            osg::Matrix::value_type left, right, bottom, top, zNear, zFar;
            osg::Matrix::value_type epsilon = 1e-6;
            if (fabs(projection(0, 3))<epsilon  && fabs(projection(1, 3))<epsilon  && fabs(projection(2, 3))<epsilon)
            {
                projection.getOrtho(left, right,
                    bottom, top,
                    zNear, zFar);

                OSG_INFO << "Ortho zNear=" << zNear << ", zFar=" << zFar << std::endl;
            }
            else
            {
                projection.getFrustum(left, right,
                    bottom, top,
                    zNear, zFar);

                OSG_INFO << "Frustum zNear=" << zNear << ", zFar=" << zFar << std::endl;
            }

            OSG_INFO << "Calculated zNear = " << cv->getCalculatedNearPlane() << ", zFar = " << cv->getCalculatedFarPlane() << std::endl;

            zNear = osg::maximum(zNear, cv->getCalculatedNearPlane());
            zFar = osg::minimum(zFar, cv->getCalculatedFarPlane());

            cv->setCalculatedNearPlane(zNear);
            cv->setCalculatedFarPlane(zFar);

            cv->clampProjectionMatrix(projection, zNear, zFar);

            //OSG_INFO<<"RTT zNear = "<<zNear<<", zFar = "<<zFar<<std::endl;
            OSG_INFO << "RTT Projection matrix after clamping " << projection << std::endl;

            camera->setProjectionMatrix(projection);

            _projectionMatrix = cv->getProjectionMatrix();
        }
    }

    class ComputeLightSpaceBounds : public osg::NodeVisitor, public osg::CullStack
    {
    public:
        ComputeLightSpaceBounds(osg::Viewport* viewport, const osg::Matrixd& projectionMatrix, osg::Matrixd& viewMatrix) :
            osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)
        {
            setCullingMode(osg::CullSettings::VIEW_FRUSTUM_CULLING);

            pushViewport(viewport);
            pushProjectionMatrix(new osg::RefMatrix(projectionMatrix));
            pushModelViewMatrix(new osg::RefMatrix(viewMatrix), osg::Transform::ABSOLUTE_RF);
        }

        void apply(osg::Node& node)
        {
            if (isCulled(node)) return;

            // push the culling mode.
            pushCurrentMask();

            traverse(node);

            // pop the culling mode.
            popCurrentMask();
        }

        void apply(osg::Geode& node)
        {
            if (isCulled(node)) return;

            // push the culling mode.
            pushCurrentMask();

            for (unsigned int i = 0; i<node.getNumDrawables(); ++i)
            {
                if (node.getDrawable(i))
                {
                    updateBound(node.getDrawable(i)->getBoundingBox());
                }
            }

            // pop the culling mode.
            popCurrentMask();
        }

        void apply(osg::Billboard&)
        {
            OSG_INFO << "Warning Billboards not yet supported" << std::endl;
            return;
        }

        void apply(osg::Projection&)
        {
            // projection nodes won't affect a shadow map so their subgraphs should be ignored
            return;
        }

        void apply(osg::Transform& transform)
        {
            if (isCulled(transform)) return;

            // push the culling mode.
            pushCurrentMask();

            // absolute transforms won't affect a shadow map so their subgraphs should be ignored.
            if (transform.getReferenceFrame() == osg::Transform::RELATIVE_RF)
            {
                osg::ref_ptr<osg::RefMatrix> matrix = new osg::RefMatrix(*getModelViewMatrix());
                transform.computeLocalToWorldMatrix(*matrix, this);
                pushModelViewMatrix(matrix.get(), transform.getReferenceFrame());

                traverse(transform);

                popModelViewMatrix();
            }

            // pop the culling mode.
            popCurrentMask();

        }

        void apply(osg::Camera&)
        {
            // camera nodes won't affect a shadow map so their subgraphs should be ignored
            return;
        }

        void updateBound(const osg::BoundingBox& bb)
        {
            if (!bb.valid()) return;

            const osg::Matrix& matrix = *getModelViewMatrix() * *getProjectionMatrix();

            update(bb.corner(0) * matrix);
            update(bb.corner(1) * matrix);
            update(bb.corner(2) * matrix);
            update(bb.corner(3) * matrix);
            update(bb.corner(4) * matrix);
            update(bb.corner(5) * matrix);
            update(bb.corner(6) * matrix);
            update(bb.corner(7) * matrix);
        }

        void update(const osg::Vec3& v)
        {
            if (v.z()<-1.0f)
            {
                //OSG_NOTICE<<"discarding("<<v<<")"<<std::endl;
                return;
            }
            float x = v.x();
            if (x<-1.0f) x = -1.0f;
            if (x>1.0f) x = 1.0f;
            float y = v.y();
            if (y<-1.0f) y = -1.0f;
            if (y>1.0f) y = 1.0f;
            _bb.expandBy(osg::Vec3(x, y, v.z()));
        }

        osg::BoundingBox _bb;
    };
    
    void MWShadow::cull(osgUtil::CullVisitor& cv)
    {
        OSG_INFO << std::endl << std::endl << "ViewDependentShadowMap::cull(osg::CullVisitor&" << &cv << ")" << std::endl;

        if (!_shadowCastingStateSet)
        {
            OSG_INFO << "Warning, init() has not yet been called so ShadowCastingStateSet has not been setup yet, unable to create shadows." << std::endl;
            _shadowedScene->osg::Group::traverse(cv);
            return;
        }

        ViewDependentData* vdd = getViewDependentData(&cv);

        if (!vdd)
        {
            OSG_INFO << "Warning, now ViewDependentData created, unable to create shadows." << std::endl;
            _shadowedScene->osg::Group::traverse(cv);
            return;
        }

        ShadowSettings* settings = getShadowedScene()->getShadowSettings();

        OSG_INFO << "cv->getProjectionMatrix()=" << *cv.getProjectionMatrix() << std::endl;

        osg::CullSettings::ComputeNearFarMode cachedNearFarMode = cv.getComputeNearFarMode();

        osg::RefMatrix& viewProjectionMatrix = *cv.getProjectionMatrix();

        // check whether this main views projection is perspective or orthographic
        bool orthographicViewFrustum = viewProjectionMatrix(0, 3) == 0.0 &&
            viewProjectionMatrix(1, 3) == 0.0 &&
            viewProjectionMatrix(2, 3) == 0.0;

        double minZNear = 0.0;
        double maxZFar = DBL_MAX;

        if (cachedNearFarMode == osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR)
        {
            double left, right, top, bottom;
            if (orthographicViewFrustum)
            {
                viewProjectionMatrix.getOrtho(left, right, bottom, top, minZNear, maxZFar);
            }
            else
            {
                viewProjectionMatrix.getFrustum(left, right, bottom, top, minZNear, maxZFar);
            }
            OSG_INFO << "minZNear=" << minZNear << ", maxZFar=" << maxZFar << std::endl;
        }

        // set the compute near/far mode to the highest quality setting to ensure we push the near plan out as far as possible
        if (settings->getComputeNearFarModeOverride() != osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR)
        {
            cv.setComputeNearFarMode(settings->getComputeNearFarModeOverride());
        }

        // 1. Traverse main scene graph
        cv.pushStateSet(_shadowRecievingPlaceholderStateSet.get());

        osg::ref_ptr<osgUtil::StateGraph> decoratorStateGraph = cv.getCurrentStateGraph();

        cullShadowReceivingScene(&cv);

        cv.popStateSet();

        if (cv.getComputeNearFarMode() != osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR)
        {
            OSG_INFO << "Just done main subgraph traversak" << std::endl;
            // make sure that the near plane is computed correctly so that any projection matrix computations
            // are all done correctly.
            cv.computeNearPlane();
        }

        // clamp the minZNear and maxZFar to those provided by ShadowSettings
        maxZFar = osg::minimum(settings->getMaximumShadowMapDistance(), maxZFar);
        if (minZNear>maxZFar) minZNear = maxZFar*settings->getMinimumShadowMapNearFarRatio();

        //OSG_NOTICE<<"maxZFar "<<maxZFar<<std::endl;

        Frustum frustum(&cv, minZNear, maxZFar);

        // return compute near far mode back to it's original settings
        cv.setComputeNearFarMode(cachedNearFarMode);

        OSG_INFO << "frustum.eye=" << frustum.eye << ", frustum.centerNearPlane, " << frustum.centerNearPlane << " distance = " << (frustum.eye - frustum.centerNearPlane).length() << std::endl;


        // 2. select active light sources
        //    create a list of light sources + their matrices to place them
        selectActiveLights(&cv, vdd);


        unsigned int pos_x = 0;
        unsigned int textureUnit = settings->getBaseShadowTextureUnit();
        unsigned int numValidShadows = 0;

        ShadowDataList& sdl = vdd->getShadowDataList();
        ShadowDataList previous_sdl;
        previous_sdl.swap(sdl);

        unsigned int numShadowMapsPerLight = settings->getNumShadowMapsPerLight();
        if (numShadowMapsPerLight>2)
        {
            OSG_NOTICE << "numShadowMapsPerLight of " << numShadowMapsPerLight << " is greater than maximum supported, falling back to 2." << std::endl;
            numShadowMapsPerLight = 2;
        }

        LightDataList& pll = vdd->getLightDataList();
        for (LightDataList::iterator itr = pll.begin();
            itr != pll.end();
            ++itr)
        {
            // 3. create per light/per shadow map division of lightspace/frustum
            //    create a list of light/shadow map data structures

            LightData& pl = **itr;

            // 3.1 compute light space polytope
            //
            osg::Polytope polytope = computeLightViewFrustumPolytope(frustum, pl);

            // if polytope is empty then no rendering.
            if (polytope.empty())
            {
                OSG_NOTICE << "Polytope empty no shadow to render" << std::endl;
                continue;
            }

            // 3.2 compute RTT camera view+projection matrix settings
            //
            osg::Matrixd projectionMatrix;
            osg::Matrixd viewMatrix;
            if (!computeShadowCameraSettings(frustum, pl, projectionMatrix, viewMatrix))
            {
                OSG_NOTICE << "No valid Camera settings, no shadow to render" << std::endl;
                continue;
            }

            // if we are using multiple shadow maps and CastShadowTraversalMask is being used
            // traverse the scene to compute the extents of the objects
            if (/*numShadowMapsPerLight>1 &&*/ _shadowedScene->getCastsShadowTraversalMask() != 0xffffffff)
            {
                // osg::ElapsedTime timer;

                osg::ref_ptr<osg::Viewport> viewport = new osg::Viewport(0, 0, 2048, 2048);
                ComputeLightSpaceBounds clsb(viewport.get(), projectionMatrix, viewMatrix);
                clsb.setTraversalMask(_shadowedScene->getCastsShadowTraversalMask());

                osg::Matrixd invertModelView;
                invertModelView.invert(viewMatrix);
                osg::Polytope local_polytope(polytope);
                local_polytope.transformProvidingInverse(invertModelView);

                osg::CullingSet& cs = clsb.getProjectionCullingStack().back();
                cs.setFrustum(local_polytope);
                clsb.pushCullingSet();

                _shadowedScene->accept(clsb);

                // OSG_NOTICE<<"Extents of LightSpace "<<clsb._bb.xMin()<<", "<<clsb._bb.xMax()<<", "<<clsb._bb.yMin()<<", "<<clsb._bb.yMax()<<", "<<clsb._bb.zMin()<<", "<<clsb._bb.zMax()<<std::endl;
                // OSG_NOTICE<<"  time "<<timer.elapsedTime_m()<<"ms, mask = "<<std::hex<<_shadowedScene->getCastsShadowTraversalMask()<<std::endl;

                if (clsb._bb.xMin()>-1.0f || clsb._bb.xMax()<1.0f || clsb._bb.yMin()>-1.0f || clsb._bb.yMax()<1.0f)
                {
                    // OSG_NOTICE<<"Need to clamp projection matrix"<<std::endl;

#if 1
                    double xMid = (clsb._bb.xMin() + clsb._bb.xMax())*0.5f;
                    double xRange = clsb._bb.xMax() - clsb._bb.xMin();
#else
                    double xMid = 0.0;
                    double xRange = 2.0;
#endif
                    double yMid = (clsb._bb.yMin() + clsb._bb.yMax())*0.5f;
                    double yRange = (clsb._bb.yMax() - clsb._bb.yMin());

                    // OSG_NOTICE<<"  xMid="<<xMid<<", yMid="<<yMid<<", xRange="<<xRange<<", yRange="<<yRange<<std::endl;

                    projectionMatrix =
                        projectionMatrix *
                        osg::Matrixd::translate(osg::Vec3d(-xMid, -yMid, 0.0)) *
                        osg::Matrixd::scale(osg::Vec3d(2.0 / xRange, 2.0 / yRange, 1.0));

                }

            }

            double splitPoint = 0.0;

            if (numShadowMapsPerLight>1)
            {
                osg::Vec3d eye_v = frustum.eye * viewMatrix;
                osg::Vec3d center_v = frustum.center * viewMatrix;
                osg::Vec3d viewdir_v = center_v - eye_v; viewdir_v.normalize();
                osg::Vec3d lightdir(0.0, 0.0, -1.0);

                double dotProduct_v = lightdir * viewdir_v;
                double angle = acosf(dotProduct_v);

                osg::Vec3d eye_ls = eye_v * projectionMatrix;

                OSG_INFO << "Angle between view vector and eye " << osg::RadiansToDegrees(angle) << std::endl;
                OSG_INFO << "eye_ls=" << eye_ls << std::endl;

                if (eye_ls.y() >= -1.0 && eye_ls.y() <= 1.0)
                {
                    OSG_INFO << "Eye point inside light space clip region   " << std::endl;
                    splitPoint = 0.0;
                }
                else
                {
                    double n = -1.0 - eye_ls.y();
                    double f = 1.0 - eye_ls.y();
                    double sqrt_nf = sqrt(n*f);
                    double mid = eye_ls.y() + sqrt_nf;
                    double ratioOfMidToUseForSplit = 0.8;
                    splitPoint = mid * ratioOfMidToUseForSplit;

                    OSG_INFO << "  n=" << n << ", f=" << f << ", sqrt_nf=" << sqrt_nf << " mid=" << mid << std::endl;
                }
            }

            // 4. For each light/shadow map
            for (unsigned int sm_i = 0; sm_i<numShadowMapsPerLight; ++sm_i)
            {
                osg::ref_ptr<ShadowData> sd;

                if (previous_sdl.empty())
                {
                    OSG_INFO << "Create new ShadowData" << std::endl;
                    sd = new ShadowData(vdd);
                }
                else
                {
                    OSG_INFO << "Taking ShadowData from from of previous_sdl" << std::endl;
                    sd = previous_sdl.front();
                    previous_sdl.erase(previous_sdl.begin());
                }

                if (true)
                {
                    osg::ref_ptr<osg::Texture2D> texture = sd->_texture;
                    osg::ref_ptr<osg::StateSet> stateSet = debugGeometry->getOrCreateStateSet();
                    if (false)
                        stateSet->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
                    else
                        stateSet->setTextureAttributeAndModes(0, testTex, osg::StateAttribute::ON);

                    unsigned int traversalMask = cv.getTraversalMask();
                    cv.setTraversalMask(debugGeometry->getNodeMask());
                    cv.pushStateSet(stateSet);
                    debugCamera->accept(cv);
                    cv.popStateSet();
                    cv.setTraversalMask(traversalMask);

                    cv.getState()->setCheckForGLErrors(osg::State::ONCE_PER_ATTRIBUTE);
                }

                osg::ref_ptr<osg::Camera> camera = sd->_camera;

                camera->setProjectionMatrix(projectionMatrix);
                camera->setViewMatrix(viewMatrix);

                if (settings->getDebugDraw())
                {
                    camera->getViewport()->x() = pos_x;
                    pos_x += static_cast<unsigned int>(camera->getViewport()->width()) + 40;
                }

                // transform polytope in model coords into light spaces eye coords.
                osg::Matrixd invertModelView;
                invertModelView.invert(camera->getViewMatrix());

                osg::Polytope local_polytope(polytope);
                local_polytope.transformProvidingInverse(invertModelView);


                if (numShadowMapsPerLight>1)
                {
                    // compute the start and end range in non-dimensional coords
#if 0
                    double r_start = (sm_i == 0) ? -1.0 : (double(sm_i) / double(numShadowMapsPerLight)*2.0 - 1.0);
                    double r_end = (sm_i + 1 == numShadowMapsPerLight) ? 1.0 : (double(sm_i + 1) / double(numShadowMapsPerLight)*2.0 - 1.0);
#endif

                    // hardwired for 2 splits
                    double r_start = (sm_i == 0) ? -1.0 : splitPoint;
                    double r_end = (sm_i + 1 == numShadowMapsPerLight) ? 1.0 : splitPoint;

                    // for all by the last shadowmap shift the r_end so that it overlaps slightly with the next shadowmap
                    // to prevent a seam showing through between the shadowmaps
                    if (sm_i + 1<numShadowMapsPerLight) r_end += 0.01;


                    if (sm_i>0)
                    {
                        // not the first shadowmap so insert a polytope to clip the scene from before r_start

                        // plane in clip space coords
                        osg::Plane plane(0.0, 1.0, 0.0, -r_start);

                        // transform into eye coords
                        plane.transformProvidingInverse(projectionMatrix);
                        local_polytope.getPlaneList().push_back(plane);

                        //OSG_NOTICE<<"Adding r_start plane "<<plane<<std::endl;

                    }

                    if (sm_i + 1<numShadowMapsPerLight)
                    {
                        // not the last shadowmap so insert a polytope to clip the scene from beyond r_end

                        // plane in clip space coords
                        osg::Plane plane(0.0, -1.0, 0.0, r_end);

                        // transform into eye coords
                        plane.transformProvidingInverse(projectionMatrix);
                        local_polytope.getPlaneList().push_back(plane);

                        //OSG_NOTICE<<"Adding r_end plane "<<plane<<std::endl;
                    }

                    local_polytope.setupMask();


                    // OSG_NOTICE<<"Need to adjust RTT camera projection and view matrix here, r_start="<<r_start<<", r_end="<<r_end<<std::endl;
                    // OSG_NOTICE<<"  textureUnit = "<<textureUnit<<std::endl;

                    double mid_r = (r_start + r_end)*0.5;
                    double range_r = (r_end - r_start);

                    // OSG_NOTICE<<"  mid_r = "<<mid_r<<", range_r = "<<range_r<<std::endl;

                    camera->setProjectionMatrix(
                        camera->getProjectionMatrix() *
                        osg::Matrixd::translate(osg::Vec3d(0.0, -mid_r, 0.0)) *
                        osg::Matrixd::scale(osg::Vec3d(1.0, 2.0 / range_r, 1.0)));

                }


                osg::ref_ptr<VDSMCameraCullCallback> vdsmCallback = new VDSMCameraCullCallback(this, local_polytope);
                camera->setCullCallback(vdsmCallback.get());

                // 4.3 traverse RTT camera
                //

                cv.pushStateSet(_shadowCastingStateSet.get());

                cullShadowCastingScene(&cv, camera.get());

                cv.popStateSet();

                if (!orthographicViewFrustum && settings->getShadowMapProjectionHint() == osgShadow::ShadowSettings::PERSPECTIVE_SHADOW_MAP)
                {
                    adjustPerspectiveShadowMapCameraSettings(vdsmCallback->getRenderStage(), frustum, pl, camera.get());
                    if (vdsmCallback->getProjectionMatrix())
                    {
                        vdsmCallback->getProjectionMatrix()->set(camera->getProjectionMatrix());
                    }
                }

                // 4.4 compute main scene graph TexGen + uniform settings + setup state
                //
                assignTexGenSettings(&cv, camera.get(), textureUnit, sd->_texgen.get());

                // mark the light as one that has active shadows and requires shaders
                pl.textureUnits.push_back(textureUnit);

                // pass on shadow data to ShadowDataList
                sd->_textureUnit = textureUnit;

                if (textureUnit >= 8)
                {
                    OSG_NOTICE << "Shadow texture unit is invalid for texgen, will not be used." << std::endl;
                }
                else
                {
                    sdl.push_back(sd);
                }

                // increment counters.
                ++textureUnit;
                ++numValidShadows;
            }
        }

        if (numValidShadows>0)
        {
            decoratorStateGraph->setStateSet(selectStateSetForRenderingShadow(*vdd));
        }

        // OSG_NOTICE<<"End of shadow setup Projection matrix "<<*cv.getProjectionMatrix()<<std::endl;
    }
}
