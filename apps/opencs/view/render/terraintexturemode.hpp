#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

#include "editmode.hpp"

namespace CSVWidget
{
    class SceneToolMode;
}

namespace CSVRender
{

    class TerrainTextureMode : public EditMode
    {
            Q_OBJECT

            CSVWidget::SceneToolMode *mSubMode;
            std::string mSubModeId;

        public:

            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

            void primarySelectPressed(const WorldspaceHitResult&);
            void secondarySelectPressed(const WorldspaceHitResult&);


            void activate(CSVWidget::SceneToolbar*);
            void deactivate(CSVWidget::SceneToolbar*);
            int getSubModeFromId (const std::string& id) const;
            void subModeChanged (const std::string& id);

    };
}


#endif
