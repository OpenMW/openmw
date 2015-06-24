#ifndef COMPONENTS_GAMEPLAY_STORE_H
#define COMPONENTS_GAMEPLAY_STORE_H

#include <vector>
#include <string>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMWriter;
    class ESMReader;
    struct Spell;
    struct Skill;
    struct MagicEffect;
}

namespace GamePlay
{
    // moved from apps/openmw/mwworld/store.hpp
    struct StoreBase
    {
        virtual ~StoreBase() {}

        virtual void setUp() {}
        virtual void listIdentifier(std::vector<std::string> &list) const {}

        virtual size_t getSize() const = 0;
        virtual int getDynamicSize() const { return 0; }
        virtual void load(ESM::ESMReader &esm, const std::string &id) = 0;

        virtual bool eraseStatic(const std::string &id) {return false;}
        virtual void clearDynamic() {}

        virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress) const {}

        virtual void read (ESM::ESMReader& reader, const std::string& id) {}
        ///< Read into dynamic storage
    };

    // moved from apps/openmw/mwworld/store.hpp
    template <class T>
    class SharedIterator
    {
        typedef typename std::vector<T *>::const_iterator Iter;

        Iter mIter;

    public:
        SharedIterator() {}

        SharedIterator(const SharedIterator &orig)
          : mIter(orig.mIter)
        {}

        SharedIterator(const Iter &iter)
          : mIter(iter)
        {}

        SharedIterator &operator++() {
            ++mIter;
            return *this;
        }

        SharedIterator operator++(int) {
            SharedIterator iter = *this;
            ++mIter;

            return iter;
        }

        SharedIterator &operator--() {
            --mIter;
            return *this;
        }

        SharedIterator operator--(int) {
            SharedIterator iter = *this;
            --mIter;

            return iter;
        }

        bool operator==(const SharedIterator &x) const {
            return mIter == x.mIter;
        }

        bool operator!=(const SharedIterator &x) const {
            return !(*this == x);
        }

        const T &operator*() const {
            return **mIter;
        }

        const T *operator->() const {
            return &(**mIter);
        }
    };

    // interface class for sharing the autocalc component between OpenMW and OpenCS
    template <class T>
    class CommonStore : public StoreBase
    {

    public:
        typedef SharedIterator<T> iterator;

        virtual iterator begin() const = 0;

        virtual iterator end() const = 0;

        virtual const T *find(const std::string &id) const = 0;
	};

    // interface class for sharing the autocalc component between OpenMW and OpenCS
    class StoreWrap
    {

    public:
        StoreWrap() {}
        virtual ~StoreWrap() {}

        virtual int findGmstInt(const std::string& gmst) const = 0;

        virtual float findGmstFloat(const std::string& gmst) const = 0;

        virtual const ESM::Skill *findSkill(int index) const = 0;

        virtual const ESM::MagicEffect* findMagicEffect(int id) const = 0;

        virtual const CommonStore<ESM::Spell>& getSpells() const = 0;
    };
}
#endif // COMPONENTS_GAMEPLAY_STORE_H
