#include "animation.hpp"


namespace MWRender{
    std::map<std::string, int> Animation::mUniqueIDs;
    Animation::~Animation(){
        base = 0;
    }
    std::string Animation::getUniqueID(std::string mesh){
    int counter;
    if(mUniqueIDs.find(mesh) == mUniqueIDs.end()){
        counter = mUniqueIDs[mesh] = 0;
    }
    else
        counter = mUniqueIDs[mesh]++;
    
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

   void Animation::handleShapes(std::vector<Nif::NiTriShapeCopy>* allshapes, Ogre::Entity* creaturemodel, Ogre::SkeletonInstance *skel){
        shapeNumber = 0;
        std::vector<Nif::NiTriShapeCopy>::iterator allshapesiter;
	    for(allshapesiter = allshapes->begin(); allshapesiter != allshapes->end(); allshapesiter++)
		{
        
			Nif::NiTriShapeCopy copy = *allshapesiter;
			std::vector<Ogre::Vector3> allvertices = copy.vertices;
			std::vector<Ogre::Vector3> allnormals = copy.normals;



			std::set<unsigned int> vertices;
			std::set<unsigned int> normals;
			std::vector<Nif::NiSkinData::BoneInfoCopy> boneinfovector =  copy.boneinfo;
	
			//std::cout << "Name " << copy.sname << "\n";
			Ogre::HardwareVertexBufferSharedPtr vbuf = creaturemodel->getMesh()->getSubMesh(copy.sname)->vertexData->vertexBufferBinding->getBuffer(0);
		            Ogre::Real* pReal = static_cast<Ogre::Real*>(vbuf->lock(Ogre::HardwareBuffer::HBL_NORMAL));
			       Ogre::HardwareVertexBufferSharedPtr vbufNormal = creaturemodel->getMesh()->getSubMesh(copy.sname)->vertexData->vertexBufferBinding->getBuffer(1);
		            Ogre::Real* pRealNormal = static_cast<Ogre::Real*>(vbufNormal->lock(Ogre::HardwareBuffer::HBL_NORMAL));

				std::vector<Ogre::Vector3> initialVertices = copy.morph.getInitialVertices();
				//Each shape has multiple indices
				if(initialVertices.size() )
				{

					if(copy.vertices.size() == initialVertices.size())
					{
						//Create if it doesn't already exist
						if(shapeIndexI.size() == shapeNumber)
						{
							std::vector<int> vec;
							shapeIndexI.push_back(vec);
						}
                        if(time >= copy.morph.getStartTime() && time <= copy.morph.getStopTime()){
						float x;
						for (int i = 0; i < copy.morph.getAdditionalVertices().size(); i++){
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
								for (int v = 0; v < initialVertices.size(); v++){
									initialVertices[v] += ((copy.morph.getAdditionalVertices()[i])[v]) * t;
								}
							}

							}

							

						}
						//After everything, write everything out
						
						/*
						for(int i = 0; i < initialVertices.size(); i++){
						Ogre::Vector3 current = initialVertices[i];
						Ogre::Real* addr = pReal + i * 3;
					    *addr = current.x;
						*(addr+1) = current.y;
						*(addr + 2) = current.z;

					}*/
						allvertices = initialVertices;
                        }
						shapeNumber++;
					}
				}


			    if(boneinfovector.size() > 0){

				
				for (int i = 0; i < boneinfovector.size(); i++)
				{
					Nif::NiSkinData::BoneInfoCopy boneinfo = boneinfovector[i];
					if(skel->hasBone(boneinfo.bonename)){
					Ogre::Bone *bonePtr = skel->getBone(boneinfo.bonename);
					Ogre::Vector3 vecPos = bonePtr->_getDerivedPosition() + bonePtr->_getDerivedOrientation() * boneinfo.trafo.trans;
					Ogre::Quaternion vecRot = bonePtr->_getDerivedOrientation() * boneinfo.trafo.rotation;
					//std::cout << "Bone" << bonePtr->getName() << "\n";
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

								//std::cout << "Vertex" << vertices[verIndex] << "\n";
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

					 }
				}
				
					
				}
				   
				   
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
					if(creaturemodel->getSkeleton()->hasBone(*boneSequenceIter)){
					Ogre::Bone *bonePtr = creaturemodel->getSkeleton()->getBone(*boneSequenceIter);
					
						
					

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
					
					for(int i = 0; i < allvertices.size(); i++){
						Ogre::Vector3 current = transmult + rotmult * allvertices[i];
						Ogre::Real* addr = pReal + i * 3;
					    *addr = current.x;
						*(addr+1) = current.y;
						*(addr + 2) = current.z;

					}
					for(int i = 0; i < allnormals.size(); i++){
						Ogre::Vector3 current =rotmult * allnormals[i];
						Ogre::Real* addr = pRealNormal + i * 3;
					    *addr = current.x;
						*(addr+1) = current.y;
						*(addr + 2) = current.z;

					}

				}
				
		}
		

		for(allshapesiter = allshapes->begin(); allshapesiter != allshapes->end(); allshapesiter++)
		{
			Nif::NiTriShapeCopy copy = *allshapesiter;
			Ogre::HardwareVertexBufferSharedPtr vbuf = creaturemodel->getMesh()->getSubMesh(copy.sname)->vertexData->vertexBufferBinding->getBuffer(0);

			        Ogre::HardwareVertexBufferSharedPtr vbufNormal = creaturemodel->getMesh()->getSubMesh(copy.sname)->vertexData->vertexBufferBinding->getBuffer(1);
					vbuf->unlock();
					vbufNormal->unlock();
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
}