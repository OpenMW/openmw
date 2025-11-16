#include "json.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace AIDialogue
{
    // JSONBuilder implementation

    void JSONBuilder::addComma()
    {
        if (mNeedComma)
        {
            mStream << ",";
        }
        mNeedComma = true;
    }

    std::string JSONBuilder::escapeString(const std::string& str)
    {
        std::string escaped;
        escaped.reserve(str.size());

        for (char c : str)
        {
            switch (c)
            {
                case '"':
                    escaped += "\\\"";
                    break;
                case '\\':
                    escaped += "\\\\";
                    break;
                case '\b':
                    escaped += "\\b";
                    break;
                case '\f':
                    escaped += "\\f";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        escaped += buf;
                    }
                    else
                    {
                        escaped += c;
                    }
            }
        }
        return escaped;
    }

    JSONBuilder& JSONBuilder::startObject()
    {
        if (mDepth > 0)
            addComma();
        mStream << "{";
        mNeedComma = false;
        mDepth++;
        return *this;
    }

    JSONBuilder& JSONBuilder::endObject()
    {
        mStream << "}";
        mNeedComma = true;
        mDepth--;
        return *this;
    }

    JSONBuilder& JSONBuilder::startArray(const std::string& key)
    {
        addComma();
        mStream << "\"" << escapeString(key) << "\":[";
        mNeedComma = false;
        mDepth++;
        return *this;
    }

    JSONBuilder& JSONBuilder::endArray()
    {
        mStream << "]";
        mNeedComma = true;
        mDepth--;
        return *this;
    }

    JSONBuilder& JSONBuilder::addString(const std::string& key, const std::string& value)
    {
        addComma();
        mStream << "\"" << escapeString(key) << "\":\"" << escapeString(value) << "\"";
        return *this;
    }

    JSONBuilder& JSONBuilder::addInt(const std::string& key, int value)
    {
        addComma();
        mStream << "\"" << escapeString(key) << "\":" << value;
        return *this;
    }

    JSONBuilder& JSONBuilder::addFloat(const std::string& key, float value)
    {
        addComma();
        mStream << "\"" << escapeString(key) << "\":" << value;
        return *this;
    }

    JSONBuilder& JSONBuilder::addBool(const std::string& key, bool value)
    {
        addComma();
        mStream << "\"" << escapeString(key) << "\":" << (value ? "true" : "false");
        return *this;
    }

    JSONBuilder& JSONBuilder::addRawValue(const std::string& key, const std::string& jsonValue)
    {
        addComma();
        mStream << "\"" << escapeString(key) << "\":" << jsonValue;
        return *this;
    }

    // JSONParser implementation

    JSONParser::JSONParser(const std::string& json)
        : mJson(json)
    {
        mValid = !json.empty() && (json[0] == '{' || json[0] == '[');
    }

    std::string JSONParser::trim(const std::string& str) const
    {
        auto start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos)
            return "";
        auto end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    std::string JSONParser::unquote(const std::string& str) const
    {
        std::string trimmed = trim(str);
        if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"')
        {
            return trimmed.substr(1, trimmed.size() - 2);
        }
        return trimmed;
    }

    std::string JSONParser::findKeyValue(const std::string& json, const std::string& key) const
    {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);

        if (keyPos == std::string::npos)
            return "";

        // Find the colon after the key
        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos)
            return "";

        // Skip whitespace after colon
        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() && std::isspace(json[valueStart]))
            valueStart++;

        if (valueStart >= json.size())
            return "";

        // Determine value end based on type
        char firstChar = json[valueStart];
        size_t valueEnd;

        if (firstChar == '"')
        {
            // String value - find closing quote
            valueEnd = valueStart + 1;
            while (valueEnd < json.size())
            {
                if (json[valueEnd] == '"' && json[valueEnd - 1] != '\\')
                    break;
                valueEnd++;
            }
            valueEnd++;
        }
        else if (firstChar == '{')
        {
            // Object - find matching brace
            int braceCount = 1;
            valueEnd = valueStart + 1;
            while (valueEnd < json.size() && braceCount > 0)
            {
                if (json[valueEnd] == '{')
                    braceCount++;
                else if (json[valueEnd] == '}')
                    braceCount--;
                valueEnd++;
            }
        }
        else if (firstChar == '[')
        {
            // Array - find matching bracket
            int bracketCount = 1;
            valueEnd = valueStart + 1;
            while (valueEnd < json.size() && bracketCount > 0)
            {
                if (json[valueEnd] == '[')
                    bracketCount++;
                else if (json[valueEnd] == ']')
                    bracketCount--;
                valueEnd++;
            }
        }
        else
        {
            // Number, boolean, or null - find delimiter
            valueEnd = json.find_first_of(",}", valueStart);
            if (valueEnd == std::string::npos)
                valueEnd = json.size();
        }

        return json.substr(valueStart, valueEnd - valueStart);
    }

    std::string JSONParser::extractValue(const std::string& key) const
    {
        return findKeyValue(mJson, key);
    }

    bool JSONParser::hasKey(const std::string& key) const
    {
        return !extractValue(key).empty();
    }

    std::string JSONParser::getString(const std::string& key, const std::string& defaultValue) const
    {
        std::string value = extractValue(key);
        if (value.empty())
            return defaultValue;
        return unquote(value);
    }

    int JSONParser::getInt(const std::string& key, int defaultValue) const
    {
        std::string value = trim(extractValue(key));
        if (value.empty())
            return defaultValue;
        try
        {
            return std::stoi(value);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    bool JSONParser::getBool(const std::string& key, bool defaultValue) const
    {
        std::string value = trim(extractValue(key));
        if (value.empty())
            return defaultValue;
        return value == "true";
    }

    JSONParser JSONParser::getObject(const std::string& key) const
    {
        std::string value = extractValue(key);
        return JSONParser(value);
    }

    JSONParser JSONParser::getArrayElement(int index) const
    {
        if (mJson.empty() || mJson[0] != '[')
            return JSONParser("");

        int currentIndex = 0;
        size_t pos = 1; // Skip opening bracket
        int depth = 0;
        size_t elementStart = pos;

        while (pos < mJson.size())
        {
            char c = mJson[pos];

            if (c == '[' || c == '{')
                depth++;
            else if (c == ']' || c == '}')
                depth--;
            else if (c == ',' && depth == 0)
            {
                if (currentIndex == index)
                {
                    return JSONParser(trim(mJson.substr(elementStart, pos - elementStart)));
                }
                currentIndex++;
                elementStart = pos + 1;
            }

            pos++;
        }

        // Check last element
        if (currentIndex == index && elementStart < mJson.size() - 1)
        {
            return JSONParser(trim(mJson.substr(elementStart, mJson.size() - 1 - elementStart)));
        }

        return JSONParser("");
    }

    int JSONParser::getArraySize() const
    {
        if (mJson.empty() || mJson[0] != '[')
            return 0;

        int count = 0;
        int depth = 0;
        size_t pos = 1;

        while (pos < mJson.size())
        {
            char c = mJson[pos];

            if (c == '[' || c == '{')
                depth++;
            else if (c == ']' || c == '}')
                depth--;
            else if (c == ',' && depth == 0)
                count++;

            pos++;
        }

        // If there's any content, there's at least one element
        if (mJson.size() > 2)
            count++;

        return count;
    }
}
