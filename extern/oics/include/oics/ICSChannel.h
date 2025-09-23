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

#ifndef OICS_Channel_H_
#define OICS_Channel_H_

#include "ICSPrerequisites.h"

#include "ICSChannelListener.h"

namespace ICS
{	
	struct FilterInterval{
		//std::string type; //! @todo uncomment when more types implemented
		float startX;
		float startY;
		float midX;
		float midY;
		float endX;
		float endY;
		float step;
	};

	typedef std::list<FilterInterval> IntervalList;

	class DllExport Channel
	{
	public:
		enum ChannelDirection
		{
			INVERSE = -1, DIRECT = 1
		};

		typedef struct {
			ChannelDirection direction;
			float percentage;
			Control* control;
		} ControlChannelBinderItem;


		Channel(int number, float initialValue = 0.5
			, float bezierMidPointY = 0.5, float bezierMidPointX = 0.5
			, float symmetricAt = 0, float bezierStep = 0.2); //! @todo implement symetry
		~Channel(){};

		void setValue(float value);
		float getValue();

        inline int getNumber(){ return mNumber; };

		void addControl(Control* control, Channel::ChannelDirection dir, float percentage);
		inline size_t getControlsCount(){ return mAttachedControls.size(); };
		std::vector<ControlChannelBinderItem> getAttachedControls(){ return mAttachedControls; };
		ControlChannelBinderItem getAttachedControlBinding(Control* control);

        void addListener(ChannelListener* ob){ mListeners.push_back(ob); };
	    void removeListener(ChannelListener* ob){ mListeners.remove(ob); };

		void update();

		void setBezierFunction(float bezierMidPointY, float bezierMidPointX = 0.5
			, float symmetricAt = 0, float bezierStep = 0.2);

		void addBezierInterval(float startX, float startY, float midX, float midY
			, float endX, float endY, float step = 0.1);

		IntervalList& getIntervals(){ return mIntervals; };

        void setEnabled(bool enabled);

    protected:

        int mNumber;
		float mValue;
		
		struct BezierPoint{
			float x;
			float y;
			bool operator < (const BezierPoint& other){ return x < other.x; }
		};

		typedef std::list<BezierPoint> BezierFunction;

		BezierPoint mBezierMidPoint;
		BezierFunction mBezierFunction;
		float mSymmetricAt;
		float mBezierStep;

		IntervalList mIntervals;

		std::vector<ControlChannelBinderItem> mAttachedControls;

        std::list<ChannelListener* > mListeners;
        void notifyListeners(float previousValue);

        bool mEnabled;

	};

}


#endif
