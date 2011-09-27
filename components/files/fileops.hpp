#ifndef COMPONENTS_FILES_FILEOPS_HPP
#define COMPONENTS_FILES_FILEOPS_HPP

namespace Files
{

///\brief Check if a given path is an existing file (not a directory)
///\param [in] name - filename
bool isFile(const char *name);

}

#endif /* COMPONENTS_FILES_FILEOPS_HPP */
