#ifndef OPENMW_COMPONENTS_ESM_REFID_HPP
#define OPENMW_COMPONENTS_ESM_REFID_HPP

#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>

#include <components/esm4/formid.hpp>

namespace ESM
{
    // RefId is used to represent an Id that identifies an ESM record. These Ids can then be used in
    // ESM::Stores to find the actual record. These Ids can be serialized/de-serialized, stored on disk and remain
    // valid. They are used by ESM files, by records to reference other ESM records.
    struct RefId
    {
        const static RefId sEmpty;

        bool empty() const { return mId.empty(); }

        bool operator==(const RefId& rhs) const;

        bool operator<(const RefId& rhs) const;

        friend std::ostream& operator<<(std::ostream& os, const RefId& dt);

        // The 2 following functions are used to move back and forth between string and RefID. Used for hard coded
        // RefIds that are as string in the code. For serialization, and display. Using explicit conversions make it
        // very clear where in the code we need to convert from string to RefId and Vice versa.
        static RefId stringRefId(std::string_view id);
        static RefId formIdRefId(const ESM4::FormId id);
        const std::string& getRefIdString() const { return mId; }

    private:
        std::string mId;

        bool operator==(std::string_view rhs) const;

    public:
        template <std::size_t size>
        bool operator==(const char (&rhs)[size]) const
        {
            return *this == std::string_view(rhs);
        }
    };
}

namespace std
{

    template <>
    struct hash<ESM::RefId>
    {
        std::size_t operator()(const ESM::RefId& k) const;
    };
}
#endif
