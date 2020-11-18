#ifndef OPENMW_COMPONENTS_RESOURCE_ANIMATION_HPP
#define OPENMW_COMPONENTS_RESOURCE_ANIMATION_HPP

#include <vector>

#include <osg/Node>
#include <osg/Object>
#include <osgAnimation/Channel>

namespace Resource
{
    /// Stripped down class of osgAnimation::Animation, only needed for OSG's plugin formats like dae
    class Animation : public osg::Object
    {
        public:
            META_Object(Resource, Animation)

            Animation() :
                mDuration(0.0), mStartTime(0) {}

            Animation(const Animation&, const osg::CopyOp&);
            ~Animation() {}

            void addChannel (osg::ref_ptr<osgAnimation::Channel> pChannel);

            std::vector<osg::ref_ptr<osgAnimation::Channel>>& getChannels();

            const std::vector<osg::ref_ptr<osgAnimation::Channel>>& getChannels() const;

            bool update (double time);

        protected:
            double mDuration;
            double mStartTime;
            std::vector<osg::ref_ptr<osgAnimation::Channel>> mChannels;
        };
}

#endif
