#ifndef COMPILER_EXCEPTION_H_INCLUDED
#define COMPILER_EXCEPTION_H_INCLUDED

#include <exception>

namespace Compiler
{
    /// \brief Exception: Error while parsing the source

    class SourceException : public std::exception
    {
        public:
        
            virtual const char *what() const throw() { return "Compile error";} 
            ///< Return error message
    };

    /// \brief Exception: File error

    class FileException : public SourceException
    {
        public:
        
            virtual const char *what() const throw() { return "Can't read file"; }
            ///< Return error message
    };

    /// \brief Exception: EOF condition encountered

    class EOFException : public SourceException
    {       
        public:
        
            virtual const char *what() const throw() { return "End of file"; }
            ///< Return error message
    };
}

#endif
