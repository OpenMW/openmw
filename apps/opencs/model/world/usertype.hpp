#ifndef CSM_WORLD_USERTYPE_H
#define CSM_WORLD_USERTYPE_H

#include <QMetaType>
#include <QVariant>

namespace CSMWorld
{
    // Idea from ksimons @stackoverflow
    class UserInt
    {
        public:

            UserInt() : mValue(0) { }
            UserInt(int value) : mValue(value) { }
            UserInt(const UserInt &other) { mValue = other.mValue; }
            ~UserInt() { }
            int value() const { return mValue; }

        private:

            int mValue;
    };

    class UserFloat
    {
        public:

            UserFloat() : mValue(0) { }
            UserFloat(float value) : mValue(value) { }
            UserFloat(const UserFloat &other) { mValue = other.mValue; }
            ~UserFloat() { }
            float value() const { return mValue; }

        private:

            float mValue;
    };
}

Q_DECLARE_METATYPE(CSMWorld::UserInt)
Q_DECLARE_METATYPE(CSMWorld::UserFloat)

#endif // CSM_WORLD_USERTYPE_H
