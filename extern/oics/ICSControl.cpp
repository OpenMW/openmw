/* -------------------------------------------------------
Copyright (c) 2011 Alberto G. Salguero (alberto.salguero (at) uca.es)

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------- */

#include "ICSInputControlSystem.h"

#include "ICSControl.h"

namespace ICS
{
    Control::Control(const std::string& name, bool autoChangeDirectionOnLimitsAfterStop, bool autoReverseToInitialValue
		, float initialValue, float stepSize, float stepsPerSeconds, bool axisBindable)
	: mValue(initialValue)
	 , mInitialValue(initialValue)
	 , mName(name)
	 , mStepSize(stepSize)
	 , mStepsPerSeconds(stepsPerSeconds)
	 , mAutoReverseToInitialValue(autoReverseToInitialValue)
	 , mIgnoreAutoReverse(false)
	 , mAutoChangeDirectionOnLimitsAfterStop(autoChangeDirectionOnLimitsAfterStop)
	 , mAxisBindable(axisBindable)
	 , currentChangingDirection(STOP)
	{

	}

	Control::~Control()
	{
		mAttachedChannels.clear();
	}

	void Control::setValue(float value)
	{
		float previousValue = mValue;

		mValue = std::max<float>(0.0,std::min<float>(1.0,value));

		if(mValue != previousValue)
		{
			updateChannels();

			notifyListeners(previousValue);
		}
	}

	void Control::attachChannel(Channel* channel, Channel::ChannelDirection direction, float percentage)
	{
		mAttachedChannels.push_back(channel);
		channel->addControl(this, direction, percentage);
	}

	void Control::updateChannels()
    {
        std::list<Channel*>::iterator pos = mAttachedChannels.begin();
        while (pos != mAttachedChannels.end())
        {
            ((Channel* )(*pos))->update();
            ++pos;
        }
    }

	void Control::notifyListeners(float previousValue)
    {
        std::list<ControlListener*>::iterator pos = mListeners.begin();
        while (pos != mListeners.end())
        {
            (*pos)->controlChanged((Control*)this, this->getValue(), previousValue);
            ++pos;
        }
    }

	void Control::setChangingDirection(ControlChangingDirection direction)
	{
		currentChangingDirection = direction;
		mPendingActions.push_back(direction);
	}

	void Control::update(float timeSinceLastFrame)
	{
        if(!mPendingActions.empty())
		{
			size_t timedActionsCount = 0;

			std::list<Control::ControlChangingDirection>::iterator cached_end = mPendingActions.end();
			for(std::list<Control::ControlChangingDirection>::iterator it = mPendingActions.begin() ;
                it != cached_end ; ++it)
			{
				if( (*it) != Control::STOP )
				{
					timedActionsCount++;
				}
			}

			float timeSinceLastFramePart = timeSinceLastFrame / std::max<size_t>(1, timedActionsCount);
			for(std::list<Control::ControlChangingDirection>::iterator it = mPendingActions.begin() ;
                it != cached_end ; ++it)
			{
				if( (*it) != Control::STOP )
				{
					this->setValue(mValue +
						(((int)(*it)) * mStepSize * mStepsPerSeconds * (timeSinceLastFramePart)));
				}
				else if(mAutoReverseToInitialValue && !mIgnoreAutoReverse && mValue != mInitialValue )
				{

					if(mValue > mInitialValue)
					{
						this->setValue( std::max<float>( mInitialValue,
							mValue - (mStepSize * mStepsPerSeconds * (timeSinceLastFramePart))));
					}
					else if(mValue < mInitialValue)
					{
						this->setValue( std::min<float>( mInitialValue,
							mValue + (mStepSize * mStepsPerSeconds * (timeSinceLastFramePart))));
					}
				}
			}
			mPendingActions.clear();
		}
		else if( currentChangingDirection != Control::STOP )
		{
			this->setValue(mValue +
				(((int)currentChangingDirection) * mStepSize * mStepsPerSeconds * (timeSinceLastFrame)));
		}
		else if(mAutoReverseToInitialValue && !mIgnoreAutoReverse && mValue != mInitialValue )
		{
			if(mValue > mInitialValue)
			{
				this->setValue( std::max<float>( mInitialValue,
					mValue - (mStepSize * mStepsPerSeconds * (timeSinceLastFrame))));
			}
			else if(mValue < mInitialValue)
			{
				this->setValue( std::min<float>( mInitialValue,
					mValue + (mStepSize * mStepsPerSeconds * (timeSinceLastFrame))));
			}
		}
	}
}
