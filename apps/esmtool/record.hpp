#ifndef OPENMW_ESMTOOL_RECORD_H
#define OPENMW_ESMTOOL_RECORD_H

#include <string>

#include <components/esm/records.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace EsmTool
{
    template <class T> class Record;

    class RecordBase
    {
    protected:
        std::string mId;
        uint32_t mFlags;
        ESM::NAME mType;
        bool mPrintPlain;

    public:
        RecordBase ()
          : mFlags(0)
          , mPrintPlain(false)
        {
        }

        virtual ~RecordBase() {}

        virtual std::string getId() const = 0;

        uint32_t getFlags() const {
            return mFlags;
        }

        void setFlags(uint32_t flags) {
            mFlags = flags;
        }

        ESM::NAME getType() const {
            return mType;
        }

        void setPrintPlain(bool plain) {
            mPrintPlain = plain;
        }

        virtual void load(ESM::ESMReader &esm) = 0;
        virtual void save(ESM::ESMWriter &esm) = 0;
        virtual void print() = 0;

        static RecordBase *create(ESM::NAME type);

        // just make it a bit shorter
        template <class T>
        Record<T> *cast() {
            return static_cast<Record<T> *>(this);
        }
    };

    template <class T>
    class Record : public RecordBase
    {
        T mData;
        bool mIsDeleted;

    public:
        Record()
            : mIsDeleted(false)
        {}

        std::string getId() const override {
            return mData.mId;
        }

        T &get() {
            return mData;
        }

        void save(ESM::ESMWriter &esm) override {
            mData.save(esm, mIsDeleted);
        }

        void load(ESM::ESMReader &esm) override {
            mData.load(esm, mIsDeleted);
        }

        void print() override;
    };
    
    template<> std::string Record<ESM::Cell>::getId() const;
    template<> std::string Record<ESM::Land>::getId() const;
    template<> std::string Record<ESM::MagicEffect>::getId() const;
    template<> std::string Record<ESM::Pathgrid>::getId() const;
    template<> std::string Record<ESM::Skill>::getId() const;

    template<> void Record<ESM::Activator>::print();
    template<> void Record<ESM::Potion>::print();
    template<> void Record<ESM::Armor>::print();
    template<> void Record<ESM::Apparatus>::print();
    template<> void Record<ESM::BodyPart>::print();
    template<> void Record<ESM::Book>::print();
    template<> void Record<ESM::BirthSign>::print();
    template<> void Record<ESM::Cell>::print();
    template<> void Record<ESM::Class>::print();
    template<> void Record<ESM::Clothing>::print();
    template<> void Record<ESM::Container>::print();
    template<> void Record<ESM::Creature>::print();
    template<> void Record<ESM::Dialogue>::print();
    template<> void Record<ESM::Door>::print();
    template<> void Record<ESM::Enchantment>::print();
    template<> void Record<ESM::Faction>::print();
    template<> void Record<ESM::Global>::print();
    template<> void Record<ESM::GameSetting>::print();
    template<> void Record<ESM::DialInfo>::print();
    template<> void Record<ESM::Ingredient>::print();
    template<> void Record<ESM::Land>::print();
    template<> void Record<ESM::CreatureLevList>::print();
    template<> void Record<ESM::ItemLevList>::print();
    template<> void Record<ESM::Light>::print();
    template<> void Record<ESM::Lockpick>::print();
    template<> void Record<ESM::Probe>::print();
    template<> void Record<ESM::Repair>::print();
    template<> void Record<ESM::LandTexture>::print();
    template<> void Record<ESM::MagicEffect>::print();
    template<> void Record<ESM::Miscellaneous>::print();
    template<> void Record<ESM::NPC>::print();
    template<> void Record<ESM::Pathgrid>::print();
    template<> void Record<ESM::Race>::print();
    template<> void Record<ESM::Region>::print();
    template<> void Record<ESM::Script>::print();
    template<> void Record<ESM::Skill>::print();
    template<> void Record<ESM::SoundGenerator>::print();
    template<> void Record<ESM::Sound>::print();
    template<> void Record<ESM::Spell>::print();
    template<> void Record<ESM::StartScript>::print();
    template<> void Record<ESM::Static>::print();
    template<> void Record<ESM::Weapon>::print();
}

#endif
