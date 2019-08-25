/*
  Copyright (C) 2016, 2018 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef ESM4_SBSP_H
#define ESM4_SBSP_H

#include <string>
#include <cstdint>

namespace ESM4
{
    class Reader;
    typedef std::uint32_t FormId;

    struct Subspace
    {
        struct Dimension
        {
            float x;
            float y;
            float z;
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        Dimension mDimension;

        Subspace();
        virtual ~Subspace();

        virtual void load(Reader& reader);
        //virtual void save(Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_SBSP_H
