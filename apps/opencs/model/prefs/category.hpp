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
            std::string mName;

        public:

            Category (State *parent, const std::string& key, const std::string& name);

            const std::string& getKey() const;

            const std::string& getName() const;
    };
}

#endif
