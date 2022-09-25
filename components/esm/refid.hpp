#ifndef OPENMW_COMPONENTS_ESM_REFID_HPP
#define OPENMW_COMPONENTS_ESM_REFID_HPP
#include <string>
#include <iosfwd>
#include <functional>

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
        bool operator <(const RefId& rhs) const
        {
            return mId < rhs.mId;
        }
        bool operator >(const RefId& rhs) const
        {
            return mId > rhs.mId;
        }

        friend std::ostream& operator<<(std::ostream& os, const RefId& dt);

        static RefId stringRefId(
            const std::string_view& id); // This makes it very visible the places where I had to convert from string to Refid

        std::string& getRefIdString() { return mId; } // Same thing
        const std::string& getRefIdString() const { return mId; } // Same thing

        static bool ciEqual(const RefId& left, const RefId& right);

    private:
        std::string mId;
    };
}

namespace std {

    template <>
    struct hash<ESM::RefId>
    {
        std::size_t operator()(const ESM::RefId& k) const
        {
            using std::size_t;
            using std::hash;
            using std::string;

            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:

            return hash<string>()(k.getRefIdString());
        }
    };
}
#endif
