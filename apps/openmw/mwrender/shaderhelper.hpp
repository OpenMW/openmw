#ifndef GAME_SHADERHELPER_H
#define GAME_SHADERHELPER_H

#include <string>

namespace MWRender
{
    class RenderingManager;

    ///
    /// \brief manages the main shader
    ///
    class ShaderHelper
    {
    public:
        ShaderHelper(RenderingManager* rend);

        void applyShaders();
        ///< apply new settings

    private:
        RenderingManager* mRendering;

        void createShader(const bool mrt, const bool shadows, const bool split, const std::string& name);
    };

}

#endif
