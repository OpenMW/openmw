class MWLand
{
public:
  MWLand()
  {
    mMaxX = mMaxY = mMinX = mMinY = 0;
  }

  void addLandTextureData(RecordPtr record, const std::string& source)
  {
    LandTexture l;
    l.name = record->getSubRecordData("NAME");
    l.data = record->getSubRecordData("DATA");
    l.intv = *((short*) record->getSubRecordData("INTV").c_str());
    mLTEXRecords[source][l.intv] = l;
  }

  void addLandData(RecordPtr record, const std::string& source)
  {
    if ( !record->hasSubRecord("VHGT") || !record->hasSubRecord("VTEX") ) //ensure all records exist
      return;

    //copy these, else we end up with invliad data
    LAND::INTV intv = *(LAND::INTV*)record->getSubRecordData("INTV").c_str();
    LAND::VHGT vhgt = *(LAND::VHGT*)record->getSubRecordData("VHGT").c_str();
    LAND::VNML vnml = *(LAND::VNML*)record->getSubRecordData("VNML").c_str();
    LAND::VTEX vtex = *(LAND::VTEX*)record->getSubRecordData("VTEX").c_str();

    //GridPosition gp(intv.x, intv.y);
    mLandRecords[intv.x][intv.y].heights = parseHeights(&vhgt); //convert into a format we want
    mLandRecords[intv.x][intv.y].normals = parseNormals(&vnml);
    mLandRecords[intv.x][intv.y].textures = parseTextures(&vtex);
    mLandRecords[intv.x][intv.y].source = source;

    mMaxX = std::max<int>(mMaxX, intv.x);
    mMaxY = std::max<int>(mMaxY, intv.y);
    mMinX = std::min<int>(mMinX, intv.x);
    mMinY = std::min<int>(mMinY, intv.y);
  }

  ///Maximum distance of a cell on the X plane from grid pos 0 in the positive direction
  inline int getMaxX() const { return mMaxX; }
  ///Maximum distance of a cell on the Y plane from grid pos 0 in the positvie direction
  inline int getMaxY() const { return mMaxY; }
  ///Maximum distance of a cell on the X plane from grid pos 0 in the negative direction
  inline int getMinX() const { return mMinX; }
  ///see others
  inline int getMinY() const { return mMinY; }

  inline std::vector<float>& getHeights(int x, int y)
  {
    if ( hasData(x,y) )
      return mLandRecords[x][y].heights;
    static std::vector<float> e(LAND_NUM_VERTS, LAND_DEFAULT_HEIGHT);
    return e;
  }

  inline std::vector<char>& getNormals(int x, int y)
  {
    if ( hasData(x,y) )
      return mLandRecords[x][y].normals;
    static std::vector<char> e(LAND_NUM_VERTS*3,0);
    return e;
  }

  inline const std::string& getSource(int x, int y)
  {
    assert(hasData(x,y));
    return mLandRecords[x][y].source;
  }

  inline bool hasData(int x, int y) const
  {
    std::map<int , std::map<int, LandData> >::const_iterator itr = mLandRecords.find(x);
    if ( itr == mLandRecords.end() )
      return false;
    if ( itr->second.find(y) == itr->second.end() )
      return false;
    return true;
  }


  inline std::string& getLTEXRecord(const std::string& source, short i)
  {
    return mLTEXRecords[source][i].data;
  }

  inline bool hasLTEXRecord(const std::string& source, short index) const
  {
    std::map<std::string, std::map<short, LandTexture> >::const_iterator itr = mLTEXRecords.find(source);
    if ( itr == mLTEXRecords.end() )
      return false;
    if ( itr->second.find(index) == itr->second.end() )
      return false;
    return true;
  }

  inline short getLTEXIndex(int x, int y, int pos)
  {
    assert(hasData(x,y));
    return mLandRecords[x][y].textures[pos];
  }

  inline short getLTEXIndex(int x1, int y1, int x2, int y2)
  {
    return getLTEXIndex(x1, y1, y2*LAND_LTEX_WIDTH+x2);
  }

private:

  ///the min/max size of cells
  int mMaxY, mMinY;
  int mMaxX, mMinX;

  // Land structure as held in the ESM
  struct LAND {
    struct INTV { /// x, y grid pos of the cell
      long x;
      long y;
    };
    struct VNML { ///vertex normal data
      struct NORMAL {
        char x;
        char y;
        char z;
      };
      NORMAL normals[LAND_NUM_VERTS];
    };
    struct VHGT { ///height data
      float heightOffset;
      char  heightData[LAND_NUM_VERTS];
      short unknown1;
      char unknown2;
    };
    struct VTEX { ///splat texture data
      short index[LAND_NUM_LTEX];
    };
    INTV* intv;
    VNML* vnml;
    VHGT* vhgt;
    VTEX* vtex;
  };

  std::vector<float> parseHeights(LAND::VHGT* vhgt)
  {
    std::vector<float> ph;
    ph.resize(LAND_NUM_VERTS, LAND_DEFAULT_HEIGHT);
    float offset = vhgt->heightOffset;
    for (int y = 0; y < LAND_VERT_WIDTH; y++) { //code from MGE
      offset += vhgt->heightData[y*LAND_VERT_WIDTH];
      ph[y*LAND_VERT_WIDTH] =+ (float)offset*8;
      float pos = offset;
      for (int x = 1; x < LAND_VERT_WIDTH; x++) {
        pos += vhgt->heightData[y*LAND_VERT_WIDTH+x];
        ph[y*LAND_VERT_WIDTH+x] = pos*8; //flipped x
      }
    }
    return ph;
  }

  std::vector<char> parseNormals( LAND::VNML* vnml )
  {
    std::vector<char> n;
    n.resize(LAND_NUM_VERTS*3,0);
    for ( int y = 0; y < LAND_VERT_WIDTH; y++ ) { //this could just be cast.
      for ( int x = 0; x < LAND_VERT_WIDTH; x++ ) { //as a vector is a continus segment of mem...
        n[(y*LAND_VERT_WIDTH+x)*3] = vnml->normals[y*LAND_VERT_WIDTH+x].x;
        n[(y*LAND_VERT_WIDTH+x)*3+1] = vnml->normals[y*LAND_VERT_WIDTH+x].y;
        n[(y*LAND_VERT_WIDTH+x)*3+2] = vnml->normals[y*LAND_VERT_WIDTH+x].z;
      }
    }
    return n;
  }

  std::vector<short> parseTextures( LAND::VTEX* vtex )
  {
    std::vector<short> t;
    t.resize(LAND_NUM_LTEX,0);

    //thanks to timeslip (MGE) for the code
    int rpos = 0; //bit ugly, but it works
    for ( int y1 = 0; y1 < 4; y1++ )
      for ( int x1 = 0; x1 < 4; x1++ )
        for ( int y2 = 0; y2 < 4; y2++)
          for ( int x2 = 0; x2 < 4; x2++ )
            t[(y1*4+y2)*16+(x1*4+x2)]=vtex->index[rpos++];
    return t;
  }

  /**
   * Holds the representation of a cell in the way that is most usefull to me
   */
  struct LandData
  {
    std::string source; //data file the data is from
    std::vector<float> heights;
    std::vector<char> normals;
    std::vector<short> textures;
  };

  struct LandTexture
  {
    std::string name, data;
    short intv;
  };

  std::map<std::string, std::map<short, LandTexture> > mLTEXRecords;
  std::map<int, std::map<int,LandData> > mLandRecords;
};
