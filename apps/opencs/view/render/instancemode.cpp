
#include "instancemode.hpp"

#include "../../model/prefs/state.hpp"

#include "elements.hpp"
#include "object.hpp"
#include "worldspacewidget.hpp"

CSVRender::InstanceMode::InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon (":placeholder"), Element_Reference, "Instance editing",
  parent)
{

}

void CSVRender::InstanceMode::primaryEditPressed (osg::ref_ptr<TagBase> tag)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        primarySelectPressed (tag);
}

void CSVRender::InstanceMode::secondaryEditPressed (osg::ref_ptr<TagBase> tag)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
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
