#include "editmode.hpp"

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

void CSVRender::EditMode::updateUserSetting (const QString& name, const QStringList& value)
{

}

void CSVRender::EditMode::setEditLock (bool locked)
{

}

void CSVRender::EditMode::primaryEditPressed (osg::ref_ptr<TagBase> tag) {}

void CSVRender::EditMode::secondaryEditPressed (osg::ref_ptr<TagBase> tag) {}

void CSVRender::EditMode::selectPressed (osg::ref_ptr<TagBase> tag) {}

bool CSVRender::EditMode::primaryEditStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

bool CSVRender::EditMode::secondaryEditStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

bool CSVRender::EditMode::selectStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

void CSVRender::EditMode::drag (int diffX, int diffY) {}

void CSVRender::EditMode::dragCompleted() {}

void CSVRender::EditMode::dragAborted() {}

void CSVRender::EditMode::dragWheel (int diff) {}
