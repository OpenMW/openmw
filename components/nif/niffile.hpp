///Main header for reading .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

#include <stdexcept>
#include <vector>

#include <components/debug/debuglog.hpp>
#include <components/files/constrainedfilestream.hpp>

#include "record.hpp"

namespace Nif
{

struct File
{
    virtual ~File() = default;

    virtual Record *getRecord(size_t index) const = 0;

    virtual size_t numRecords() const = 0;

    virtual Record *getRoot(size_t index = 0) const = 0;

    virtual size_t numRoots() const = 0;

    virtual void setUseSkinning(bool skinning) = 0;

    virtual bool getUseSkinning() const = 0;

    virtual std::string getFilename() const = 0;
};

class NIFFile final : public File
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

    bool mUseSkinning;

    /// Parse the file
    void parse(Files::IStreamPtr stream);

    /// Get the file's version in a human readable form
    ///\returns A string containing a human readable NIF version number
    std::string printVersion(unsigned int version);

    ///Private Copy Constructor
    NIFFile (NIFFile const &);
    ///\overload
    void operator = (NIFFile const &);

public:
    /// Used if file parsing fails
    void fail(const std::string &msg) const
    {
        std::string err = " NIFFile Error: " + msg;
        err += "\nFile: " + filename;
        throw std::runtime_error(err);
    }
    /// Used when something goes wrong, but not catastrophically so
    void warn(const std::string &msg) const
    {
        Log(Debug::Warning) << " NIFFile Warning: " << msg << "\nFile: " << filename;
    }

    /// Open a NIF stream. The name is used for error messages.
    NIFFile(Files::IStreamPtr stream, const std::string &name);
    ~NIFFile();

    /// Get a given record
    Record *getRecord(size_t index) const override
    {
        Record *res = records.at(index);
        return res;
    }
    /// Number of records
    size_t numRecords() const override { return records.size(); }

    /// Get a given root
    Record *getRoot(size_t index=0) const override
    {
        Record *res = roots.at(index);
        return res;
    }
    /// Number of roots
    size_t numRoots() const override { return roots.size(); }

    /// Set whether there is skinning contained in this NIF file.
    /// @note This is just a hint for users of the NIF file and has no effect on the loading procedure.
    void setUseSkinning(bool skinning) override;

    bool getUseSkinning() const override;

    /// Get the name of the file
    std::string getFilename() const override { return filename; }
};
typedef std::shared_ptr<const Nif::NIFFile> NIFFilePtr;



} // Namespace
#endif
