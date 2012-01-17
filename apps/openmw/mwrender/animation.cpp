#include "animation.hpp"


namespace MWRender{
    std::map<std::string, int> Animation::mUniqueIDs;

    Animation::~Animation(){
    }

    std::string Animation::getUniqueID(std::string mesh){
    int counter;
    std::string copy = mesh;
    std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
    if(mUniqueIDs.find(copy) == mUniqueIDs.end()){
        counter = mUniqueIDs[copy] = 0;
    }
    else{
        mUniqueIDs[copy] = mUniqueIDs[copy] + 1;
        counter = mUniqueIDs[copy];
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
	void Animation::startScript(std::string groupname, int mode, int loops){
		//If groupname is recognized set animate to true
		//Set the start time and stop time
		//How many times to loop
        if(groupname == "all"){
            animate = loops;
            time = startTime;
        }
        else if(textmappings){

            std::string startName = groupname + ": loop start";
            std::string stopName = groupname + ": loop stop";

            bool first = false;

            if(loops > 1){
                startName = groupname + ": loop start";
                stopName = groupname + ": loop stop";

                for(std::map<std::string, float>::iterator iter = textmappings->begin(); iter != textmappings->end(); iter++){

                std::string current = iter->first.substr(0, startName.size());
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::string current2 = iter->first.substr(0, stopName.size());
                std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                if(current == startName){
                    startTime = iter->second;
                     animate = loops;
                     time = startTime;
                     first = true;
                }
                if(current2 == stopName){
                    stopTime = iter->second;
                    if(first)
                        break;
                }
                }
            }
            if(!first){
                startName = groupname + ": start";
                stopName = groupname + ": stop";

            for(std::map<std::string, float>::iterator iter = textmappings->begin(); iter != textmappings->end(); iter++){

                std::string current = iter->first.substr(0, startName.size());
                std::transform(current.begin(), current.end(), current.begin(), ::tolower);
                std::string current2 = iter->first.substr(0, stopName.size());
                std::transform(current2.begin(), current2.end(), current2.begin(), ::tolower);

                if(current == startName){
                    startTime = iter->second;
                     animate = loops;
                     time = startTime;
                     first = true;
                }
                if(current2 == stopName){
                    stopTime = iter->second;
                    if(first)
                        break;
                }
            }
            }

        }

	}
    void Animation::stopScript(){
        animate = 0;
    }

   void Animation::handleShapes(std::vector<Nif::NiTriShapeCopy>* allshapes, Ogre::Entity* creaturemodel, Ogre::SkeletonInstance *skel){
       bool useHandles = skel == creaturemodel->getSkeleton();
        shapeNumber = 0;

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
			       //Ogre::HardwareVertexBufferSharedPtr vbufNormal = creaturemodel->getMesh()->getSubMesh(copy.sname)->vertexData->vertexBufferBinding->getBuffer(1);
		           // Ogre::Real* pRealNormal = static_cast<Ogre::Real*>(vbufNormal->lock(Ogre::HardwareBuffer::HBL_NORMAL));

				std::vector<Ogre::Vector3> initialVertices = copy.morph.getInitialVertices();
				//Each shape has multiple indices
				if(initialVertices.size() )
				{

					if(copy.vertices.size() == initialVertices.size())
					{
						//Create if it doesn't already exist
						if(shapeIndexI.size() == static_cast<std::size_t> (shapeNumber))
						{
							std::vector<int> vec;
							shapeIndexI.push_back(vec);
						}
                        if(time >= copy.morph.getStartTime() && time <= copy.morph.getStopTime()){
						float x;
						for (unsigned int i = 0; i < copy.morph.getAdditionalVertices().size(); i++){
							int j = 0;
							if(shapeIndexI[shapeNumber].size() <= i)
								shapeIndexI[shapeNumber].push_back(0);


							if(timeIndex(time,copy.morph.getRelevantTimes()[i],(shapeIndexI[shapeNumber])[i], j, x)){
							int indexI = (shapeIndexI[shapeNumber])[i];
							std::vector<Ogre::Vector3> relevantData = (copy.morph.getRelevantData()[i]);
							float v1 = relevantData[indexI].x;
							float v2 = relevantData[j].x;
							float t = v1 + (v2 - v1) * x;
							if ( t < 0 ) t = 0;
							if ( t > 1 ) t = 1;
						    if( t != 0 && initialVertices.size() == copy.morph.getAdditionalVertices()[i].size())
							{
								for (unsigned int v = 0; v < initialVertices.size(); v++){
									initialVertices[v] += ((copy.morph.getAdditionalVertices()[i])[v]) * t;
								}
							}

							}



						}

						allvertices = &initialVertices;
                        }
						shapeNumber++;
					}
				}


			    if(verticesToChange->size() > 0){

                for(std::map<int, std::vector<Nif::NiSkinData::IndividualWeight> >::iterator iter = verticesToChange->begin();
                    iter != verticesToChange->end(); iter++)
                {
                    std::vector<Nif::NiSkinData::IndividualWeight> inds = iter->second;
                    int verIndex = iter->first;
                    Ogre::Vector3 currentVertex = (*allvertices)[verIndex];
                    Nif::NiSkinData::BoneInfoCopy* boneinfocopy = &(allshapesiter->boneinfo[inds[0].boneinfocopyindex]);
                    Ogre::Bone *bonePtr = 0;
                    if(useHandles)
                    {
                        bonePtr = skel->getBone(boneinfocopy->bonehandle);
                    }
                    else
                        bonePtr = skel->getBone(boneinfocopy->bonename);

                    Ogre::Vector3 vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.trans;
                    Ogre::Quaternion vecRot = bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.rotation;


                    /*if(vecPosRot.find(boneinfocopy->bonehandle) == vecPosRot.end()){
                        vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.trans;
                        vecRot = bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.rotation;

                        if(useHandles){
                             PosAndRot both;
                            both.vecPos = vecPos;
                            both.vecRot = vecRot;
                            vecPosRot[boneinfocopy->bonehandle] = both;
                        }
                    }
                    else{
                        PosAndRot both = vecPosRot[boneinfocopy->bonehandle];
                        vecPos = both.vecPos;
                        vecRot = both.vecRot;
                    }*/

                    Ogre::Vector3 absVertPos = (vecPos + vecRot * currentVertex) * inds[0].weight;


                    for(std::size_t i = 1; i < inds.size(); i++){
                        boneinfocopy = &(allshapesiter->boneinfo[inds[i].boneinfocopyindex]);
                        if(useHandles)
                            bonePtr = skel->getBone(boneinfocopy->bonehandle);
                        else
                            bonePtr = skel->getBone(boneinfocopy->bonename);
                        vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.trans;
                            vecRot = bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.rotation;

                        /*if(vecPosRot.find(boneinfocopy->bonehandle) == vecPosRot.end()){
                            vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.trans;
                            vecRot = bonePtr->_getDerivedOrientation() * boneinfocopy->trafo.rotation;

                            if(useHandles){
                                PosAndRot both;
                                both.vecPos = vecPos;
                                both.vecRot = vecRot;
                                vecPosRot[boneinfocopy->bonehandle] = both;
                            }
                        }
                         else{
                                PosAndRot both = vecPosRot[boneinfocopy->bonehandle];
                                vecPos = both.vecPos;
                                vecRot = both.vecRot;
                        }*/


                        absVertPos += (vecPos + vecRot * currentVertex) * inds[i].weight;

                    }
                     Ogre::Real* addr = (pReal + 3 * verIndex);
							  *addr = absVertPos.x;
							  *(addr+1) = absVertPos.y;
				              *(addr+2) = absVertPos.z;
                }

#if 0
				for (unsigned int i = 0; i < boneinfovector.size(); i++)
				{
					Nif::NiSkinData::BoneInfoCopy boneinfo = boneinfovector[i];
					if(skel->hasBone(boneinfo.bonename)){
					Ogre::Bone *bonePtr = skel->getBone(boneinfo.bonename);
					Ogre::Vector3 vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfo.trafo.trans;
					Ogre::Quaternion vecRot = bonePtr->_getDerivedOrientation() * boneinfo.trafo.rotation;

					 for (unsigned int j=0; j < boneinfo.weights.size(); j++)
					 {
						  unsigned int verIndex = boneinfo.weights[j].vertex;
						  if(vertices.find(verIndex) == vertices.end())
						  {
							  Ogre::Vector3 absVertPos = vecPos + vecRot * allvertices[verIndex];
							  absVertPos = absVertPos * boneinfo.weights[j].weight;
							  vertices.insert(verIndex);
							   Ogre::Real* addr = (pReal + 3 * verIndex);
							  *addr = absVertPos.x;
							  *(addr+1) = absVertPos.y;
				              *(addr+2) = absVertPos.z;


						  }
						  else
						  {

							   Ogre::Vector3 absVertPos = vecPos + vecRot * allvertices[verIndex];
							   absVertPos = absVertPos * boneinfo.weights[j].weight;
							   Ogre::Vector3 old = Ogre::Vector3(pReal + 3 * verIndex);
							   absVertPos = absVertPos + old;
							   Ogre::Real* addr = (pReal + 3 * verIndex);
							  *addr = absVertPos.x;
							  *(addr+1) = absVertPos.y;
				              *(addr+2) = absVertPos.z;

							  //std::cout << "Vertex" << verIndex << "Weight: " << boneinfo.weights[i].weight << "was seen twice\n";

						  }

						  if(normals.find(verIndex) == normals.end())
						  {
							  Ogre::Vector3 absNormalsPos = vecRot * allnormals[verIndex];
							  absNormalsPos = absNormalsPos * boneinfo.weights[j].weight;
							  normals.insert(verIndex);
							  Ogre::Real* addr = (pRealNormal + 3 * verIndex);
							  *addr = absNormalsPos.x;
				              *(addr+1) = absNormalsPos.y;
				              *(addr+2) = absNormalsPos.z;
						  }
						  else
						  {
							   Ogre::Vector3 absNormalsPos = vecRot * allnormals[verIndex];
							  absNormalsPos = absNormalsPos * boneinfo.weights[j].weight;
							 Ogre::Vector3 old = Ogre::Vector3(pRealNormal + 3 * verIndex);
							 absNormalsPos = absNormalsPos + old;

							  Ogre::Real* addr = (pRealNormal + 3 * verIndex);
							  *addr = absNormalsPos.x;
				              *(addr+1) = absNormalsPos.y;
				              *(addr+2) = absNormalsPos.z;

						  }
#endif
					 //}
				//}


				//}   //Comment out

				   ;
				}
				else
				{
					//Ogre::Bone *bonePtr = creaturemodel->getSkeleton()->getBone(copy.bonename);
					Ogre::Quaternion shaperot = copy.trafo.rotation;
					Ogre::Vector3 shapetrans = copy.trafo.trans;
					float shapescale = copy.trafo.scale;
					std::vector<std::string> boneSequence = copy.boneSequence;
					std::vector<std::string>::iterator boneSequenceIter = boneSequence.begin();
					Ogre::Vector3 transmult;
						Ogre::Quaternion rotmult;
						float scale;
					if(skel->hasBone(*boneSequenceIter)){
					Ogre::Bone *bonePtr = skel->getBone(*boneSequenceIter);




						transmult = bonePtr->getPosition();
						rotmult = bonePtr->getOrientation();
						scale = bonePtr->getScale().x;
						boneSequenceIter++;
					    for(; boneSequenceIter != boneSequence.end(); boneSequenceIter++)
					    {
							if(creaturemodel->getSkeleton()->hasBone(*boneSequenceIter)){
							Ogre::Bone *bonePtr = creaturemodel->getSkeleton()->getBone(*boneSequenceIter);
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
					else
					{
						transmult = shapetrans;
						rotmult = shaperot;
						scale = shapescale;
					}




					// Computes C = B + AxC*scale
					 // final_vector = old_vector + old_rotation*new_vector*old_scale/

					for(unsigned int i = 0; i < allvertices->size(); i++){
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
					//vbufNormal->unlock();
		}

    }
    bool Animation::timeIndex( float time, std::vector<float> times, int & i, int & j, float & x ){
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

 void Animation::handleAnimationTransforms(){
    Ogre::SkeletonInstance* skel = base->getSkeleton();


    Ogre::Bone* b = skel->getRootBone();
	    b->setOrientation(Ogre::Real(.3),Ogre::Real(.3),Ogre::Real(.3), Ogre::Real(.3));   //This is a trick

        skel->_updateTransforms();
	    //skel->_notifyManualBonesDirty();

         base->getAllAnimationStates()->_notifyDirty();
     //base->_updateAnimation();
   base->_notifyMoved();

    for(unsigned int i = 0; i < entityparts.size(); i++){
         Ogre::SkeletonInstance* skel = entityparts[i]->getSkeleton();

        Ogre::Bone* b = skel->getRootBone();
	   b->setOrientation(Ogre::Real(.3),Ogre::Real(.3),Ogre::Real(.3), Ogre::Real(.3));//This is a trick

        skel->_updateTransforms();
	   // skel->_notifyManualBonesDirty();

         entityparts[i]->getAllAnimationStates()->_notifyDirty();
        //entityparts[i]->_updateAnimation();
         entityparts[i]->_notifyMoved();
    }

    std::vector<Nif::NiKeyframeData>::iterator iter;
    int slot = 0;
    if(transformations){
    for(iter = transformations->begin(); iter != transformations->end(); iter++){
        if(time < iter->getStartTime() || time < startTime || time > iter->getStopTime())
	    {
            slot++;
            //iter++;
		    continue;

	    }

         float x;
		float x2;

	    std::vector<Ogre::Quaternion> quats = iter->getQuat();

        std::vector<float> ttime = iter->gettTime();
        std::vector<float>::iterator ttimeiter = ttime.begin();

        std::vector<float> rtime = iter->getrTime();
        int rindexJ = 0;
	    timeIndex(time, rtime, rindexI[slot], rindexJ, x2);
	    int tindexJ = 0;


        std::vector<Ogre::Vector3> translist1 = iter->getTranslist1();

        timeIndex(time, ttime, tindexI[slot], tindexJ, x);

		//std::cout << "X: " << x << " X2: " << x2 << "\n";
        Ogre::Vector3 t;
        Ogre::Quaternion r;

        bool bTrans = translist1.size() > 0;
	    if(bTrans){
            Ogre::Vector3 v1 = translist1[tindexI[slot]];
            Ogre::Vector3 v2 = translist1[tindexJ];
           t = (v1 + (v2 - v1) * x);

	    }

        bool bQuats = quats.size() > 0;
	    if(bQuats){
		    r = Ogre::Quaternion::Slerp(x2, quats[rindexI[slot]], quats[rindexJ], true);
		    //bone->setOrientation(r);
	    }
        skel = base->getSkeleton();
    if(skel->hasBone(iter->getBonename())){
        Ogre::Bone* bone = skel->getBone(iter->getBonename());
        if(bTrans)
            bone->setPosition(t);
        if(bQuats)
            bone->setOrientation(r);



        skel->_updateTransforms();
	    //skel->_notifyManualBonesDirty();
        base->getAllAnimationStates()->_notifyDirty();
        //base->_updateAnimation();
	    base->_notifyMoved();
	}
    for(std::size_t i = 0; i < entityparts.size(); i++){
        skel = entityparts[i]->getSkeleton();
         if(skel->hasBone(iter->getBonename())){
            Ogre::Bone* bone = skel->getBone(iter->getBonename());
            if(bTrans)
                bone->setPosition(t);
            if(bQuats)
                bone->setOrientation(r);

            skel->_updateTransforms();
	        //skel->_notifyManualBonesDirty();
            entityparts[i]->getAllAnimationStates()->_notifyDirty();
           // entityparts[i]->_updateAnimation();
	        entityparts[i]->_notifyMoved();
	    }
    }
    slot++;
    }
}
}

}
