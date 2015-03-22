#include "controller.hpp"

#include <osg/MatrixTransform>
#include <osg/TexMat>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/io_utils>
#include <osg/UserDataContainer>

#include <osgAnimation/MorphGeometry>

#include <osgParticle/Emitter>

#include <components/nif/data.hpp>

#include "userdata.hpp"

namespace NifOsg
{

ControllerFunction::ControllerFunction(const Nif::Controller *ctrl, bool deltaInput)
    : mDeltaInput(deltaInput)
    , mFrequency(ctrl->frequency)
    , mPhase(ctrl->phase)
    , mStartTime(ctrl->timeStart)
    , mStopTime(ctrl->timeStop)
    , mDeltaCount(0.f)
{
    if(mDeltaInput)
        mDeltaCount = mPhase;
}

float ControllerFunction::calculate(float value)
{
    if(mDeltaInput)
    {
        if (mStopTime - mStartTime == 0.f)
            return 0.f;

        mDeltaCount += value*mFrequency;
        if(mDeltaCount < mStartTime)
            mDeltaCount = mStopTime - std::fmod(mStartTime - mDeltaCount,
                                                mStopTime - mStartTime);
        mDeltaCount = std::fmod(mDeltaCount - mStartTime,
                                mStopTime - mStartTime) + mStartTime;
        return mDeltaCount;
    }

    value = std::min(mStopTime, std::max(mStartTime, value+mPhase));
    return value;
}

FrameTimeSource::FrameTimeSource()
    : mLastTime(0.0)
{
}

float FrameTimeSource::getValue(osg::NodeVisitor *nv)
{
    // TODO: dt could be computed globally instead of once per instance
    double time = nv->getFrameStamp()->getReferenceTime();
    float dt = static_cast<float>(time - mLastTime);
    mLastTime = time;
    return dt;
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
    , mXRotations(data->mXRotations)
    , mYRotations(data->mYRotations)
    , mZRotations(data->mZRotations)
    , mTranslations(data->mTranslations)
    , mScales(data->mScales)
{
}

osg::Quat KeyframeController::interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time)
{
    if(time <= keys.begin()->first)
        return keys.begin()->second.mValue;

    Nif::QuaternionKeyMap::MapType::const_iterator it = keys.lower_bound(time);
    if (it != keys.end())
    {
        float aTime = it->first;
        const Nif::QuaternionKey* aKey = &it->second;

        assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

        Nif::QuaternionKeyMap::MapType::const_iterator last = --it;
        float aLastTime = last->first;
        const Nif::QuaternionKey* aLastKey = &last->second;

        float a = (time - aLastTime) / (aTime - aLastTime);

        osg::Quat v1 = aLastKey->mValue;
        osg::Quat v2 = aKey->mValue;
        // don't take the long path
        if (v1.x()*v2.x() + v1.y()*v2.y() + v1.z()*v2.z() + v1.w()*v2.w() < 0) // dotProduct(v1,v2)
            v1 = -v1;

        osg::Quat result;
        result.slerp(a, v1, v2);
        return result;
    }
    else
        return keys.rbegin()->second.mValue;
}

osg::Quat KeyframeController::getXYZRotation(float time) const
{
    float xrot = 0, yrot = 0, zrot = 0;
    if (mXRotations.get())
        xrot = interpKey(mXRotations->mKeys, time);
    if (mYRotations.get())
        yrot = interpKey(mYRotations->mKeys, time);
    if (mZRotations.get())
        zrot = interpKey(mZRotations->mKeys, time);
    osg::Quat xr(xrot, osg::Vec3f(1,0,0));
    osg::Quat yr(yrot, osg::Vec3f(0,1,0));
    osg::Quat zr(zrot, osg::Vec3f(0,0,1));
    return (zr*yr*xr);
}

osg::Vec3f KeyframeController::getTranslation(float time) const
{
    if(mTranslations.get() && mTranslations->mKeys.size() > 0)
        return interpKey(mTranslations->mKeys, time);
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
        if(mRotations.get() && !mRotations->mKeys.empty())
        {
            mat.setRotate(interpKey(mRotations->mKeys, time));
            setRot = true;
        }
        else if (mXRotations.get() || mYRotations.get() || mZRotations.get())
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
        if(mScales.get() && !mScales->mKeys.empty())
            scale = interpKey(mScales->mKeys, time);

        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                mat(i,j) *= scale;

        if(mTranslations.get() && !mTranslations->mKeys.empty())
            mat.setTrans(interpKey(mTranslations->mKeys, time));

        trans->setMatrix(mat);
    }

    traverse(node, nv);
}

Controller::Controller()
{
}

bool Controller::hasInput() const
{
    return mSource.get() != NULL;
}

float Controller::getInputValue(osg::NodeVisitor* nv)
{
    return mFunction->calculate(mSource->getValue(nv));
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
        mKeyFrames.push_back(data->mMorphs[i].mKeyFrames);
}

void GeomMorpherController::update(osg::NodeVisitor *nv, osg::Drawable *drawable)
{
    osgAnimation::MorphGeometry* morphGeom = dynamic_cast<osgAnimation::MorphGeometry*>(drawable);
    if (morphGeom)
    {
        if (hasInput())
        {
            if (mKeyFrames.size() <= 1)
                return;
            float input = getInputValue(nv);
            int i = 0;
            for (std::vector<Nif::FloatKeyMapPtr>::iterator it = mKeyFrames.begin()+1; it != mKeyFrames.end(); ++it,++i)
            {
                float val = 0;
                if (!(*it)->mKeys.empty())
                    val = interpKey((*it)->mKeys, input);
                val = std::max(0.f, std::min(1.f, val));

                morphGeom->setWeight(i, val);
            }
        }

        morphGeom->transformSoftwareMethod();
    }
}

UVController::UVController()
{
}

UVController::UVController(const Nif::NiUVData *data, std::set<int> textureUnits)
    : mUTrans(data->mKeyList[0])
    , mVTrans(data->mKeyList[1])
    , mUScale(data->mKeyList[2])
    , mVScale(data->mKeyList[3])
    , mTextureUnits(textureUnits)
{
}

UVController::UVController(const UVController& copy, const osg::CopyOp& copyop)
    : osg::Object(copy, copyop), osg::NodeCallback(copy, copyop), Controller(copy)
    , mUTrans(copy.mUTrans)
    , mVTrans(copy.mVTrans)
    , mUScale(copy.mUScale)
    , mVScale(copy.mVScale)
    , mTextureUnits(copy.mTextureUnits)
{
}

void UVController::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (hasInput())
    {
        osg::StateSet* stateset = node->getStateSet();
        float value = getInputValue(nv);
        float uTrans = interpKey(mUTrans->mKeys, value, 0.0f);
        float vTrans = interpKey(mVTrans->mKeys, value, 0.0f);
        float uScale = interpKey(mUScale->mKeys, value, 1.0f);
        float vScale = interpKey(mVScale->mKeys, value, 1.0f);

        osg::Matrixf mat = osg::Matrixf::scale(uScale, vScale, 1);
        mat.setTrans(uTrans, vTrans, 0);

        osg::TexMat* texMat = new osg::TexMat;
        texMat->setMatrix(mat);

        for (std::set<int>::const_iterator it = mTextureUnits.begin(); it != mTextureUnits.end(); ++it)
        {
            stateset->setTextureAttributeAndModes(*it, texMat, osg::StateAttribute::ON);
        }
    }
    traverse(node, nv);
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
        node->setNodeMask(vis ? ~0 : 0x1);
    }
    traverse(node, nv);
}

AlphaController::AlphaController(const Nif::NiFloatData *data)
    : mData(data->mKeyList)
{

}

AlphaController::AlphaController()
{
}

AlphaController::AlphaController(const AlphaController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop), ValueInterpolator(), Controller(copy)
{
}

void AlphaController::operator () (osg::Node* node, osg::NodeVisitor* nv)
{
    if (hasInput())
    {
        osg::StateSet* stateset = node->getStateSet();
        float value = interpKey(mData->mKeys, getInputValue(nv));
        osg::Material* mat = dynamic_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        if (mat)
        {
            osg::Vec4f diffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
            diffuse.a() = value;
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
        }
    }
    traverse(node, nv);
}

MaterialColorController::MaterialColorController(const Nif::NiPosData *data)
    : mData(data->mKeyList)
{
}

MaterialColorController::MaterialColorController()
{
}

MaterialColorController::MaterialColorController(const MaterialColorController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop), Controller(copy)
    , mData(copy.mData)
{
}

void MaterialColorController::operator() (osg::Node* node, osg::NodeVisitor* nv)
{
    if (hasInput())
    {
        osg::StateSet* stateset = node->getStateSet();
        osg::Vec3f value = interpKey(mData->mKeys, getInputValue(nv));
        osg::Material* mat = dynamic_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        if (mat)
        {
            osg::Vec4f diffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
            diffuse.set(value.x(), value.y(), value.z(), diffuse.a());
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
        }
    }
    traverse(node, nv);
}

FlipController::FlipController(const Nif::NiFlipController *ctrl, std::vector<osg::ref_ptr<osg::Image> > textures)
    : mTexSlot(ctrl->mTexSlot)
    , mDelta(ctrl->mDelta)
    , mTextures(textures)
{
}

FlipController::FlipController()
{
}

FlipController::FlipController(const FlipController &copy, const osg::CopyOp &copyop)
    : osg::NodeCallback(copy, copyop)
    , Controller(copy)
    , mTexSlot(copy.mTexSlot)
    , mDelta(copy.mDelta)
    , mTextures(copy.mTextures)
{
}

void FlipController::operator() (osg::Node* node, osg::NodeVisitor* nv)
{
    if (hasInput() && mDelta != 0)
    {
        osg::StateSet* stateset = node->getStateSet();
        int curTexture = int(getInputValue(nv) / mDelta) % mTextures.size();
        osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>(stateset->getAttribute(osg::StateAttribute::TEXTURE));
        if (tex)
            tex->setImage(mTextures[curTexture].get());
    }
    traverse(node, nv);
}

ParticleSystemController::ParticleSystemController(const Nif::NiParticleSystemController *ctrl)
    : mEmitStart(ctrl->startTime), mEmitStop(ctrl->stopTime)
{
}

ParticleSystemController::ParticleSystemController()
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
    if (hasInput())
    {
        osgParticle::ParticleProcessor* emitter = dynamic_cast<osgParticle::ParticleProcessor*>(node);
        float time = getInputValue(nv);
        if (emitter)
            emitter->setEnabled(time >= mEmitStart && time < mEmitStop);
    }
    traverse(node, nv);
}

}
