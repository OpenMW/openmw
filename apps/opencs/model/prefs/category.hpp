#ifndef CSM_PREFS_CATEGORY_H
#define CSM_PREFS_CATEGORY_H

#include <string>
#include <vector>

namespace CSMPrefs
{
    class State;
    class Setting;

    class Category
    {
        public:

            typedef std::vector<Setting *> Container;
            typedef Container::iterator Iterator;

        private:

            State *mParent;
            std::string mKey;
            Container mSettings;

        public:

            Category (State *parent, const std::string& key);

            const std::string& getKey() const;

            State *getState() const;

            void addSetting (Setting *setting);

            Iterator begin();

            Iterator end();

            Setting& operator[] (const std::string& key);

            void update();
    };
}

#endif
