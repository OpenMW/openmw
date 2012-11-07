#include "store.hpp"

namespace MWWorld
{
    template <>
    void Store<ESM::Dialogue>::load(ESM::ESMReader &esm, const std::string &id) {
        mStatic.push_back(ESM::Dialogue());
        mStatic.back().mId = id;
        mStatic.back().load(esm);
    }

    template <>
    void Store<ESM::Script>::load(ESM::ESMReader &esm, const std::string &id) {
        mStatic.push_back(ESM::Script());
        mStatic.back().load(esm);
        StringUtils::toLower(mStatic.back().mId);
    }
}
