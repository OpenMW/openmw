#ifndef COMPILER_LOCALS_H_INCLUDED
#define COMPILER_LOCALS_H_INCLUDED

#include <vector>
#include <string>
#include <iosfwd>

namespace Compiler
{
    /// \brief Local variable declarations

    class Locals
    {
            std::vector<std::string> mShorts;
            std::vector<std::string> mLongs;
            std::vector<std::string> mFloats;

            std::vector<std::string>& get (char type);

        public:

            char getType (const std::string& name) const;
            ///< 's': short, 'l': long, 'f': float, ' ': does not exist.

            int getIndex (const std::string& name) const;
            ///< return index for local variable \a name (-1: does not exist).

            bool search (char type, const std::string& name) const;

            /// Return index for local variable \a name of type \a type (-1: variable does not
            /// exit).
            int searchIndex (char type, const std::string& name) const;

            const std::vector<std::string>& get (char type) const;

            void write (std::ostream& localFile) const;
            ///< write declarations to file.

            void declare (char type, const std::string& name);
            ///< declares a variable.

            void clear();
            ///< remove all declarations.
    };
}

#endif
