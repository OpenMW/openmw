#include "animation.hpp"

#include <OgreHardwarePixelBuffer.h>
#include <OgreSkeletonInstance.h>
#include <OgreEntity.h>
#include <OgreBone.h>
#include <OgreSubMesh.h>

namespace MWRender
{
    std::map<std::string, int> Animation::sUniqueIDs;

    Animation::Animation(OEngine::Render::OgreRenderer& _rend)
        : mInsert(NULL)
        , mRend(_rend)
        , mVecRotPos()
        , mTime(0.0f)
        , mStartTime(0.0f)
        , mStopTime(0.0f)
        , mAnimate(0)
        , mRindexI()
        , mTindexI()
        , mShapeNumber(0)
        , mShapeIndexI()
        , mShapes(NULL)
        , mTransformations(NULL)
        , mTextmappings(NULL)
        , mBase(NULL)
    {
    }

    Animation::~Animation()
    {
    }

    std::string Animation::getUniqueID(std::string mesh)
    {
        int counter;
        std::string copy = mesh;
        std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
        
        if(sUniqueIDs.find(copy) == sUniqueIDs.end())
        {
            counter = sUniqueIDs[copy] = 0;
        }
        else
        {
            sUniqueIDs[copy] = sUniqueIDs[copy] + 1;
            counter = sUniqueIDs[copy];
        }

        std::stringstream out;
        
        if(counter > 99 && counter < 1000)
            out << "0";
        else if(counter > 9)
            out << "00";
        else
            out << "000";
        out << counter;
        
        return out.str();
    }
    
    void Animation::startScript(std::string groupname, int mode, int loops)
    {
        //If groupname is recognized set animate to true
        //Set the start time and stop time
        //How many times to loop
        if(groupname == "all")
        {
            mAnimate = loops;
            mTime = mStartTime;
        }
        else if(mTextmappings)
        {

            std::string startName = groupname + ": loop start";
            std::string stopName = groupname + ": loop stop";

            bool first = false;

            if(loops > 1)
            {
                startName = groupname + ": loop start";
                stopName = groupname + ": loop stop";

                for(std::map<std::string, float>::iterator iter = mTextmappings->begin(); iter != mTextmappings->end(); iter++)
                {

                    std::string current = iter->first.substr(0, startName.size());
                    std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                    std::string current2 = iter->first.substr(0, stopName.size());
                    std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                    if(current == startName)
                    {
                        mStartTime = iter->second;
                         mAnimate = loops;
                         mTime = mStartTime;
                         first = true;
                    }
                    if(current2 == stopName)
                    {
                        mStopTime = iter->second;
                        if(first)
                            break;
                    }
                }
            }
            if(!first)
            {
                startName = groupname + ": start";
                stopName = groupname + ": stop";

                for(std::map<std::string, float>::iterator iter = mTextmappings->begin(); iter != mTextmappings->end(); iter++)
                {

                    std::string current = iter->first.substr(0, startName.size());
                    std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                    std::string current2 = iter->first.substr(0, stopName.size());
                    std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                    if(current == startName)
                    {
                        mStartTime = iter->second;
                        mAnimate = loops;
                        mTime = mStartTime;
                        first = true;
                    }
                    if(current2 == stopName)
                    {
                        mStopTime = iter->second;
                        if(first)
                            break;
                    }
                }
            }
        }
    }
        
    void Animation::stopScript()
    {
        mAnimate = 0;
    }
    
    void Animation::handleShapes(std::vector<Nif::NiTriShapeCopy>* allshapes, Ogre::Entity* creaturemodel, Ogre::SkeletonInstance *skel)
    {
        mShapeNumber = 0;

        if (allshapes == NULL || creaturemodel == NULL || skel == NULL)
            return;

        std::vector<Nif::NiTriShapeCopy>::iterator allshapesiter;
        for(allshapesiter = allshapes->begin(); allshapesiter != allshapes->end(); allshapesiter++)
        {
            //std::map<unsigned short, PosAndRot> vecPosRot;

            Nif::NiTriShapeCopy& copy = *allshapesiter;
            std::vector<Ogre::Vector3>* allvertices = &copy.vertices;

            //std::set<unsigned int> vertices;
            //std::set<unsigned int> normals;
            //std::vector<Nif::NiSkinData::BoneInfoCopy> boneinfovector =  copy.boneinfo;
            std::map<int, std::vector<Nif::NiSkinData::IndividualWeight> >* verticesToChange = &copy.vertsToWeights;

            //std::cout << "Name " << copy.sname << "\n";
            Ogre::HardwareVertexBufferSharedPtr vbuf = creaturemodel->getMesh()->getSubMesh(copy.sname)->vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::Real* pReal = static_cast<Ogre::Real*>(vbuf->lock(Ogre::HardwareBuffer::HBL_NORMAL));


            std::vector<Ogre::Vector3> initialVertices = copy.morph.getInitialVertices();
            //Each shape has multiple indices
            if(initialVertices.size() )
            {
                if(copy.vertices.size() == initialVertices.size())
                {
                    //Create if it doesn't already exist
                    if(mShapeIndexI.size() == static_cast<std::size_t> (mShapeNumber))
                    {
                        std::vector<int> vec;
                        mShapeIndexI.push_back(vec);
                    }
                    if(mTime >= copy.morph.getStartTime() && mTime <= copy.morph.getStopTime())
                    {
                        float x;
                        for (unsigned int i = 0; i < copy.morph.getAdditionalVertices().size(); i++)
                        {
                            int j = 0;
                            if(mShapeIndexI[mShapeNumber].size() <= i)
                                mShapeIndexI[mShapeNumber].push_back(0);

                            if(timeIndex(mTime,copy.morph.getRelevantTimes()[i],(mShapeIndexI[mShapeNumber])[i], j, x))
                            {
                                int indexI = (mShapeIndexI[mShapeNumber])[i];
                                std::vector<Ogre::Vector3> relevantData = (copy.morph.getRelevantData()[i]);
                                float v1 = relevantData[indexI].x;
                                float v2 = relevantData[j].x;
                                float t = v1 + (v2 - v1) * x;
                                
                                if ( t < 0 )
                                    t = 0;
                                if ( t > 1 )
                                    t = 1;
                                if( t != 0 && initialVertices.size() == copy.morph.getAdditionalVertices()[i].size())
                                    for (unsigned int v = 0; v < initialVertices.size(); v++)
                                        initialVertices[v] += ((copy.morph.getAdditionalVertices()[i])[v]) * t;
                            }

                        }

                        allvertices = &initialVertices;
                    }
                    mShapeNumber++;
                }
            }


            if(verticesToChange->size() > 0)
            {

                for(std::map<int, std::vector<Nif::NiSkinData::IndividualWeight> >::iterator iter = verticesToChange->begin();
                    iter != verticesToChange->end(); iter++)
                {
                    std::vector<Nif::NiSkinData::IndividualWeight> inds = iter->second;
                    int verIndex = iter->first;
                    Ogre::Vector3 currentVertex = (*allvertices)[verIndex];
                    Nif::NiSkinData::BoneInfoCopy* boneinfocopy = &(allshapesiter->boneinfo[inds[0].boneinfocopyindex]);
                    Ogre::Bone *bonePtr = 0;

                    Ogre::Vector3 vecPos;
                    Ogre::Quaternion vecRot;
                    std::map<Nif::NiSkinData::BoneInfoCopy*, PosAndRot>::iterator result = mVecRotPos.find(boneinfocopy);

                    if(result == mVecRotPos.end())
                    {
                        bonePtr = skel->getBone(boneinfocopy->bonename);

                        vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.trans;
                        vecRot = bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.rotation;

                        PosAndRot both;
                        both.vecPos = vecPos;
                        both.vecRot = vecRot;
                        mVecRotPos[boneinfocopy] = both;

                    }
                    else
                    {
                        PosAndRot both = result->second;
                        vecPos = both.vecPos;
                        vecRot = both.vecRot;
                    }

                    Ogre::Vector3 absVertPos = (vecPos + vecRot * currentVertex) * inds[0].weight;

                    for(std::size_t i = 1; i < inds.size(); i++)
                    {
                        boneinfocopy = &(allshapesiter->boneinfo[inds[i].boneinfocopyindex]);
                        result = mVecRotPos.find(boneinfocopy);

                        if(result == mVecRotPos.end())
                        {
                            bonePtr = skel->getBone(boneinfocopy->bonename);
                            vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.trans;
                            vecRot = bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.rotation;

                            PosAndRot both;
                            both.vecPos = vecPos;
                            both.vecRot = vecRot;
                            mVecRotPos[boneinfocopy] = both;

                        }
                        else
                        {
                            PosAndRot both = result->second;
                            vecPos = both.vecPos;
                            vecRot = both.vecRot;
                        }

                        absVertPos += (vecPos + vecRot * currentVertex) * inds[i].weight;

                    }
                    Ogre::Real* addr = (pReal + 3 * verIndex);
                    *addr = absVertPos.x;
                    *(addr+1) = absVertPos.y;
                    *(addr+2) = absVertPos.z;

                }
            }
            else
            {
                //Ogre::Bone *bonePtr = creaturemodel->getSkeleton()->getBone(copy.bonename);
                Ogre::Quaternion shaperot = copy.trafo.rotation;
                Ogre::Vector3 shapetrans = copy.trafo.trans;
                float shapescale = copy.trafo.scale;
                std::vector<std::string> boneSequence = copy.boneSequence;

                Ogre::Vector3 transmult;
                Ogre::Quaternion rotmult;
                float scale;
                if(boneSequence.size() > 0)
                {
                    std::vector<std::string>::iterator boneSequenceIter = boneSequence.begin();
                    if(skel->hasBone(*boneSequenceIter))
                    {
                        Ogre::Bone *bonePtr = skel->getBone(*boneSequenceIter);

                        transmult = bonePtr->getPosition();
                        rotmult = bonePtr->getOrientation();
                        scale = bonePtr->getScale().x;
                        boneSequenceIter++;

                        for(; boneSequenceIter != boneSequence.end(); boneSequenceIter++)
                        {
                            if(skel->hasBone(*boneSequenceIter))
                            {
                                Ogre::Bone *bonePtr = skel->getBone(*boneSequenceIter);
                                // Computes C = B + AxC*scale
                                transmult = transmult + rotmult * bonePtr->getPosition();
                                rotmult = rotmult * bonePtr->getOrientation();
                                scale = scale * bonePtr->getScale().x;
                            }
                            //std::cout << "Bone:" << *boneSequenceIter << "   ";
                        }
                        transmult = transmult + rotmult * shapetrans;
                        rotmult = rotmult * shaperot;
                        scale = shapescale * scale;

                        //std::cout << "Position: " << transmult << "Rotation: " << rotmult << "\n";
                    }
                }
                else
                {
                    transmult = shapetrans;
                    rotmult = shaperot;
                    scale = shapescale;
                }

                // Computes C = B + AxC*scale
                // final_vector = old_vector + old_rotation*new_vector*old_scale/

                for(unsigned int i = 0; i < allvertices->size(); i++)
                {
                    Ogre::Vector3 current = transmult + rotmult * (*allvertices)[i];
                    Ogre::Real* addr = pReal + i * 3;
                    *addr = current.x;
                    *(addr+1) = current.y;
                    *(addr + 2) = current.z;

                }/*
                for(int i = 0; i < allnormals.size(); i++){
                    Ogre::Vector3 current =rotmult * allnormals[i];
                    Ogre::Real* addr = pRealNormal + i * 3;
                    *addr = current.x;
                    *(addr+1) = current.y;
                    *(addr + 2) = current.z;

                }*/

            }
            vbuf->unlock();
        }
    
    }
        
    bool Animation::timeIndex( float time, const std::vector<float> & times, int & i, int & j, float & x )
    {
        int count;
        if (  (count = times.size()) > 0 )
        {
            if ( time <= times[0] )
            {
                i = j = 0;
                x = 0.0;
                return true;
            }
            if ( time >= times[count - 1] )
            {
                i = j = count - 1;
                x = 0.0;
                return true;
            }

            if ( i < 0 || i >= count )
                i = 0;

            float tI = times[i];
            if ( time > tI )
            {
                j = i + 1;
                float tJ;
                while ( time >= ( tJ = times[j]) )
                {
                    i = j++;
                    tI = tJ;
                }
                x = ( time - tI ) / ( tJ - tI );
                return true;
            }
            else if ( time < tI )
            {
                j = i - 1;
                float tJ;
                while ( time <= ( tJ = times[j] ) )
                {
                    i = j--;
                    tI = tJ;
                }
                x = ( time - tI ) / ( tJ - tI );
                return true;
            }
            else
            {
                j = i;
                x = 0.0;
                return true;
            }
        }
        else
            return false;

    }

    void Animation::handleAnimationTransforms()
    {
        Ogre::SkeletonInstance* skel = mBase->getSkeleton();

        Ogre::Bone* b = skel->getRootBone();
        b->setOrientation(Ogre::Real(.3),Ogre::Real(.3),Ogre::Real(.3), Ogre::Real(.3));   //This is a trick

        skel->_updateTransforms();
        //skel->_notifyManualBonesDirty();

        mBase->getAllAnimationStates()->_notifyDirty();
        //mBase->_updateAnimation();
        //mBase->_notifyMoved();

        std::vector<Nif::NiKeyframeData>::iterator iter;
        int slot = 0;
        if(mTransformations)
        {
            for(iter = mTransformations->begin(); iter != mTransformations->end(); iter++)
            {
                if(mTime < iter->getStartTime() || mTime < mStartTime || mTime > iter->getStopTime())
                {
                    slot++;
                    continue;
                }

                float x;
                float x2;

                const std::vector<Ogre::Quaternion> & quats = iter->getQuat();

                const std::vector<float> & ttime = iter->gettTime();
                const std::vector<float> & rtime = iter->getrTime();
                int rindexJ = mRindexI[slot];

                timeIndex(mTime, rtime, mRindexI[slot], rindexJ, x2);
                int tindexJ = mTindexI[slot];

                const std::vector<Ogre::Vector3> & translist1 = iter->getTranslist1();

                timeIndex(mTime, ttime, mTindexI[slot], tindexJ, x);

                Ogre::Vector3 t;
                Ogre::Quaternion r;

                bool bTrans = translist1.size() > 0;

                bool bQuats = quats.size() > 0;

                if(skel->hasBone(iter->getBonename()))
                {
                    Ogre::Bone* bone = skel->getBone(iter->getBonename());
                    
                    if(bTrans)
                    {
                        Ogre::Vector3 v1 = translist1[mTindexI[slot]];
                        Ogre::Vector3 v2 = translist1[tindexJ];
                        t = (v1 + (v2 - v1) * x);
                        bone->setPosition(t);

                    }
                    
                    if(bQuats)
                    {
                        r = Ogre::Quaternion::Slerp(x2, quats[mRindexI[slot]], quats[rindexJ], true);
                        bone->setOrientation(r);
                    }

                }

                slot++;
            }
            skel->_updateTransforms();
            mBase->getAllAnimationStates()->_notifyDirty();
        }
    }

}
