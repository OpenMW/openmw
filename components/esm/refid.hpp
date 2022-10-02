#ifndef OPENMW_COMPONENTS_ESM_REFID_HPP
#define OPENMW_COMPONENTS_ESM_REFID_HPP
#include <functional>
#include <iosfwd>
#include <string>

namespace ESM
{
    struct RefId
    {
        const static RefId sEmpty;
        void clear() { mId.clear(); }
        bool empty() const { return mId.empty(); }
        void swap(RefId& rhs) { mId.swap(rhs.mId); }
        bool operator==(const RefId& rhs) const { return mId == rhs.mId; }
        void operator=(const RefId& rhs) { mId = rhs.mId; }
        bool operator<(const RefId& rhs) const { return mId < rhs.mId; }
        bool operator>(const RefId& rhs) const { return mId > rhs.mId; }
        static bool ciEqual(const RefId& left, const RefId& right);

        friend std::ostream& operator<<(std::ostream& os, const RefId& dt);

        static RefId stringRefId(const std::string_view& id); //The 
        std::string& getRefIdString() { return mId; } 
        const std::string& getRefIdString() const { return mId; } 


    private:
        std::string mId;
    };
}

namespace std
{

    template <>
    struct hash<ESM::RefId>
    {
        std::size_t operator()(const ESM::RefId& k) const
        {
            using std::hash;
            using std::size_t;
            using std::string;

            return hash<string>()(k.getRefIdString());
        }
    };
}
#endif
