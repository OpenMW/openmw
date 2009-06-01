
typedef void* D_MmFile;

// These functions are implemented in util/c_mmfile.d
extern "C"
{
  // Open a new memory mapped file
  D_MmFile mmf_open(const char *fileName);

  // Close a file. Do not use the handle after calling this function,
  // as the object gets deleted
  void mmf_close(D_MmFile mmf);

  // Map a region of the file. Do NOT attempt to access several
  // regions at once. Map will almost always unmap the current mapping
  // (thus making all current pointers invalid) when a new map is
  // requested.
  void* mmf_map(D_MmFile mmf, int64_t offset, int64_t size);
}

// This struct allows you to open, read and close a memory mapped
// file. It uses the D MmFile class to achieve platform independence
// and an abstract interface.
struct MmFile
{
  MmFile(const std::string &file)
  {
    mmf = mmf_open(file.c_str());
  }

  ~MmFile()
  {
    mmf_close(mmf);
  }

  void *map(int64_t offset, int64_t size)
  {
    return mmf_map(mmf, offset, size);
  }

  private:

  D_MmFile mmf;
};
