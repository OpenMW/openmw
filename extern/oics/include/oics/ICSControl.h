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

#ifndef OICS_Control_H_
#define OICS_Control_H_

#include "ICSPrerequisites.h"

#include "ICSChannel.h"
#include "ICSControlListener.h"

namespace ICS
{

    class DllExport Control
	{
	public:

		enum ControlChangingDirection
		{
			DECREASE = -1, STOP = 0, INCREASE = 1
		};

        Control(const std::string& name, bool autoChangeDirectionOnLimitsAfterStop = false, bool autoReverseToInitialValue = false, float initialValue = 0.5, float stepSize = 0.1, float stepsPerSeconds = 2.0, bool axisBindable = true);
		~Control();

		void setChangingDirection(ControlChangingDirection direction);
		inline ControlChangingDirection getChangingDirection(){ return currentChangingDirection; };

		void setValue(float value);
		inline float getValue(){ return mValue; };
		inline float getInitialValue(){ return mInitialValue; };
		inline void setInitialValue(float i) {mInitialValue = i;};

		void attachChannel(Channel* channel, Channel::ChannelDirection direction, float percentage = 1.0);
		std::list<Channel*> getAttachedChannels(){ return mAttachedChannels; };

		inline float getStepSize(){ return mStepSize; };
		inline float getStepsPerSeconds(){ return mStepsPerSeconds; };

		inline void setIgnoreAutoReverse(bool value){ mIgnoreAutoReverse = value; }; // mouse disable autoreverse
		inline bool isAutoReverseIgnored(){ return mIgnoreAutoReverse; };
		inline bool getAutoReverse(){ return mAutoReverseToInitialValue; };

		inline bool getAutoChangeDirectionOnLimitsAfterStop(){ return mAutoChangeDirectionOnLimitsAfterStop; };

		inline std::string getName(){ return mName; };

		inline bool isAxisBindable(){ return mAxisBindable; };
		inline void setAxisBindable(bool value){ mAxisBindable = value; };

		inline void addListener(ControlListener* ob){ mListeners.push_back(ob); };
	    inline void removeListener(ControlListener* ob){ mListeners.remove(ob); };

		void update(float timeSinceLastFrame);

	protected:
		float mValue;
		float mInitialValue;
		std::string mName;
		float mStepSize;
		float mStepsPerSeconds;
		bool mAutoReverseToInitialValue;
		bool mIgnoreAutoReverse;
		bool mAutoChangeDirectionOnLimitsAfterStop;
		bool mAxisBindable;

		Control::ControlChangingDirection currentChangingDirection;
		std::list<Channel*> mAttachedChannels;

		std::list<ControlListener*> mListeners;

		std::list<Control::ControlChangingDirection> mPendingActions;

	protected:

		void updateChannels();
	    void notifyListeners(float previousValue);

	};

}


#endif
