#ifndef CSV_RENDER_TERRAINTEXTUREMODE_H
#define CSV_RENDER_TERRAINTEXTUREMODE_H

#include "editmode.hpp"

namespace CSVRender
{
    class PagedWorldspaceWidget;

    class TerrainTextureMode : public EditMode
    {
            Q_OBJECT

        public:

            TerrainTextureMode(WorldspaceWidget*, QWidget* parent = nullptr);

            void primarySelectPressed(const WorldspaceHitResult&) override;
            void secondarySelectPressed(const WorldspaceHitResult&) override;

            void activate(CSVWidget::SceneToolbar*) override;
            void deactivate(CSVWidget::SceneToolbar*) override;

        private:

            PagedWorldspaceWidget& getPagedWorldspaceWidget();
    };
}

#endif
