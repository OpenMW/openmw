#ifndef CSM_PREFS_CATEGORY_H
#define CSM_PREFS_CATEGORY_H

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace CSMPrefs
{
    class State;
    class Setting;
    class Subcategory;

    class Category
    {
    public:
        typedef std::vector<Setting*> Container;
        typedef Container::iterator Iterator;

    private:
        State* mParent;
        std::string mKey;
        Container mSettings;
        std::unordered_map<std::string, Setting*> mIndex;

    public:
        Category(State* parent, const std::string& key);

        const std::string& getKey() const;

        State* getState() const;

        void addSetting(Setting* setting);

        void addSubcategory(Subcategory* setting);

        Iterator begin();

        Iterator end();

        Setting& operator[](const std::string& key);

        void update();
    };
}

#endif
