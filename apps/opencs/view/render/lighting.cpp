#include "lighting.hpp"

#include <osg/LightSource>
#include <osg/NodeVisitor>
#include <osg/Switch>

#include <components/misc/constants.hpp>

class DayNightSwitchVisitor : public osg::NodeVisitor
{
public:
    DayNightSwitchVisitor(int index)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mIndex(index)
    { }

    virtual void apply(osg::Switch &switchNode)
    {
        if (switchNode.getName() == Constants::NightDayLabel)
            switchNode.setSingleChildOn(mIndex);

        traverse(switchNode);
    }

private:
    int mIndex;
};

CSVRender::Lighting::~Lighting() {}

void CSVRender::Lighting::updateDayNightMode(int index)
{
    if (mRootNode == nullptr)
        return;

    DayNightSwitchVisitor visitor(index);
    mRootNode->accept(visitor);
}
