#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

#include <string>

namespace Compiler
{
    class Extensions;

    class Context
    {
            const Extensions *mExtensions;

        public:

            Context() : mExtensions (nullptr) {}

            virtual ~Context() = default;

            virtual bool canDeclareLocals() const = 0;
            ///< Is the compiler allowed to declare local variables?

            void setExtensions (const Extensions *extensions = nullptr)
            {
                mExtensions = extensions;
            }

            const Extensions *getExtensions() const
            {
                return mExtensions;
            }

            virtual char getGlobalType (const std::string& name) const = 0;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.

            virtual std::pair<char, bool> getMemberType (const std::string& name,
                const std::string& id) const = 0;
            ///< Return type of member variable \a name in script \a id or in script of reference of
            /// \a id
            /// \return first: 'l: long, 's': short, 'f': float, ' ': does not exist.
            /// second: true: script of reference

            virtual bool isId (const std::string& name) const = 0;
            ///< Does \a name match an ID, that can be referenced?

            virtual bool isJournalId (const std::string& name) const = 0;
            ///< Does \a name match a journal ID?
    };
}

#endif
