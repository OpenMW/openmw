#include "omwscriptsparser.hpp"

#include <algorithm>

#include <components/debug/debuglog.hpp>

std::vector<std::string> LuaUtil::parseOMWScriptsFiles(const VFS::Manager* vfs, const std::vector<std::string>& scriptLists)
{
    auto endsWith = [](std::string_view s, std::string_view suffix)
    {
        return s.size() >= suffix.size() && std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
    };
    std::vector<std::string> res;
    for (const std::string& scriptListFile : scriptLists)
    {
        if (!endsWith(scriptListFile, ".omwscripts"))
        {
            Log(Debug::Error) << "Script list should have suffix '.omwscripts', got: '" << scriptListFile << "'";
            continue;
        }
        std::string content(std::istreambuf_iterator<char>(*vfs->get(scriptListFile)), {});
        std::string_view view(content);
        while (!view.empty())
        {
            size_t pos = 0;
            while (pos < view.size() && view[pos] != '\n')
                pos++;
            std::string_view line = view.substr(0, pos);
            view = view.substr(std::min(pos + 1, view.size()));
            if (!line.empty() && line.back() == '\r')
                line = line.substr(0, pos - 1);
            // Lines starting with '#' are comments.
            // TODO: Maybe make the parser more robust. It is a bit inconsistent that 'path/#to/file.lua'
            //       is a valid path, but '#path/to/file.lua' is considered as a comment and ignored.
            if (line.empty() || line[0] == '#')
                continue;
            if (endsWith(line, ".lua"))
                res.push_back(std::string(line));
            else
                Log(Debug::Error) << "Lua script should have suffix '.lua', got: '" << line.substr(0, 300) << "'";
        }
    }
    return res;
}
