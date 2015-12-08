#ifndef CSV_PREFS_CATEGORY_H
#define CSM_PREFS_CATEGORY_H

#include <iostream>

namespace CSMPrefs
{
    class State;

    class Category
    {
            State *mParent;
            std::string mKey;

        public:

            Category (State *parent, const std::string& key);

            const std::string& getKey() const;

    };
}

#endif
