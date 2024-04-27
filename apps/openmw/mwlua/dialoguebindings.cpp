#include "dialoguebindings.hpp"
#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/store.hpp"
#include "context.hpp"
#include "object.hpp"
#include <algorithm>
#include <components/esm3/loaddial.hpp>
#include <components/lua/luastate.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/vfs/pathutil.hpp>

namespace
{
    template <ESM::Dialogue::Type filter>
    class FilteredDialogueStore
    {
        const MWWorld::Store<ESM::Dialogue>& mDialogueStore;

        const ESM::Dialogue* foundDialogueFilteredOut(const ESM::Dialogue* possibleResult) const
        {
            if (possibleResult && possibleResult->mType == filter)
            {
                return possibleResult;
            }
            return nullptr;
        }

    public:
        FilteredDialogueStore()
            : mDialogueStore{ MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>() }
        {
        }

        class FilteredDialogueIterator
        {
            using DecoratedIterator = MWWorld::Store<ESM::Dialogue>::iterator;
            DecoratedIterator mIter;
            DecoratedIterator mEndIter;

        public:
            using iterator_category = DecoratedIterator::iterator_category;
            using value_type = DecoratedIterator::value_type;
            using difference_type = DecoratedIterator::difference_type;
            using pointer = DecoratedIterator::pointer;
            using reference = DecoratedIterator::reference;

            FilteredDialogueIterator(const DecoratedIterator& pointingIterator, const DecoratedIterator& end)
                : mIter{ pointingIterator }
                , mEndIter{ end }
            {
            }

            FilteredDialogueIterator& operator++()
            {
                if (mIter == mEndIter)
                {
                    return *this;
                }

                do
                {
                    ++mIter;
                } while (mIter != mEndIter && mIter->mType != filter);
                return *this;
            }

            FilteredDialogueIterator operator++(int)
            {
                FilteredDialogueIterator iter = *this;
                ++(*this);
                return iter;
            }

            FilteredDialogueIterator& operator+=(difference_type advance)
            {
                while (advance > 0 && mIter != mEndIter)
                {
                    ++(*this);
                    --advance;
                }
                return *this;
            }

            bool operator==(const FilteredDialogueIterator& x) const { return mIter == x.mIter; }

            bool operator!=(const FilteredDialogueIterator& x) const { return !(*this == x); }

            const value_type& operator*() const { return *mIter; }

            const value_type* operator->() const { return &(*mIter); }
        };

        using iterator = FilteredDialogueIterator;

        const ESM::Dialogue* search(const ESM::RefId& id) const
        {
            return foundDialogueFilteredOut(mDialogueStore.search(id));
        }

        const ESM::Dialogue* at(size_t index) const
        {
            auto result = begin();
            result += index;

            if (result == end())
            {
                return nullptr;
            }

            return &(*result);
        }

        size_t getSize() const
        {
            return std::count_if(
                mDialogueStore.begin(), mDialogueStore.end(), [](const auto& d) { return d.mType == filter; });
        }

        iterator begin() const
        {
            iterator result{ mDialogueStore.begin(), mDialogueStore.end() };
            while (result != end() && result->mType != filter)
            {
                ++result;
            }
            return result;
        }

        iterator end() const { return iterator{ mDialogueStore.end(), mDialogueStore.end() }; }
    };

    template <ESM::Dialogue::Type filter>
    void prepareBindingsForDialogueRecordStores(sol::table& table, const MWLua::Context& context)
    {
        using StoreT = FilteredDialogueStore<filter>;

        sol::state_view& lua = context.mLua->sol();
        sol::usertype<StoreT> storeBindingsClass
            = lua.new_usertype<StoreT>("ESM3_Dialogue_Type" + std::to_string(filter) + " Store");
        storeBindingsClass[sol::meta_function::to_string] = [](const StoreT& store) {
            return "{" + std::to_string(store.getSize()) + " ESM3_Dialogue_Type" + std::to_string(filter) + " records}";
        };
        storeBindingsClass[sol::meta_function::length] = [](const StoreT& store) { return store.getSize(); };
        storeBindingsClass[sol::meta_function::index] = sol::overload(
            [](const StoreT& store, size_t index) -> const ESM::Dialogue* {
                if (index == 0)
                {
                    return nullptr;
                }
                return store.at(index - 1);
            },
            [](const StoreT& store, std::string_view id) -> const ESM::Dialogue* {
                return store.search(ESM::RefId::deserializeText(Misc::StringUtils::lowerCase(id)));
            });
        storeBindingsClass[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        storeBindingsClass[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();

        table["records"] = StoreT{};
    }

    struct DialogueInfos
    {
        ESM::RefId mDialogueRecordId;

        const ESM::Dialogue* getDialogueRecord() const
        {
            const auto& dialogueStore{ MWBase::Environment::get().getESMStore()->get<ESM::Dialogue>() };
            return dialogueStore.search(mDialogueRecordId);
        }
    };

    void prepareBindingsForDialogueRecord(sol::state_view& lua)
    {
        auto recordBindingsClass = lua.new_usertype<ESM::Dialogue>("ESM3_Dialogue");
        recordBindingsClass[sol::meta_function::to_string]
            = [](const ESM::Dialogue& rec) { return "ESM3_Dialogue[" + rec.mId.toDebugString() + "]"; };
        recordBindingsClass["id"]
            = sol::readonly_property([](const ESM::Dialogue& rec) { return rec.mId.serializeText(); });
        recordBindingsClass["name"]
            = sol::readonly_property([](const ESM::Dialogue& rec) -> std::string_view { return rec.mStringId; });
        recordBindingsClass["questName"] = sol::readonly_property([lua](const ESM::Dialogue& rec) -> sol::object {
            if (rec.mType != ESM::Dialogue::Type::Journal)
            {
                return sol::nil;
            }
            for (const auto& mwDialogueInfo : rec.mInfo)
            {
                if (mwDialogueInfo.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Name)
                {
                    return sol::make_object(lua, mwDialogueInfo.mResponse);
                }
            }
            return sol::nil;
        });
        recordBindingsClass["infos"]
            = sol::readonly_property([](const ESM::Dialogue& rec) { return DialogueInfos{ rec.mId }; });
    }

    void prepareBindingsForDialogueRecordInfoList(sol::state_view& lua)
    {
        auto recordInfosBindingsClass = lua.new_usertype<DialogueInfos>("ESM3_Dialogue_Infos");
        recordInfosBindingsClass[sol::meta_function::to_string] = [lua](const DialogueInfos& store) -> sol::object {
            if (const ESM::Dialogue* dialogueRecord = store.getDialogueRecord())
            {
                return sol::make_object(lua,
                    "{" + std::to_string(dialogueRecord->mInfo.size()) + " ESM3_Dialogue["
                        + dialogueRecord->mId.toDebugString() + "] info elements}");
            }
            return sol::nil;
        };
        recordInfosBindingsClass[sol::meta_function::length] = [](const DialogueInfos& store) {
            const ESM::Dialogue* dialogueRecord = store.getDialogueRecord();
            return dialogueRecord ? dialogueRecord->mInfo.size() : 0;
        };
        recordInfosBindingsClass[sol::meta_function::index]
            = [](const DialogueInfos& store, size_t index) -> const ESM::DialInfo* {
            const ESM::Dialogue* dialogueRecord = store.getDialogueRecord();
            if (!dialogueRecord || index == 0 || index > dialogueRecord->mInfo.size())
            {
                return nullptr;
            }
            ESM::Dialogue::InfoContainer::const_iterator iter{ dialogueRecord->mInfo.cbegin() };
            std::advance(iter, index - 1);
            return &(*iter);
        };
        recordInfosBindingsClass[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
        recordInfosBindingsClass[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
    }

    void prepareBindingsForDialogueRecordInfoListElement(sol::state_view& lua)
    {
        auto recordInfoBindingsClass = lua.new_usertype<ESM::DialInfo>("ESM3_Dialogue_Info");

        recordInfoBindingsClass[sol::meta_function::to_string]
            = [](const ESM::DialInfo& rec) { return "ESM3_Dialogue_Info[" + rec.mId.toDebugString() + "]"; };
        recordInfoBindingsClass["id"]
            = sol::readonly_property([](const ESM::DialInfo& rec) { return rec.mId.serializeText(); });
        recordInfoBindingsClass["text"]
            = sol::readonly_property([](const ESM::DialInfo& rec) -> std::string_view { return rec.mResponse; });
        recordInfoBindingsClass["questStage"] = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
            if (rec.mData.mType != ESM::Dialogue::Type::Journal)
            {
                return sol::nil;
            }
            return sol::make_object(lua, rec.mData.mJournalIndex);
        });
        recordInfoBindingsClass["isQuestFinished"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType != ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  return sol::make_object(lua, rec.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Finished);
              });
        recordInfoBindingsClass["isQuestRestart"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType != ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  return sol::make_object(lua, rec.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Restart);
              });
        recordInfoBindingsClass["isQuestName"] = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
            if (rec.mData.mType != ESM::Dialogue::Type::Journal)
            {
                return sol::nil;
            }
            return sol::make_object(lua, rec.mQuestStatus == ESM::DialInfo::QuestStatus::QS_Name);
        });
        recordInfoBindingsClass["filterActorId"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mActor.serializeText();
                  return result.empty() ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterActorRace"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mRace.serializeText();
                  return result.empty() ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterActorClass"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mClass.serializeText();
                  return result.empty() ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterActorFaction"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mFaction.serializeText();
                  return result.empty() ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterActorFactionRank"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mData.mRank;
                  return result == -1 ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterActorCell"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mCell.serializeText();
                  return result.empty() ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterActorDisposition"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  return sol::make_object(lua, rec.mData.mDisposition);
              });
        recordInfoBindingsClass["filterActorGender"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mData.mGender;
                  return result == -1 ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterPlayerFaction"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mPcFaction.serializeText();
                  return result.empty() ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["filterPlayerFactionRank"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  if (rec.mData.mType == ESM::Dialogue::Type::Journal)
                  {
                      return sol::nil;
                  }
                  const auto result = rec.mData.mPCrank;
                  return result == -1 ? sol::nil : sol::make_object(lua, result);
              });
        recordInfoBindingsClass["sound"] = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
            if (rec.mData.mType == ESM::Dialogue::Type::Journal || rec.mSound == "")
            {
                return sol::nil;
            }
            return sol::make_object(
                lua, Misc::ResourceHelpers::correctSoundPath(VFS::Path::Normalized(rec.mSound)).value());
        });
        recordInfoBindingsClass["resultScript"]
            = sol::readonly_property([lua](const ESM::DialInfo& rec) -> sol::object {
                  return rec.mResultScript.empty() ? sol::nil : sol::make_object(lua, rec.mResultScript);
              });
    }

    void prepareBindingsForDialogueRecords(sol::state_view& lua)
    {
        prepareBindingsForDialogueRecord(lua);
        prepareBindingsForDialogueRecordInfoList(lua);
        prepareBindingsForDialogueRecordInfoListElement(lua);
    }
}

namespace sol
{
    template <ESM::Dialogue::Type filter>
    struct is_automagical<FilteredDialogueStore<filter>> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Dialogue> : std::false_type
    {
    };
    template <>
    struct is_automagical<DialogueInfos> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::DialInfo> : std::false_type
    {
    };
}

namespace MWLua
{
    sol::table initCoreDialogueBindings(const Context& context)
    {
        sol::state_view& lua = context.mLua->sol();
        sol::table api(lua, sol::create);

        sol::table journalTable(lua, sol::create);
        sol::table topicTable(lua, sol::create);
        sol::table greetingTable(lua, sol::create);
        sol::table persuasionTable(lua, sol::create);
        sol::table voiceTable(lua, sol::create);
        prepareBindingsForDialogueRecordStores<ESM::Dialogue::Type::Journal>(journalTable, context);
        prepareBindingsForDialogueRecordStores<ESM::Dialogue::Type::Topic>(topicTable, context);
        prepareBindingsForDialogueRecordStores<ESM::Dialogue::Type::Greeting>(greetingTable, context);
        prepareBindingsForDialogueRecordStores<ESM::Dialogue::Type::Persuasion>(persuasionTable, context);
        prepareBindingsForDialogueRecordStores<ESM::Dialogue::Type::Voice>(voiceTable, context);
        api["journal"] = journalTable;
        api["topic"] = topicTable;
        api["greeting"] = greetingTable;
        api["persuasion"] = persuasionTable;
        api["voice"] = voiceTable;

        prepareBindingsForDialogueRecords(lua);

        return LuaUtil::makeReadOnly(api);
    }
}
