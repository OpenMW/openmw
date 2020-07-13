#ifndef VIEWOVERSHOULDER_H
#define VIEWOVERSHOULDER_H

#include "camera.hpp"

namespace MWRender
{

    class ViewOverShoulderController
    {
        public:
            ViewOverShoulderController(Camera* camera);

            void update();

        private:
            void trySwitchShoulder();
            enum class Mode { RightShoulder, LeftShoulder, Combat, Swimming };

            Camera* mCamera;
            Mode mMode;
            bool mAutoSwitchShoulder;
            float mOverShoulderHorizontalOffset;
            float mOverShoulderVerticalOffset;
            bool mDefaultShoulderIsRight;
    };

}

#endif // VIEWOVERSHOULDER_H
