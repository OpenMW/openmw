#ifndef COMPONENTS_AIDIALOGUE_JSON_HPP
#define COMPONENTS_AIDIALOGUE_JSON_HPP

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace AIDialogue
{
    /// \brief Minimal JSON builder for API requests
    /// Simple, dependency-free JSON generation for our specific use case
    class JSONBuilder
    {
    public:
        JSONBuilder& startObject();
        JSONBuilder& endObject();
        JSONBuilder& startArray(const std::string& key);
        JSONBuilder& endArray();
        JSONBuilder& addString(const std::string& key, const std::string& value);
        JSONBuilder& addInt(const std::string& key, int value);
        JSONBuilder& addFloat(const std::string& key, float value);
        JSONBuilder& addBool(const std::string& key, bool value);
        JSONBuilder& addRawValue(const std::string& key, const std::string& jsonValue);

        std::string build() const { return mStream.str(); }

    private:
        std::ostringstream mStream;
        bool mNeedComma = false;
        int mDepth = 0;

        void addComma();
        static std::string escapeString(const std::string& str);
    };

    /// \brief Minimal JSON parser for API responses
    /// Simple parser for extracting values from JSON
    class JSONParser
    {
    public:
        explicit JSONParser(const std::string& json);

        bool hasKey(const std::string& key) const;
        std::string getString(const std::string& key, const std::string& defaultValue = "") const;
        int getInt(const std::string& key, int defaultValue = 0) const;
        bool getBool(const std::string& key, bool defaultValue = false) const;

        /// Get nested object
        JSONParser getObject(const std::string& key) const;

        /// Get array element
        JSONParser getArrayElement(int index) const;

        /// Get array size
        int getArraySize() const;

        bool isValid() const { return mValid; }

    private:
        std::string mJson;
        bool mValid = false;

        std::string extractValue(const std::string& key) const;
        std::string findKeyValue(const std::string& json, const std::string& key) const;
        std::string trim(const std::string& str) const;
        std::string unquote(const std::string& str) const;
    };
}

#endif // COMPONENTS_AIDIALOGUE_JSON_HPP
