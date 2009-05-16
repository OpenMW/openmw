/**
 * Holds index and other data describing the landscape.data file.
 */
class Index
{
public:
  typedef std::map<long, std::map<long, long> >::iterator OffsetItr;
  typedef std::map<long, std::map<long, long> >::const_iterator OffsetConstItr;

  /**
   * @brief sets the root quads side length in gu
   * @param l the side length
   *
   * This is used for working out the locations of quad children.
   * I am assuming a long is enough...
   */
  inline void setRootSideLength(long l) {
    mRootSideLength = l;
  }
  /**
   * @return the side length of the root quad.
   */
  inline long getRootSideLength() const {
    return mRootSideLength;
  }

  inline void setMaxDepth(int d) {
    mMaxDepth = d;
  }
  inline int getMaxDepth() const {
    return mMaxDepth;
  }

  /**
   * @return -1 is returned if there is no offset
   * @param x, y the position of the quad in gu
   *
   * Slightly faster using hasOffset to check if it exists
   * Shouldn't be noticable diffrence.
   */
  inline long getOffset(long x, long y) const { //inline?
    OffsetConstItr itr1 = mQuadOffsets.find(x);
    if ( itr1 == mQuadOffsets.end() ) return -1;
    std::map<long, long>::const_iterator itr2 = itr1->second.find(y);
    if ( itr2 == itr1->second.end() ) return -1;
    return itr2->second;
  }

  /**
   * @brief checks if a quad for the given position exists
   * @return true/false
   * @param x, y the position of the quad in gu
   *
   * @todo Would it be worth merging this with getOffset?
   */
  inline bool hasOffset(long x, long y) const {
    OffsetConstItr itr = mQuadOffsets.find(x);
    if ( itr == mQuadOffsets.end() ) return false;
    return (itr->second.find(y) != itr->second.end());
  }

  /**
   * @brief sets an offset of a quad
   * @param x, y the position in gu of the quad
   * @param o the offset within the file of the records for this quad
   */
  inline void setOffset(long x, long y, long o) {
    mQuadOffsets[x][y] = o;
  }

protected:
  std::map<long, std::map<long, long> > mQuadOffsets;
  long mRootSideLength; ///length in gu of the root quad
  int mMaxDepth; ///maximum depth assuming root quad depth = 0

  friend class boost::serialization::access;
  /**
   * Saves the data for the max depth, the root side legnth, and the quad offsets
   */
  template<class Archive>
  inline void serialize(Archive& ar, const unsigned int version){

    ar &mMaxDepth;
    ar &mRootSideLength;
    ar &mQuadOffsets;

  }

};

BOOST_CLASS_TRACKING(Index, boost::serialization::track_never);
