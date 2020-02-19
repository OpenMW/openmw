#include "controller.hpp"

#include <osg/MatrixTransform>
#include <osg/TexMat>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/UserDataContainer>

#include <osgParticle/Emitter>

#include <components/nif/data.hpp>
#include <components/sceneutil/morphgeometry.hpp>
#include <components/sceneutil/vismask.hpp>

#include "userdata.hpp"

namespace NifOsg
{

ControllerFunction::ControllerFunction(const Nif::Controller *ctrl)
    : mFrequency(ctrl->frequency)
    , mPhase(ctrl->phase)
    , mStartTime(ctrl->timeStart)
    , mStopTime(ctrl->timeStop)
    , mExtrapolationMode(static_cast<ExtrapolationMode>((ctrl->flags&0x6) >> 1))
{
}

float ControllerFunction::calculate(float value) const
{
    float time = mFrequency * value + mPhase;
    if (time >= mStartTime && time <= mStopTime)
        return time;
    switch (mExtrapolationMode)
    {
    case Cycle:
    {
        float delta = mStopTime - mStartTime;
        if ( delta <= 0 )
            return mStartTime;
        float cycles = ( time - mStartTime ) / delta;
        float remainder = ( cycles - std::floor( cycles ) ) * delta;
        return mStartTime + remainder;
    }
    case Reverse:
    {
        float delta = mStopTime - mStartTime;
        if ( delta <= 0 )
            return mStartTime;

        float cycles = ( time - mStartTime ) / delta;
        float remainder = ( cycles - std::floor( cycles ) ) * delta;

        // Even number of cycles?
        if ( ( static_cast<int>(std::fabs( std::floor( cycles ) )) % 2 ) == 0 )
            return mStartTime + remainder;

        return mStopTime - remainder;
    }
    case Constant:
    default:
        return std::min(mStopTime, std::max(mStartTime, time));
    }
}

float ControllerFunction::getMaximum() const
{
    return mStopTime;
}

KeyframeController::KeyframeController()
{
}

KeyframeController::KeyframeController(const KeyframeController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop)
    , Controller(copy)
    , mRotations(copy.mRotations)
    , mXRotations(copy.mXRotations)
    , mYRotations(copy.mYRotations)
    , mZRotations(copy.mZRotations)
    , mTranslations(copy.mTranslations)
    , mScales(copy.mScales)
{
}

KeyframeController::KeyframeController(const Nif::NiKeyframeData *data)
    : mRotations(data->mRotations)
    , mXRotations(data->mXRotations, 0.f)
    , mYRotations(data->mYRotations, 0.f)
    , mZRotations(data->mZRotations, 0.f)
    , mTranslations(data->mTranslations, osg::Vec3f())
    , mScales(data->mScales, 1.f)
{
}

osg::Quat KeyframeController::getXYZRotation(float time) const
{
    float xrot = 0, yrot = 0, zrot = 0;
    if (!mXRotations.empty())
        xrot = mXRotations.interpKey(time);
    if (!mYRotations.empty())
        yrot = mYRotations.interpKey(time);
    if (!mZRotations.empty())
        zrot = mZRotations.interpKey(time);
    osg::Quat xr(xrot, osg::Vec3f(1,0,0));
    osg::Quat yr(yrot, osg::Vec3f(0,1,0));
    osg::Quat zr(zrot, osg::Vec3f(0,0,1));
    return (xr*yr*zr);
}

osg::Vec3f KeyframeController::getTranslation(float time) const
{
    if(!mTranslations.empty())
        return mTranslations.interpKey(time);
    return osg::Vec3f();
}

void KeyframeController::operator() (osg::Node* node, osg::NodeVisitor* nv)
{
    if (hasInput())
    {
        osg::MatrixTransform* trans = static_cast<osg::MatrixTransform*>(node);
        osg::Matrix mat = trans->getMatrix();

        float time = getInputValue(nv);

        NodeUserData* userdata = static_cast<NodeUserData*>(trans->getUserDataContainer()->getUserObject(0));
        Nif::Matrix3& rot = userdata->mRotationScale;

        bool setRot = false;
        if(!mRotations.empty())
        {
            mat.setRotate(mRotations.interpKey(time));
            setRot = true;
        }
        else if (!mXRotations.empty() || !mYRotations.empty() || !mZRotations.empty())
        {
            mat.setRotate(getXYZRotation(time));
            setRot = true;
        }
        else
        {
            // no rotation specified, use the previous value from the UserData
            for (int i=0;i<3;++i)
                for (int j=0;j<3;++j)
                    mat(j,i) = rot.mValues[i][j]; // NB column/row major difference
        }

        if (setRot) // copy the new values back to the UserData
            for (int i=0;i<3;++i)
                for (int j=0;j<3;++j)
                    rot.mValues[i][j] = mat(j,i); // NB column/row major difference

        float& scale = userdata->mScale;
        if(!mScales.empty())
            scale = mScales.interpKey(time);

        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                mat(i,j) *= scale;

        if(!mTranslations.empty())
            mat.setTrans(mTranslations.interpKey(time));

        trans->setMatrix(mat);
    }

    traverse(node, nv);
}

GeomMorpherController::GeomMorpherController()
{
}

GeomMorpherController::GeomMorpherController(const GeomMorpherController &copy, const osg::CopyOp &copyop)
    : osg::Drawable::UpdateCallback(copy, copyop)
    , Controller(copy)
    , mKeyFrames(copy.mKeyFrames)
{
}

GeomMorpherController::GeomMorpherController(const Nif::NiMorphData *data)
{
    for (unsigned int i=0; i<data->mMorphs.size(); ++i)
        mKeyFrames.push_back(FloatInterpolator(data->mMorphs[i].mKeyFrames));
}

void GeomMorpherController::update(osg::NodeVisitor *nv, osg::Drawable *drawable)
{
    SceneUtil::MorphGeometry* morphGeom = static_cast<SceneUtil::MorphGeometry*>(drawable);
    if (hasInput())
    {
        if (mKeyFrames.size() <= 1)
            return;
        float input = getInputValue(nv);
        int i = 0;
        for (std::vector<FloatInterpolator>::iterator it = mKeyFrames.begin()+1; it != mKeyFrames.end(); ++it,++i)
        {
            float val = 0;
            if (!(*it).empty())
                val = it->interpKey(input);
            val = std::max(0.f, std::min(1.f, val));

            SceneUtil::MorphGeometry::MorphTarget& target = morphGeom->getMorphTarget(i);
            if (target.getWeight() != val)
            {
                target.setWeight(val);
                morphGeom->dirty();
            }
        }
    }
}

UVController::UVController()
{
}

UVController::UVController(const Nif::NiUVData *data, const std::set<int>& textureUnits)
    : mUTrans(data->mKeyList[0], 0.f)
    , mVTrans(data->mKeyList[1], 0.f)
    , mUScale(data->mKeyList[2], 1.f)
    , mVScale(data->mKeyList[3], 1.f)
    , mTextureUnits(textureUnits)
{
}

UVController::UVController(const UVController& copy, const osg::CopyOp& copyop)
    : osg::Object(copy, copyop), StateSetUpdater(copy, copyop), Controller(copy)
    , mUTrans(copy.mUTrans)
    , mVTrans(copy.mVTrans)
    , mUScale(copy.mUScale)
    , mVScale(copy.mVScale)
    , mTextureUnits(copy.mTextureUnits)
{
}

void UVController::setDefaults(osg::StateSet *stateset)
{
    osg::ref_ptr<osg::TexMat> texMat (new osg::TexMat);
    for (std::set<int>::const_iterator it = mTextureUnits.begin(); it != mTextureUnits.end(); ++it)
        stateset->setTextureAttributeAndModes(*it, texMat, osg::StateAttribute::ON);
}

void UVController::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
{
    if (hasInput())
    {
        float value = getInputValue(nv);
        float uTrans = mUTrans.interpKey(value);
        float vTrans = mVTrans.interpKey(value);
        float uScale = mUScale.interpKey(value);
        float vScale = mVScale.interpKey(value);

        osg::Matrix flipMat;
        flipMat.preMultTranslate(osg::Vec3f(0,1,0));
        flipMat.preMultScale(osg::Vec3f(1,-1,1));

        osg::Matrixf mat = osg::Matrixf::scale(uScale, vScale, 1);
        mat.setTrans(uTrans, vTrans, 0);

        mat = flipMat * mat * flipMat;

        // setting once is enough because all other texture units share the same TexMat (see setDefaults).
        if (!mTextureUnits.empty())
        {
            osg::TexMat* texMat = static_cast<osg::TexMat*>(stateset->getTextureAttribute(*mTextureUnits.begin(), osg::StateAttribute::TEXMAT));
            texMat->setMatrix(mat);
        }
    }
}

VisController::VisController(const Nif::NiVisData *data)
    : mData(data->mVis)
{
}

VisController::VisController()
{
}

VisController::VisController(const VisController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop)
    , Controller(copy)
    , mData(copy.mData)
{
}

bool VisController::calculate(float time) const
{
    if(mData.size() == 0)
        return true;

    for(size_t i = 1;i < mData.size();i++)
    {
        if(mData[i].time > time)
            return mData[i-1].isSet;
    }
    return mData.back().isSet;
}

void VisController::operator() (osg::Node* node, osg::NodeVisitor* nv)
{
    if (hasInput())
    {
        bool vis = calculate(getInputValue(nv));
        // Leave 0x1 enabled for UpdateVisitor, so we can make ourselves visible again in the future from this update callback
        node->setNodeMask(vis ? SceneUtil::Mask_Default : SceneUtil::Mask_UpdateVisitor);
    }
    traverse(node, nv);
}

RollController::RollController(const Nif::NiFloatData *data)
    : mData(data->mKeyList, 1.f)
    , mStartingTime(0)
{
}

RollController::RollController() : mStartingTime(0)
{
}

RollController::RollController(const RollController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop)
    , Controller(copy)
    , mData(copy.mData)
    , mStartingTime(0)
{
}

void RollController::operator() (osg::Node* node, osg::NodeVisitor* nv)
{
    traverse(node, nv);

    if (hasInput())
    {
        double newTime = nv->getFrameStamp()->getSimulationTime();
        double duration = newTime - mStartingTime;
        mStartingTime = newTime;

        float value = mData.interpKey(getInputValue(nv));
        osg::MatrixTransform* transform = static_cast<osg::MatrixTransform*>(node);
        osg::Matrix matrix = transform->getMatrix();

        // Rotate around "roll" axis.
        // Note: in original game rotation speed is the framerate-dependent in a very tricky way.
        // Do not replicate this behaviour until we will really need it.
        // For now consider controller's current value as an angular speed in radians per 1/60 seconds.
        matrix = osg::Matrix::rotate(value * duration * 60.f, 0, 0, 1) * matrix;
        transform->setMatrix(matrix);
    }
}

AlphaController::AlphaController(const Nif::NiFloatData *data)
    : mData(data->mKeyList, 1.f)
{

}

AlphaController::AlphaController()
{
}

AlphaController::AlphaController(const AlphaController &copy, const osg::CopyOp &copyop)
    : StateSetUpdater(copy, copyop), Controller(copy)
    , mData(copy.mData)
{
}

void AlphaController::setDefaults(osg::StateSet *stateset)
{
    // need to create a deep copy of StateAttributes we will modify
    osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
    stateset->setAttribute(osg::clone(mat, osg::CopyOp::DEEP_COPY_ALL), osg::StateAttribute::ON);
}

void AlphaController::apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
{
    if (hasInput())
    {
        float value = mData.interpKey(getInputValue(nv));
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        osg::Vec4f diffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
        diffuse.a() = value;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
    }
}

MaterialColorController::MaterialColorController(const Nif::NiPosData *data, TargetColor color)
    : mData(data->mKeyList, osg::Vec3f(1,1,1))
    , mTargetColor(color)
{
}

MaterialColorController::MaterialColorController()
{
}

MaterialColorController::MaterialColorController(const MaterialColorController &copy, const osg::CopyOp &copyop)
    : StateSetUpdater(copy, copyop), Controller(copy)
    , mData(copy.mData)
    , mTargetColor(copy.mTargetColor)
{
}

void MaterialColorController::setDefaults(osg::StateSet *stateset)
{
    // need to create a deep copy of StateAttributes we will modify
    osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
    stateset->setAttribute(osg::clone(mat, osg::CopyOp::DEEP_COPY_ALL), osg::StateAttribute::ON);
}

void MaterialColorController::apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
{
    if (hasInput())
    {
        osg::Vec3f value = mData.interpKey(getInputValue(nv));
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        switch (mTargetColor)
        {
            case Diffuse:
            {
                osg::Vec4f diffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
                diffuse.set(value.x(), value.y(), value.z(), diffuse.a());
                mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
                break;
            }
            case Specular:
            {
                osg::Vec4f specular = mat->getSpecular(osg::Material::FRONT_AND_BACK);
                specular.set(value.x(), value.y(), value.z(), specular.a());
                mat->setSpecular(osg::Material::FRONT_AND_BACK, specular);
                break;
            }
            case Emissive:
            {
                osg::Vec4f emissive = mat->getEmission(osg::Material::FRONT_AND_BACK);
                emissive.set(value.x(), value.y(), value.z(), emissive.a());
                mat->setEmission(osg::Material::FRONT_AND_BACK, emissive);
                break;
            }
            case Ambient:
            default:
            {
                osg::Vec4f ambient = mat->getAmbient(osg::Material::FRONT_AND_BACK);
                ambient.set(value.x(), value.y(), value.z(), ambient.a());
                mat->setAmbient(osg::Material::FRONT_AND_BACK, ambient);
            }
        }
    }
}

FlipController::FlipController(const Nif::NiFlipController *ctrl, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures)
    : mTexSlot(ctrl->mTexSlot)
    , mDelta(ctrl->mDelta)
    , mTextures(textures)
{
}

FlipController::FlipController(int texSlot, float delta, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures)
    : mTexSlot(texSlot)
    , mDelta(delta)
    , mTextures(textures)
{
}

FlipController::FlipController()
    : mTexSlot(0)
    , mDelta(0.f)
{
}

FlipController::FlipController(const FlipController &copy, const osg::CopyOp &copyop)
    : StateSetUpdater(copy, copyop)
    , Controller(copy)
    , mTexSlot(copy.mTexSlot)
    , mDelta(copy.mDelta)
    , mTextures(copy.mTextures)
{
}

void FlipController::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
{
    if (hasInput() && mDelta != 0 && !mTextures.empty())
    {
        int curTexture = int(getInputValue(nv) / mDelta) % mTextures.size();
        stateset->setTextureAttribute(mTexSlot, mTextures[curTexture]);
    }
}

ParticleSystemController::ParticleSystemController(const Nif::NiParticleSystemController *ctrl)
    : mEmitStart(ctrl->startTime), mEmitStop(ctrl->stopTime)
{
}

ParticleSystemController::ParticleSystemController()
    : mEmitStart(0.f), mEmitStop(0.f)
{
}

ParticleSystemController::ParticleSystemController(const ParticleSystemController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop)
    , Controller(copy)
    , mEmitStart(copy.mEmitStart)
    , mEmitStop(copy.mEmitStop)
{
}

void ParticleSystemController::operator() (osg::Node* node, osg::NodeVisitor* nv)
{
    osgParticle::ParticleProcessor* emitter = static_cast<osgParticle::ParticleProcessor*>(node);
    if (hasInput())
    {
        float time = getInputValue(nv);
        emitter->getParticleSystem()->setFrozen(false);
        emitter->setEnabled(time >= mEmitStart && time < mEmitStop);
    }
    else
        emitter->getParticleSystem()->setFrozen(true);
    traverse(node, nv);
}

}
