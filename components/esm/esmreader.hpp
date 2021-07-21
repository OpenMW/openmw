#ifndef OPENMW_ESM_READER_H
#define OPENMW_ESM_READER_H

#include <cstdint>
#include <cassert>
#include <vector>
#include <sstream>

#include <components/files/constrainedfilestream.hpp>

#include <components/misc/stringops.hpp>

#include <components/to_utf8/to_utf8.hpp>

#include "esmcommon.hpp"
#include "loadtes3.hpp"

namespace ESM {

class ESMReader
{
public:

  ESMReader();

  /*************************************************************************
   *
   *  Information retrieval
   *
   *************************************************************************/

  int getVer() const { return mHeader.mData.version; }
  int getRecordCount() const { return mHeader.mData.records; }
  float getFVer() const { return (mHeader.mData.version == VER_12) ? 1.2f : 1.3f; }
  const std::string getAuthor() const { return mHeader.mData.author; }
  const std::string getDesc() const { return mHeader.mData.desc; }
  const std::vector<Header::MasterData> &getGameFiles() const { return mHeader.mMaster; }
  const Header& getHeader() const { return mHeader; }
  int getFormat() const { return mHeader.mFormat; };
  const NAME &retSubName() const { return mCtx.subName; }
  uint32_t getSubSize() const { return mCtx.leftSub; }
  std::string getName() const {return mCtx.filename; };

  /*************************************************************************
   *
   *  Opening and closing
   *
   *************************************************************************/

  /** Save the current file position and information in a ESM_Context
      struct
   */
  ESM_Context getContext();

  /** Restore a previously saved context */
  void restoreContext(const ESM_Context &rc);

  /** Close the file, resets all information. After calling close()
      the structure may be reused to load a new file.
  */
  void close();

  /// Raw opening. Opens the file and sets everything up but doesn't
  /// parse the header.
  void openRaw(Files::IStreamPtr _esm, const std::string &name);

  /// Load ES file from a new stream, parses the header. Closes the
  /// currently open file first, if any.
  void open(Files::IStreamPtr _esm, const std::string &name);

  void open(const std::string &file);

  void openRaw(const std::string &filename);

  /// Get the current position in the file. Make sure that the file has been opened!
  size_t getFileOffset() const { return mEsm->tellg(); };

  // This is a quick hack for multiple esm/esp files. Each plugin introduces its own
  //  terrain palette, but ESMReader does not pass a reference to the correct plugin
  //  to the individual load() methods. This hack allows to pass this reference
  //  indirectly to the load() method.
  void setIndex(const int index) { mCtx.index = index;}
  int getIndex() {return mCtx.index;}

  void setGlobalReaderList(std::vector<ESMReader> *list) {mGlobalReaderList = list;}
  std::vector<ESMReader> *getGlobalReaderList() {return mGlobalReaderList;}

  void addParentFileIndex(int index) { mCtx.parentFileIndices.push_back(index); }
  const std::vector<int>& getParentFileIndices() const { return mCtx.parentFileIndices; }

  /*************************************************************************
   *
   *  Medium-level reading shortcuts
   *
   *************************************************************************/

  // Read data of a given type, stored in a subrecord of a given name
  template <typename X>
  void getHNT(X &x, const char* name)
  {
    getSubNameIs(name);
    getHT(x);
  }

  // Optional version of getHNT
  template <typename X>
  void getHNOT(X &x, const char* name)
  {
      if(isNextSub(name))
          getHT(x);
  }

  // Version with extra size checking, to make sure the compiler
  // doesn't mess up our struct padding.
  template <typename X>
  void getHNT(X &x, const char* name, int size)
  {
      assert(sizeof(X) == size);
      getSubNameIs(name);
      getHT(x);
  }

  template <typename X>
  void getHNOT(X &x, const char* name, int size)
  {
      assert(sizeof(X) == size);
      if(isNextSub(name))
          getHT(x);
  }

  // Get data of a given type/size, including subrecord header
  template <typename X>
  void getHT(X &x)
  {
      getSubHeader();
      if (mCtx.leftSub != sizeof(X))
          reportSubSizeMismatch(sizeof(X), mCtx.leftSub);
      getT(x);
  }

  // Version with extra size checking, to make sure the compiler
  // doesn't mess up our struct padding.
  template <typename X>
  void getHT(X &x, int size)
  {
      assert(sizeof(X) == size);
      getHT(x);
  }

  // Read a string by the given name if it is the next record.
  std::string getHNOString(const char* name);

  // Read a string with the given sub-record name
  std::string getHNString(const char* name);

  // Read a string, including the sub-record header (but not the name)
  std::string getHString();

  // Read the given number of bytes from a subrecord
  void getHExact(void*p, int size);

  // Read the given number of bytes from a named subrecord
  void getHNExact(void*p, int size, const char* name);

  /*************************************************************************
   *
   *  Low level sub-record methods
   *
   *************************************************************************/

  // Get the next subrecord name and check if it matches the parameter
  void getSubNameIs(const char* name);

  /** Checks if the next sub record name matches the parameter. If it
      does, it is read into 'subName' just as if getSubName() was
      called. If not, the read name will still be available for future
      calls to getSubName(), isNextSub() and getSubNameIs().
   */
  bool isNextSub(const char* name);

  bool peekNextSub(const char* name);

  // Store the current subrecord name for the next call of getSubName()
  void cacheSubName() {mCtx.subCached = true; };

  // Read subrecord name. This gets called a LOT, so I've optimized it
  // slightly.
  void getSubName();

  // Skip current sub record, including header (but not including
  // name.)
  void skipHSub();

  // Skip sub record and check its size
  void skipHSubSize(int size);

  // Skip all subrecords until the given subrecord or no more subrecords remaining
  void skipHSubUntil(const char* name);

  /* Sub-record header. This updates leftRec beyond the current
     sub-record as well. leftSub contains size of current sub-record.
  */
  void getSubHeader();

  /*************************************************************************
   *
   *  Low level record methods
   *
   *************************************************************************/

  // Get the next record name
  NAME getRecName();

  // Skip the rest of this record. Assumes the name and header have
  // already been read
  void skipRecord();

  /* Read record header. This updatesleftFile BEYOND the data that
     follows the header, ie beyond the entire record. You should use
     leftRec to orient yourself inside the record itself.
  */
  void getRecHeader() { getRecHeader(mRecordFlags); }
  void getRecHeader(uint32_t &flags);

  bool hasMoreRecs() const { return mCtx.leftFile > 0; }
  bool hasMoreSubs() const { return mCtx.leftRec > 0; }


  /*************************************************************************
   *
   *  Lowest level data reading and misc methods
   *
   *************************************************************************/

  template <typename X>
  void getT(X &x) { getExact(&x, sizeof(X)); }

  void getExact(void* x, int size) { mEsm->read((char*)x, size); }
  void getName(NAME &name) { getT(name); }
  void getUint(uint32_t &u) { getT(u); }

  // Read the next 'size' bytes and return them as a string. Converts
  // them from native encoding to UTF8 in the process.
  std::string getString(int size);

  void skip(int bytes) { mEsm->seekg(getFileOffset()+bytes); };

  /// Used for error handling
  [[noreturn]] void fail(const std::string &msg);

  /// Sets font encoder for ESM strings
  void setEncoder(ToUTF8::Utf8Encoder* encoder) { mEncoder = encoder; };

  /// Get record flags of last record
  unsigned int getRecordFlags() { return mRecordFlags; }

  size_t getFileSize() const { return mFileSize; }

private:
  [[noreturn]] void reportSubSizeMismatch(size_t want, size_t got) {
          fail("record size mismatch, requested " +
                  std::to_string(want) +
                  ", got" +
                  std::to_string(got));
  }

  void clearCtx();

  Files::IStreamPtr mEsm;

  ESM_Context mCtx;

  unsigned int mRecordFlags;

  // Special file signifier (see SpecialFile enum above)

  // Buffer for ESM strings
  std::vector<char> mBuffer;

  Header mHeader;

  std::vector<ESMReader> *mGlobalReaderList;
  ToUTF8::Utf8Encoder* mEncoder;

  size_t mFileSize;

};
}
#endif
