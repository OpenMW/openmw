/**
 * @file
 * @brief Main header for reading .nif files
 * @details 
 * @section LICENSE
 * OpenMW is distributed as free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 3, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * version 3 along with this program. If not, see
 * http://www.gnu.org/licenses/
 */

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

#include <stdexcept>
#include <vector>
#include <string>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "record.hpp"

namespace Nif
{

///Main class for handling .nif files
class NIFFile
{
    enum NIFVersion {
        VER_MW    = 0x04000002    // Morrowind NIFs
    };

    /// Nif file version
    int ver;

    /// File name, used for error messages and opening the file
    std::string filename;

    /// Record list
    /// \TODO See about converting this to not use pointers.
    std::vector<Record*> records;

    /**
     * @brief Root list
     * @details This is a select portion of the pointers from records, so don't worry about deallocating them.
     * \TODO See about converting this to not use pointers.
     */
    std::vector<Record*> roots;

    /**
     * @brief Parse the file.
     * @details This reads and parses the file.
     * It makes sure that the file is a proper Morrowind version NIF file.
     * It then reads all the individual records in the file, and stores pointers to the created objects in the "records" vector.
     * Next it determines which of these records are "roots" and stores pointers to those in the "roots" vector.
     * Finally, it does whatever post processing is needed.
     */
    void parse();

   /**
     * @name Private Copy Constructors
     * @brief Prevent anyone from copying one of these classes
     * @details These aren't even coded in.  Calling one of these functions in code will trigger a compiler error.
     * @{
     */
    NIFFile (NIFFile const &);
    void operator = (NIFFile const &);
    //@}

public:
    /**
     * @name Error handling functions
     * @{
     */
    /// Throw an error containing the filename and a message.
    void fail(const std::string &msg)
    {
        std::string err = "NIFFile Error: " + msg;
        err += "\nFile: " + filename;
        throw std::runtime_error(err);
    }
    /// Send a warning message and the filename to cerr, but do not throw an error.
    void warn(const std::string &msg)
    {
        std::cerr << "NIFFile Warning: " << msg <<std::endl
                  << "File: " << filename <<std::endl;
    }
    //@}

    /**
     * @brief Open a nif file
     * 
     * @param name A string containing the filename to be opened.
     */
    NIFFile(const std::string &name);
    ~NIFFile();

    /**
     * @brief Get a given record pointer
     * @details WARNING:  Do not free this pointer.  NIFFile handles that
     * 
     * @param index The number of the record to be retrieved.
     * @return A pointer to a record held by NIFFIle (DO NOT free this pointer)
     */
    Record *getRecord(size_t index) const
    {
        Record *res = records.at(index);
        assert(res != NULL);
        return res;
    }
    /// Number of records
    size_t numRecords() const { return records.size(); }

    /**
     * @brief Get a given (root) record pointer
     * @details WARNING:  Do not free this pointer.  NIFFile handles that
     * 
     * @param index The number of the (root) record to be retrieved.
     * @return A pointer to a (root) record held by NIFFIle (DO NOT free this pointer)
     */
    Record *getRoot(size_t index=0) const
    {
        Record *res = roots.at(index);
        assert(res != NULL);
        return res;
    }
    /// Number of roots
    size_t numRoots() const { return roots.size(); }
};



} // Namespace
#endif
