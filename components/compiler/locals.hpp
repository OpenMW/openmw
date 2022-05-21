#ifndef COMPILER_LOCALS_H_INCLUDED
#define COMPILER_LOCALS_H_INCLUDED

#include <vector>
#include <string>
#include <string_view>
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

            char getType (std::string_view name) const;
            ///< 's': short, 'l': long, 'f': float, ' ': does not exist.

            int getIndex (std::string_view name) const;
            ///< return index for local variable \a name (-1: does not exist).

            bool search (char type, std::string_view name) const;

            /// Return index for local variable \a name of type \a type (-1: variable does not
            /// exit).
            int searchIndex (char type, std::string_view name) const;

            const std::vector<std::string>& get (char type) const;

            void write (std::ostream& localFile) const;
            ///< write declarations to file.

            void declare (char type, std::string_view name);
            ///< declares a variable.

            void clear();
            ///< remove all declarations.
    };
}

#endif
