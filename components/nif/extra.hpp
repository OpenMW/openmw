/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (extra.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_EXTRA_HPP
#define OPENMW_COMPONENTS_NIF_EXTRA_HPP

#include "record.hpp"
#include "niffile.hpp"
#include "recordptr.hpp"

namespace Nif
{

/** A record that can have extra data. The extra data objects
    themselves decend from the Extra class, and all the extra data
    connected to an object form a linked list
*/
class Extra : public Record
{
public:
    ExtraPtr extra;

    void read(NIFStream *nif) { extra.read(nif); }
    void post(NIFFile *nif) { extra.post(nif); }
};

class NiVertWeightsExtraData : public Extra
{
public:
    void read(NIFStream *nif)
    {
        Extra::read(nif);

        // We should have s*4+2 == i, for some reason. Might simply be the
        // size of the rest of the record, unhelpful as that may be.
        /*int i =*/ nif->getInt();
        int s = nif->getUShort();

        nif->skip(s * sizeof(float)); // vertex weights I guess
    }
};

class NiTextKeyExtraData : public Extra
{
public:
    struct TextKey
    {
        float time;
        std::string text;
    };
    std::vector<TextKey> list;

    void read(NIFStream *nif)
    {
        Extra::read(nif);

        nif->getInt(); // 0

        int keynum = nif->getInt();
        list.resize(keynum);
        for(int i=0; i<keynum; i++)
        {
            list[i].time = nif->getFloat();
            list[i].text = nif->getString();
        }
    }
};

class NiStringExtraData : public Extra
{
public:
    /* Two known meanings:
       "MRK" - marker, only visible in the editor, not rendered in-game
       "NCO" - no collision
    */
    std::string string;

    void read(NIFStream *nif)
    {
        Extra::read(nif);

        nif->getInt(); // size of string + 4. Really useful...
        string = nif->getString();
    }
};

} // Namespace
#endif
