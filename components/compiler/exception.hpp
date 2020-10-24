#ifndef COMPILER_EXCEPTION_H_INCLUDED
#define COMPILER_EXCEPTION_H_INCLUDED

#include <exception>

namespace Compiler
{
    /// \brief Exception: Error while parsing the source

    class SourceException : public std::exception
    {
        public:

            const char *what() const noexcept override { return "Compile error";} 
            ///< Return error message
    };

    /// \brief Exception: File error

    class FileException : public SourceException
    {
        public:

            const char *what() const noexcept override { return "Can't read file"; }
            ///< Return error message
    };

    /// \brief Exception: EOF condition encountered

    class EOFException : public SourceException
    {
        public:

            const char *what() const noexcept override { return "End of file"; }
            ///< Return error message
    };
}

#endif
