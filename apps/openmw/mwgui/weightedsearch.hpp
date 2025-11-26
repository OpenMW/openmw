#ifndef MWGUI_WEIGHTEDSEARCH_H
#define MWGUI_WEIGHTEDSEARCH_H

#include <cstddef>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <MyGUI_UString.h>

#include <components/misc/strings/lower.hpp>

namespace MWGui
{
    std::vector<std::string> generatePatternArray(const MyGUI::UString& inputString)
    {
        if (inputString.empty() || inputString.find_first_not_of(" ") == std::string::npos)
            return std::vector<std::string>();
        std::string inputStringLowerCase = Misc::StringUtils::lowerCase(inputString);
        std::istringstream stringStream(inputStringLowerCase);
        return { std::istream_iterator<std::string>(stringStream), std::istream_iterator<std::string>() };
    }
    std::size_t weightedSearch(const std::string& corpus, const std::vector<std::string>& patternArray)
    {
        if (patternArray.empty())
            return 1;

        std::string corpusLowerCase = Misc::StringUtils::lowerCase(corpus);

        std::size_t numberOfMatches = 0;
        for (const std::string& word : patternArray)
            numberOfMatches += corpusLowerCase.find(word) != std::string::npos ? 1 : 0;

        return numberOfMatches;
    }
}

#endif
