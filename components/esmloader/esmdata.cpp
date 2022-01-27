#include "esmdata.hpp"
#include "lessbyid.hpp"
#include "record.hpp"

#include <components/esm/defs.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/variant.hpp>
#include <components/misc/stringops.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace EsmLoader
{
    namespace
    {
        template <class F>
        auto returnAs(F&& f)
        {
            using Result = decltype(std::forward<F>(f)(ESM::Static {}));
            if constexpr (!std::is_same_v<Result, void>)
                return Result {};
        }

        template <class T, class F>
        auto withStatic(std::string_view refId, const std::vector<T>& values, F&& f)
        {
            const auto it = std::lower_bound(values.begin(), values.end(), refId, LessById {});

            if (it == values.end() || it->mId != refId)
                return returnAs(std::forward<F>(f));

            return std::forward<F>(f)(*it);
        }

        template <class F>
        auto withStatic(std::string_view refId, ESM::RecNameInts type, const EsmData& content, F&& f)
        {
            switch (type)
            {
                case ESM::REC_ACTI: return withStatic(refId, content.mActivators, std::forward<F>(f));
                case ESM::REC_CONT: return withStatic(refId, content.mContainers, std::forward<F>(f));
                case ESM::REC_DOOR: return withStatic(refId, content.mDoors, std::forward<F>(f));
                case ESM::REC_STAT: return withStatic(refId, content.mStatics, std::forward<F>(f));
                default: break;
            }

            return returnAs(std::forward<F>(f));
        }
    }

    EsmData::~EsmData() {}

    std::string_view getModel(const EsmData& content, std::string_view refId, ESM::RecNameInts type)
    {
        return withStatic(refId, type, content, [] (const auto& v) { return std::string_view(v.mModel); });
    }

    ESM::Variant getGameSetting(const std::vector<ESM::GameSetting>& records, std::string_view id)
    {
        const std::string lower = Misc::StringUtils::lowerCase(id);
        auto it = std::lower_bound(records.begin(), records.end(), lower, LessById {});
        if (it == records.end() || it->mId != lower)
            throw std::runtime_error("Game settings \"" + std::string(id) + "\" is not found");
        return it->mValue;
    }
}
