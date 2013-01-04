#ifndef OPENMW_ESM_DIAL_H
#define OPENMW_ESM_DIAL_H

#include <string>
#include <vector>
#include <algorithm>

#include "loadinfo.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Dialogue topic and journal entries. The actual data is contained in
 * the INFO records following the DIAL.
 */

struct Dialogue
{
    enum Type
    {
        Topic = 0,
        Voice = 1,
        Greeting = 2,
        Persuasion = 3,
        Journal = 4,
        Deleted = -1
    };

    std::string mId;
    signed char mType;
    std::vector<DialInfo> mInfo;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

#ifdef OPENMW_ESM_ENABLE_CPP11_MOVE
	OPENMW_ESM_DEFINE_CPP11_MOVE_OPS(Dialogue)

	static void copy (Dialogue & d, Dialogue const & s)
	{
		d.mId   = s.mId;
		d.mType = s.mType;
		d.mInfo = s.mInfo;
	}
	static void move (Dialogue & d, Dialogue & s)
	{
		d.mId   = std::move (s.mId);
		d.mType = std::move (s.mType);
		d.mInfo = std::move (s.mInfo);
	}
#endif
};
}

/*
	custom swap to prevent memory allocations and deallocations for mId and mInfo
	while sorting
*/
namespace std
{
	template <> inline
	void swap <ESM::Dialogue> (ESM::Dialogue & Left, ESM::Dialogue & Right)
	{
#define _swp(id)	std::swap (Left.id,	Right.id);
		_swp(mId);
		_swp(mType);
		_swp(mInfo);
#undef  _swp
	}
}

#endif
