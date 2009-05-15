
/**
 * @todo mergre with matgen class
 */
class MWQuadMaterialGen : public QuadMaterialGenerator
{
public:
  MWQuadMaterialGen(MaterialGenerator* mg) : mMatGen(mg), mCount(0){}

  Ogre::MaterialPtr getMaterial(QuadData* qd){
    return _getMaterial((MWQuadData*)qd);
  }
  Ogre::MaterialPtr getMaterialSegment(QuadData* qd, QuadSegment* qs){
    return _getMaterialSegment((MWQuadData*)qd,qs);
  }
private:

  Ogre::MaterialPtr _getMaterial(MWQuadData* qd){
    assert(qd);
    const std::string name("MAT" + Ogre::StringConverter::toString(mCount++));

    if ( qd->getTexture().length() ){
      return mMatGen->generateSingleTexture(name, qd->getTexture(), qd->getUsedResourcesRef());

    }else{
      assert(0);
    }

    return Ogre::MaterialPtr();
  }

  Ogre::MaterialPtr _getMaterialSegment(MWQuadData* qd, QuadSegment* qs){
    const std::string name("MAT" + Ogre::StringConverter::toString(mCount++));


    if ( qd->getTexture().length() )
      assert(0&&"NOT IMPLEMENTED");

    const std::vector<int>& tref = qd->getTextureIndexRef();
    const int indexSize = sqrt(tref.size());
    const int cellIndexSize = indexSize - 2;


    //plus 1 to take into account border
    const int xoff = float(cellIndexSize) * qs->getStartX();
    const int yoff = float(cellIndexSize) * qs->getStartY();
    const int size = float(cellIndexSize) * qs->getSegmentSize();

    std::vector<int> ti;


    ti.resize((size+2)*(size+2), -1);

    for ( int y = 0; y < size+2; ++y ){
      for ( int x = 0; x < size+2; ++x ){
        ti[(y)*(size+2)+(x)] = tref.at((y+yoff)*(indexSize)+(x+xoff));
      }
    }
    /*
      ti.resize((size)*(size));

      for ( int y = 1; y < size+1; ++y ){
      for ( int x = 1; x < size+1; ++x ){

      if ( y+yoff >= indexSize ) assert(0);
      if ( x+xoff >= indexSize ) assert(0);

      ti.at((y-1)*(size)+(x-1)) = tref.at((y+yoff)*(indexSize)+(x+xoff));
      }
      }
    */
    Ogre::MaterialPtr t = Ogre::MaterialManager::getSingleton().
      getByName(mMatGen->generateAlphaMap
                (name,
                 ti,size,
                 1, 1.0f/size,
                 qd->getUsedResourcesRef()));
    return t;
  }

  MaterialGenerator* mMatGen;
  unsigned int mCount;

};
