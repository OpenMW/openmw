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

    virtual std::string getString(uint32_t index) const = 0;

    virtual void setUseSkinning(bool skinning) = 0;

    virtual bool getUseSkinning() const = 0;

    virtual std::string getFilename() const = 0;

    virtual unsigned int getVersion() const = 0;

    virtual unsigned int getUserVersion() const = 0;

    virtual unsigned int getBethVersion() const = 0;
};

class NIFFile final : public File
{
    /// File version, user version, Bethesda version
    unsigned int ver = 0;
    unsigned int userVer = 0;
    unsigned int bethVer = 0;

    /// File name, used for error messages and opening the file
    std::string filename;

    /// Record list
    std::vector<Record*> records;

    /// Root list.  This is a select portion of the pointers from records
    std::vector<Record*> roots;

    /// String table
    std::vector<std::string> strings;

    bool mUseSkinning = false;

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
    // For generic versions NIFStream::generateVersion() is used instead
    enum NIFVersion
    {
        VER_MW         = 0x04000002,    // 4.0.0.2. Main Morrowind NIF version.
        VER_CI         = 0x04020200,    // 4.2.2.0. Main Culpa Innata NIF version, also used in Civ4.
        VER_ZT2        = 0x0A000100,    // 10.0.1.0. Main Zoo Tycoon 2 NIF version, also used in Oblivion and Civ4.
        VER_OB_OLD     = 0x0A000102,    // 10.0.1.2. Main older Oblivion NIF version.
        VER_GAMEBRYO   = 0x0A010000,    // 10.1.0.0. Lots of games use it. The first version that has Gamebryo File Format header.
        VER_CIV4       = 0x14000004,    // 20.0.0.4. Main Civilization IV NIF version.
        VER_OB         = 0x14000005,    // 20.0.0.5. Main Oblivion NIF version.
        VER_BGS        = 0x14020007     // 20.2.0.7. Main Fallout 3/4/76/New Vegas and Skyrim/SkyrimSE NIF version.
    };
    enum BethVersion
    {
        BETHVER_FO3      = 34,          // Fallout 3
        BETHVER_FO4      = 130          // Fallout 4
    };

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

    /// Get a given string from the file's string table
    std::string getString(uint32_t index) const override
    {
        if (index == std::numeric_limits<uint32_t>::max())
            return std::string();
        return strings.at(index);
    }

    /// Set whether there is skinning contained in this NIF file.
    /// @note This is just a hint for users of the NIF file and has no effect on the loading procedure.
    void setUseSkinning(bool skinning) override;

    bool getUseSkinning() const override;

    /// Get the name of the file
    std::string getFilename() const override { return filename; }

    /// Get the version of the NIF format used
    unsigned int getVersion() const override { return ver; }

    /// Get the user version of the NIF format used
    unsigned int getUserVersion() const override { return userVer; }

    /// Get the Bethesda version of the NIF format used
    unsigned int getBethVersion() const override { return bethVer; }
};
using NIFFilePtr = std::shared_ptr<const Nif::NIFFile>;



} // Namespace
#endif
