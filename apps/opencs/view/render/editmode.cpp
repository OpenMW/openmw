#include "editmode.hpp"

#include "tagbase.hpp"
#include "worldspacewidget.hpp"

CSVRender::WorldspaceWidget& CSVRender::EditMode::getWorldspaceWidget()
{
    return *mWorldspaceWidget;
}

CSVRender::EditMode::EditMode (WorldspaceWidget *worldspaceWidget, const QIcon& icon,
    unsigned int mask, const QString& tooltip, QWidget *parent)
: ModeButton (icon, tooltip, parent), mWorldspaceWidget (worldspaceWidget), mMask (mask)
{}

unsigned int CSVRender::EditMode::getInteractionMask() const
{
    return mMask;
}

void CSVRender::EditMode::activate (CSVWidget::SceneToolbar *toolbar)
{
    mWorldspaceWidget->setInteractionMask (mMask);
    mWorldspaceWidget->clearSelection (~mMask);
}

void CSVRender::EditMode::setEditLock (bool locked)
{

}

void CSVRender::EditMode::primaryEditPressed (const WorldspaceHitResult& hit) {}

void CSVRender::EditMode::secondaryEditPressed (const WorldspaceHitResult& hit) {}

void CSVRender::EditMode::primarySelectPressed (const WorldspaceHitResult& hit) {}

void CSVRender::EditMode::secondarySelectPressed (const WorldspaceHitResult& hit) {}

bool CSVRender::EditMode::primaryEditStartDrag (const WorldspaceHitResult& hit)
{
    return false;
}

bool CSVRender::EditMode::secondaryEditStartDrag (const WorldspaceHitResult& hit)
{
    return false;
}

bool CSVRender::EditMode::primarySelectStartDrag (const WorldspaceHitResult& hit)
{
    return false;
}

bool CSVRender::EditMode::secondarySelectStartDrag (const WorldspaceHitResult& hit)
{
    return false;
}

void CSVRender::EditMode::drag (int diffX, int diffY, double speedFactor) {}

void CSVRender::EditMode::dragCompleted(const WorldspaceHitResult& hit) {}

void CSVRender::EditMode::dragAborted() {}

void CSVRender::EditMode::dragWheel (int diff, double speedFactor) {}

void CSVRender::EditMode::dragEnterEvent (QDragEnterEvent *event) {}

void CSVRender::EditMode::dropEvent (QDropEvent* event) {}

void CSVRender::EditMode::dragMoveEvent (QDragMoveEvent *event) {}

int CSVRender::EditMode::getSubMode() const
{
    return -1;
}
