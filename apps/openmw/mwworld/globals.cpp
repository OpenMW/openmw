
#include "globals.hpp"

#include <stdexcept>

#include <components/misc/stringops.hpp>

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>

#include "esmstore.hpp"

namespace MWWorld
{
    Globals::Collection::const_iterator Globals::find (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);

        return iter;
    }

    Globals::Collection::iterator Globals::find (const std::string& name)
    {
        Collection::iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);

        return iter;
    }

    void Globals::fill (const MWWorld::ESMStore& store)
    {
        mVariables.clear();

        const MWWorld::Store<ESM::Global>& globals = store.get<ESM::Global>();

        for (MWWorld::Store<ESM::Global>::iterator iter = globals.begin(); iter!=globals.end();
            ++iter)
        {
            mVariables.insert (std::make_pair (iter->mId, iter->mValue));
        }
    }

    const ESM::Variant& Globals::operator[] (const std::string& name) const
    {
        return find (name)->second;
    }

    ESM::Variant& Globals::operator[] (const std::string& name)
    {
        return find (name)->second;
    }

    char Globals::getType (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            return ' ';

        switch (iter->second.getType())
        {
            case ESM::VT_Short: return 's';
            case ESM::VT_Long: return 'l';
            case ESM::VT_Float: return 'f';

            default: return ' ';
        }
    }

    int Globals::countSavedGameRecords() const
    {
        return mVariables.size();
    }

    void Globals::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (Collection::const_iterator iter (mVariables.begin()); iter!=mVariables.end(); ++iter)
        {
            writer.startRecord (ESM::REC_GLOB);
            writer.writeHNString ("NAME", iter->first);
            iter->second.write (writer, ESM::Variant::Format_Global);
            writer.endRecord (ESM::REC_GLOB);
        }
    }

    bool Globals::readRecord (ESM::ESMReader& reader,  uint32_t type)
    {
        if (type==ESM::REC_GLOB)
        {
            std::string id = reader.getHNString ("NAME");

            Collection::iterator iter = mVariables.find (Misc::StringUtils::lowerCase (id));

            if (iter!=mVariables.end())
                iter->second.read (reader, ESM::Variant::Format_Global);
            else
                reader.skipRecord();

            return true;
        }

        return false;
    }
}
