/*
  Copyright (c) Jacob Essex 2009

  This file is part of MWLand.

  MWLand is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MWLand is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MWLand.  If not, see <http://www.gnu.org/licenses/>.
*/

///generic subrecord
struct SubRecord
{
  SubRecord(){}
  SubRecord(const std::string& n, const std::string& d) : subName(n), subData(d){}
  std::string subName;
  std::string subData;
};

///generic record
class Record {
public:
  Record(const std::string& type)
    : mType(type) {}

  inline void addSubRecord(const std::string& id,
                           const SubRecord& subRecord)
  {mSubRecords[id] = subRecord; }

  std::string getSubRecordData(const std::string& recordName)
  {
    SubRecordMapItr itr = mSubRecords.find(recordName );
    if ( itr != mSubRecords.end() )
      return itr->second.subData;
    return std::string("");
  }

  bool hasSubRecord(const std::string& recordName)
  {
    if ( mSubRecords.find(recordName ) != mSubRecords.end() )
      return true;
    return false;
  }

  inline const std::string& getID() { return mId; }
  inline void setID( const std::string& id) { mId = id;}
  const std::string& getType(){return mType;}

private:
  typedef std::map<std::string, SubRecord> SubRecordMap;
  typedef SubRecordMap::iterator SubRecordMapItr;
  SubRecordMap mSubRecords;

  std::string mType;
  std::string mId;

};

typedef boost::shared_ptr<Record> RecordPtr;
typedef std::list<RecordPtr> RecordList;
typedef RecordList::iterator RecordListItr;
typedef boost::shared_ptr<RecordList> RecordListPtr;

typedef std::map<std::string, RecordPtr> RecordMap;
typedef RecordMap::iterator RecordMapItr;

///top level class for loading and saving esp files.
class ESM
{
private:
  /// types of records to load
  std::map<std::string, std::string> mLoadTypes;

  /// map<id, record> of the record
  RecordMap mRecords;

  ///checks if the given type should be loaded
  inline bool loadType(const std::string& t)
  {
    return ( mLoadTypes.find(t) != mLoadTypes.end() );
  }

public:
  inline void addRecordType(const std::string& t,
                            const std::string& i = "NAME")
  { mLoadTypes[t] = i;  }

  bool loadFile(const std::string& file)
  {
    std::ifstream esp (file.c_str(), std::ios::in | std::ios::binary);

    if ( !esp.is_open() ) return false; //check open

    esp.seekg(4);

    long hdrSize; //get offset for start of data
    esp.read ((char *)&hdrSize, sizeof(long));

    //get num records
    esp.seekg(16 + 8 + 296);
    long numRecords;
    esp.read ((char *)&numRecords, sizeof(long));

    esp.seekg(hdrSize + 16); //go to end of header

    for ( long i = 0; i < numRecords; i++ ){

      char type[5];
      esp.get(type, 5);
      type[4] = '\0';

      long recordSize;
      esp.read ((char *)&recordSize, 4);
      esp.seekg(8, std::ofstream::cur);
      long endPos = recordSize +  esp.tellg();

      if ( loadType(type) ) {
        RecordPtr record = RecordPtr(new Record(type));

        //load all subrecords
        while ( esp.tellg() < endPos ) {
          char subType[5];
          esp.get(subType, 5);

          long subRecLength;
          esp.read ((char *)&subRecLength, 4);

          long subRecEnd = subRecLength + esp.tellg();
          char* subRecData = new char[subRecLength];
          esp.read(subRecData, subRecLength);

          record->addSubRecord(subType, SubRecord(subType,std::string(subRecData, subRecLength)));
          delete [] subRecData;

          assert(subRecEnd==esp.tellg());
        }
        record->setID(record->getSubRecordData(mLoadTypes[type]));
        mRecords[record->getSubRecordData(mLoadTypes[type])] = record;
      }else{
        esp.seekg(endPos);
      }
      assert(endPos==esp.tellg());

    }
    esp.close();

    return true;
  }

  inline RecordPtr getRecord(const std::string& id){ return mRecords[id]; }

  RecordListPtr getRecordsByType(const std::string& t)
  {
    RecordListPtr r = RecordListPtr(new RecordList); //need pointer....
    for ( RecordMapItr iter = mRecords.begin(); iter != mRecords.end(); ++iter)
      if ( t == iter->second->getType() )
        r->push_back(iter->second);
    return r;
  }
};
