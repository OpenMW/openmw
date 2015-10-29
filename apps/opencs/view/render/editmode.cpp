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

void CSVRender::EditMode::updateUserSetting (const QString& name, const QStringList& value)
{

}

void CSVRender::EditMode::setEditLock (bool locked)
{

}

void CSVRender::EditMode::primaryEditPressed (osg::ref_ptr<TagBase> tag) {}

void CSVRender::EditMode::secondaryEditPressed (osg::ref_ptr<TagBase> tag) {}

void CSVRender::EditMode::primarySelectPressed (osg::ref_ptr<TagBase> tag) {}

void CSVRender::EditMode::secondarySelectPressed (osg::ref_ptr<TagBase> tag) {}

bool CSVRender::EditMode::primaryEditStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

bool CSVRender::EditMode::secondaryEditStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

bool CSVRender::EditMode::primarySelectStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

bool CSVRender::EditMode::secondarySelectStartDrag (osg::ref_ptr<TagBase> tag)
{
    return false;
}

void CSVRender::EditMode::drag (int diffX, int diffY, double speedFactor) {}

void CSVRender::EditMode::dragCompleted() {}

void CSVRender::EditMode::dragAborted() {}

void CSVRender::EditMode::dragWheel (int diff, double speedFactor) {}
