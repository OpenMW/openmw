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

#define	B1(t) (t*t)
#define	B2(t) (2*t*(1-t))
#define	B3(t) ((1-t)*(1-t))

namespace ICS
{
	Channel::Channel(int number, float initialValue
			, float bezierMidPointY, float bezierMidPointX, float symmetricAt, float bezierStep)
            : mNumber(number)
            , mValue(initialValue)
			, mSymmetricAt(symmetricAt)
			, mBezierStep(bezierStep)
            , mEnabled(true)
    { 
		mBezierMidPoint.x = bezierMidPointX;
		mBezierMidPoint.y = bezierMidPointY;

		setBezierFunction(bezierMidPointY, bezierMidPointX, symmetricAt, bezierStep);
	} 

    void Channel::setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

	float Channel::getValue()
	{
		if(mValue == 0 || mValue == 1)
		{
			return mValue;
		}

        assert (!mBezierFunction.empty());
        BezierFunction::iterator it = mBezierFunction.begin();
        //size_t size_minus_1 = mBezierFunction.size() - 1;
        BezierFunction::iterator last = mBezierFunction.end();
        --last;
        for ( ; it != last ; )
        {
            BezierPoint left = (*it);
            BezierPoint right = (*(++it));

            if( (left.x <= mValue) && (right.x > mValue) )
            {
                float val = left.y - (left.x - mValue) * (left.y - right.y) / (left.x - right.x);

                return std::max<float>(0.0,std::min<float>(1.0, val));
            }
        }

		return -1; 
	}

    void Channel::setValue(float value)
    {
        float previousValue = this->getValue();
    
        mValue = value;

        if(previousValue != value && mEnabled)
        {
            notifyListeners(previousValue);
        }
    }

    void Channel::notifyListeners(float previousValue)
    {
        std::list<ChannelListener*>::iterator pos = mListeners.begin();
        while (pos != mListeners.end())
        {
            (*pos)->channelChanged((Channel*)this, this->getValue(), previousValue);
            ++pos;
        }
    }

	void Channel::addControl(Control* control, Channel::ChannelDirection dir, float percentage)
	{
		ControlChannelBinderItem ccBinderItem;
		ccBinderItem.control = control;
		ccBinderItem.direction = dir;
		ccBinderItem.percentage = percentage;

		mAttachedControls.push_back(ccBinderItem);
	}

	Channel::ControlChannelBinderItem Channel::getAttachedControlBinding(Control* control)
	{
		for(std::vector<ControlChannelBinderItem>::iterator it = mAttachedControls.begin() ;
			it != mAttachedControls.end() ; it++)
		{
			if((*it).control == control)
			{
				return (*it);
			}
		}

		ControlChannelBinderItem nullBinderItem;
		nullBinderItem.control = NULL;
		nullBinderItem.direction = Channel/*::ChannelDirection*/::DIRECT;
		nullBinderItem.percentage = 0;
		return nullBinderItem;
	}

	void Channel::update()
	{
        if(this->getControlsCount() == 1)
		{
			ControlChannelBinderItem ccBinderItem = mAttachedControls.back();
			float diff = ccBinderItem.control->getValue() - ccBinderItem.control->getInitialValue();

			if(ccBinderItem.direction == ICS::Channel::DIRECT)
			{
				this->setValue(ccBinderItem.control->getInitialValue() + (ccBinderItem.percentage * diff));
			}
			else
			{
				this->setValue(ccBinderItem.control->getInitialValue() - (ccBinderItem.percentage * diff));
			}
		}
		else
		{
			float val = 0;
			std::vector<ControlChannelBinderItem>::const_iterator it;
			for(it=mAttachedControls.begin(); it!=mAttachedControls.end(); ++it)
			{
				ControlChannelBinderItem ccBinderItem = (*it);
				float diff = ccBinderItem.control->getValue() - ccBinderItem.control->getInitialValue();

				if(ccBinderItem.direction == ICS::Channel::DIRECT)
				{
					val += (ccBinderItem.percentage * diff);
				}
				else
				{
					val -= (ccBinderItem.percentage * diff);
				}
			}

			if(mAttachedControls.size() > 0)
			{
				this->setValue(mAttachedControls.begin()->control->getInitialValue() + val);
			}
		}
	}

	void Channel::setBezierFunction(float bezierMidPointY, float bezierMidPointX, float symmetricAt, float bezierStep)
	{
		mBezierMidPoint.x = bezierMidPointX;
		mBezierMidPoint.y = bezierMidPointY;
		mBezierStep = bezierStep;
		mSymmetricAt = symmetricAt;

		mBezierFunction.clear();

		BezierPoint start;
		start.x = 0;
		start.y = 0;

		BezierPoint end;
		end.x = 1;
		end.y = 1;
		mBezierFunction.push_front(end);

		FilterInterval interval;
		interval.startX = start.x;
		interval.startY = start.y;
		interval.midX = mBezierMidPoint.x;
		interval.midY = mBezierMidPoint.y;
		interval.endX = end.x;
		interval.endY = end.y;
		interval.step = bezierStep;
		mIntervals.push_back(interval);

		if(!(mBezierMidPoint.x == 0.5 && mBezierMidPoint.y == 0.5))
		{
			float t = mBezierStep;
			while(t < 1)
			{
				BezierPoint p;
				p.x = start.x * B1(t) + mBezierMidPoint.x * B2(t) + end.x * B3(t);
				p.y = start.y * B1(t) + mBezierMidPoint.y * B2(t) + end.y * B3(t);
				mBezierFunction.push_front(p);

				t += mBezierStep;
			}
		}

		mBezierFunction.push_front(start);
	}

	void Channel::addBezierInterval(float startX, float startY, float midX, float midY
			, float endX, float endY, float step)
	{
		FilterInterval interval;
		interval.startX = startX;
		interval.startY = startY;
		interval.midX = midX;
		interval.midY = midY;
		interval.endX = endX;
		interval.endY = endY;
		interval.step = step;
		mIntervals.push_back(interval);

		float t = 0;
		while(t <= 1)
		{
			BezierPoint p;
			p.x = startX * B1(t) + midX * B2(t) + endX * B3(t);
			p.y = startY * B1(t) + midY * B2(t) + endY * B3(t);

			BezierFunction::iterator it = mBezierFunction.begin();
			while( it != mBezierFunction.end() )
			{
				BezierPoint left = (*it);
				BezierPoint right; 
				++it;
				if( it != mBezierFunction.end() )
				{
					right = (*it);
				}
				else
				{
					right.x = endX;
					right.y = endY;
				}

				if(p.x > left.x && p.x < right.x)
				{
					mBezierFunction.insert(it, p);
					break;
				}
			}

			t += 1.0f / ((endX-startX)/step);
		}
	}
}
