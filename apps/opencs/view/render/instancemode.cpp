
#include "instancemode.hpp"

#include "../../model/settings/usersettings.hpp"

#include "elements.hpp"
#include "object.hpp"
#include "worldspacewidget.hpp"

CSVRender::InstanceMode::InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon (":placeholder"), Element_Reference, "Instance editing",
  parent), mContextSelect (false)
{

}

void CSVRender::InstanceMode::activate (CSVWidget::SceneToolbar *toolbar)
{
    EditMode::activate (toolbar);

    mContextSelect = CSMSettings::UserSettings::instance().setting ("scene-input/context-select")=="true";
}

void CSVRender::InstanceMode::updateUserSetting (const QString& name, const QStringList& value)
{
    if (name=="scene-input/context-select")
        mContextSelect = value.at (0)=="true";
}

void CSVRender::InstanceMode::primaryEditPressed (osg::ref_ptr<TagBase> tag)
{
    if (mContextSelect)
        primarySelectPressed (tag);
}

void CSVRender::InstanceMode::secondaryEditPressed (osg::ref_ptr<TagBase> tag)
{
    if (mContextSelect)
        secondarySelectPressed (tag);
}

void CSVRender::InstanceMode::primarySelectPressed (osg::ref_ptr<TagBase> tag)
{
    getWorldspaceWidget().clearSelection (Element_Reference);

    if (tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            // hit an Object, select it
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (true);
            return;
        }
    }
}

void CSVRender::InstanceMode::secondarySelectPressed (osg::ref_ptr<TagBase> tag)
{
    if (tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            // hit an Object, toggle its selection state
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (!object->getSelected());
            return;
        }
    }
}
