#include <components/resource/animation.hpp>

#include <osg/ref_ptr>
#include <osgAnimation/Channel>

namespace Resource
{
    Animation::Animation(const Animation& anim, const osg::CopyOp& copyop): osg::Object(anim, copyop),
        mDuration(0.0f),
        mStartTime(0.0f)
    {
        const osgAnimation::ChannelList& channels = anim.getChannels();
        for (const auto& channel: channels)
            addChannel(channel.get()->clone());
    }

    void Animation::addChannel(osg::ref_ptr<osgAnimation::Channel> pChannel)
    {
        mChannels.push_back(pChannel);
    }

    std::vector<osg::ref_ptr<osgAnimation::Channel>>& Animation::getChannels()
    {
        return mChannels;
    }

    const std::vector<osg::ref_ptr<osgAnimation::Channel>>& Animation::getChannels() const
    {
        return mChannels;
    }

    bool Animation::update (double time)
    {
        for (const auto& channel: mChannels)
        {
            channel->update(time, 1.0f, 0);
        }
        return true;
    }
}
