#ifndef MWGUI_WEIGHTEDSEARCH_H
#define MWGUI_WEIGHTEDSEARCH_H

#include <string>
#include <vector>

#include <MyGUI_UString.h>

namespace MWGui
{
    std::vector<std::string> generatePatternArray(const MyGUI::UString& inputString);
    std::size_t weightedSearch(const std::string& corpus, const std::vector<std::string>& patternArray);
}

#endif
