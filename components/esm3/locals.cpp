#include "locals.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

void Locals::load (ESMReader &esm)
{
    while (esm.isNextSub ("LOCA"))
    {
        std::string id = esm.getHString();

        Variant value;
        value.read (esm, Variant::Format_Local);

        mVariables.emplace_back (id, value);
    }
}

void Locals::save (ESMWriter &esm) const
{
    for (std::vector<std::pair<std::string, Variant> >::const_iterator iter (mVariables.begin());
        iter!=mVariables.end(); ++iter)
    {
        esm.writeHNString ("LOCA", iter->first);
        iter->second.write (esm, Variant::Format_Local);
    }
}

}
