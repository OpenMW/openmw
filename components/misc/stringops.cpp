#include "stringops.hpp"

#include <cctype>
#include <algorithm>
#include <iterator>

#include <string.h>
#include <libs/platform/strings.h>



namespace Misc
{

std::locale StringUtils::mLocale = std::locale::classic();

}
