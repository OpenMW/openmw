///Main header for reading .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

#include <stdexcept>
#include <vector>
#include <iostream>

#include "record.hpp"

namespace Nif
{

class NIFFile
{
    enum NIFVersion {
        VER_MW    = 0x04000002    // Morrowind NIFs
    };

    /// Nif file version
    unsigned int ver;

    /// File name, used for error messages and opening the file
    std::string filename;

    /// Record list
    std::vector<Record*> records;

    /// Root list.  This is a select portion of the pointers from records
    std::vector<Record*> roots;

    /// Parse the file
    void parse();

    /// Get the file's version in a human readable form
    ///\returns A string containing a human readable NIF version number
    std::string printVersion(unsigned int version);

    ///Private Copy Constructor
    NIFFile (NIFFile const &);
    ///\overload
    void operator = (NIFFile const &);

public:
    /// Used if file parsing fails
    void fail(const std::string &msg)
    {
        std::string err = " NIFFile Error: " + msg;
        err += "\nFile: " + filename;
        throw std::runtime_error(err);
    }
    /// Used when something goes wrong, but not catastrophically so
    void warn(const std::string &msg)
    {
        std::cerr << " NIFFile Warning: " << msg <<std::endl
                  << "File: " << filename <<std::endl;
    }

    /// Open a NIF stream. The name is used for error messages and opening the file.
    NIFFile(const std::string &name);
    ~NIFFile();

    /// Get a given record
    Record *getRecord(size_t index) const
    {
        Record *res = records.at(index);
        return res;
    }
    /// Number of records
    size_t numRecords() const { return records.size(); }

    /// Get a given root
    Record *getRoot(size_t index=0) const
    {
        Record *res = roots.at(index);
        return res;
    }
    /// Number of roots
    size_t numRoots() const { return roots.size(); }

    /// Get the name of the file
    std::string getFilename(){ return filename; }
};



} // Namespace
#endif
