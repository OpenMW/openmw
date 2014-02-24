#ifndef GAME_MWWORLD_CUSTOMDATA_H
#define GAME_MWWORLD_CUSTOMDATA_H

namespace MWWorld
{
    /// \brief Base class for the MW-class-specific part of RefData
    class CustomData
    {
        public:

            virtual ~CustomData() {}

            virtual CustomData *clone() const = 0;
    };
}

#endif
