/* Represents a segment of a quad. It can also represent a large segment
 *
 * this is needed for experimenting with splitting the lowest
 * quad level into small segments for better culling.  Due to the
 * intented design of using optomized meshes, this should not work on
 * optomized meshes, as there is no easy way to segment the data
 *
 * This uses floating point numbers to define the areas of the
 * quads. There may be issues due to accuracy if we go to small but it
 * should be caught in debug mode
 */
#define QSDEBUG

class QuadSegment
{
public:
  /**
   * @param qd the parent quad data
   * @param size the proportion of the total size that the segment is. Must be greater than 0, but less than or equal to 1
   * @param x,y the start positions of the segment
   */
  QuadSegment(QuadData* qd, float size = 1, float x = 0, float y = 0)
    : mQuadData(qd),
      mSegmentSize(size),
      mX(x),
      mY(y)
  {
    assert(qd);
    assert(size>0&&size<=1);
    assert(mY>=0&&mY<=1);
    assert(mX>=0&&mY<=1);

#ifdef QSDEBUG
    {
      //check sizes
      const float qw = mQuadData->getVertexWidth()-1;
      const float fsw = qw*size;
      const int isw = (int)fsw;
      assert(fsw==isw);
    }
#endif

    //precalc offsets, as getVertex/getNormal get called a lot (1000s of times)
    computeOffsets();
  }

  /**
   * Gets how many vertexes wide this quad segment is. Should always be 2^n+1
   * @return the vertex width of this quad segment
   */
  int getVertexWidth()
  {
    return (mQuadData->getVertexWidth()-1)*mSegmentSize+1;
  }

  /**
   * @brief gets a vertex assuming that x = 0, y = 0 addresses the start of the quad
   */
  float getVertex(int x, int y)
  {
#ifdef QSDEBUG
    {
      const int vw = getVertexWidth();
      assert(x<vw);
    }
#endif
    return mQuadData->getVertex((mYOffset + y)*mQuadData->getVertexWidth()+(mXOffset+x));
  }

  float getNormal(int x, int y, int z)
  {
    assert(z>=0&&z<3);
    return mQuadData->getNormal(((mYOffset + y)*mQuadData->getVertexWidth()+(mXOffset+x))*3+z);
  }

  /**
   * @brief lazy function for getting a material
   */
  Ogre::MaterialPtr getMaterial()
  {
    if ( mMaterial.isNull() )
      createMaterial();
    assert(!mMaterial.isNull());
    return mMaterial;
  }

  void createMaterial()
  {
    assert(mSegmentSize>0);
    if ( mSegmentSize == 1 ) //we can use the top level material
      mMaterial = mQuadData->getMaterial();
    else //generate a material spesificly for this
      mMaterial = mQuadData->getMaterialGenerator()->
        getMaterialSegment(mQuadData,this);
    assert(!mMaterial.isNull());
  }

  inline bool hasParentTexture() const{
    return mQuadData->hasParentTexture();
  }
  inline const std::string& getParentTexture() const{
    return mQuadData->getParentTexture();
  }
  inline int getVertexSeperation(){
    return mQuadData->getVertexSeperation();
  }

  inline float getSegmentSize(){
    return mSegmentSize;
  }
  inline float getStartX(){
    return mX;
  }
  inline float getStartY(){
    return mY;
  }

private:
  int computeOffset(float x)
  {
#ifdef QSDEBUG
    {
      //check it is a valid position
      const int start = (mQuadData->getVertexWidth()-1)*x;
      const int vw = getVertexWidth()-1;
      assert(vw>0);
      assert((start%vw)==0);
    }
#endif
    return float((mQuadData->getVertexWidth()-1))*x;
  }

  void computeOffsets()
  {
    mXOffset = computeOffset(mX);
    mYOffset = computeOffset(mY);
  }

  QuadData* mQuadData;
  float mSegmentSize;
  float mX, mY;
  Ogre::MaterialPtr mMaterial;

  int mXOffset, mYOffset;
};
