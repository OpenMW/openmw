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

#ifndef OICS_InputControlSystem_H_
#define OICS_InputControlSystem_H_

#include "ICSPrerequisites.h"

#include "ICSControl.h"
#include "ICSChannel.h"

#include "../sdl4ogre/events.h"

#define ICS_LOG(text) if(mLog) mLog->logMessage( ("ICS: " + std::string(text)).c_str() );
#define ICS_MAX_JOYSTICK_AXIS 16
#define ICS_MOUSE_BINDING_MARGIN 30
#define ICS_JOYSTICK_AXIS_BINDING_MARGIN 10000
#define ICS_JOYSTICK_SLIDER_BINDING_MARGIN 10000
#define ICS_MOUSE_AXIS_BINDING_NULL_VALUE std::numeric_limits<int>::max()

namespace ICS
{
	class DllExport InputControlSystemLog 
	{
	public:
		virtual void logMessage(const char* text) = 0;
	};

	class DllExport InputControlSystem : 
		public SFO::MouseListener,
		public SFO::KeyListener,
        public SFO::JoyListener
	{

	public:

		enum NamedAxis { X = -1, Y = -2, Z = -3, UNASSIGNED = -4 };
		enum POVAxis { NorthSouth = 0, EastWest = 1 };

		typedef NamedAxis MouseAxis; // MouseAxis is deprecated. It will be removed in future versions

		typedef std::list<int> JoystickIDList;

		typedef struct
		{
			int index;
			POVAxis axis;
		} POVBindingPair;

		InputControlSystem(std::string file = "", bool active = true
			, DetectingBindingListener* detectingBindingListener = NULL
			, InputControlSystemLog* log = NULL, size_t channelCount = 16); 
		~InputControlSystem();

		std::string getFileName(){ return mFileName; };
		std::string getBaseFileName();

		void setDetectingBindingListener(DetectingBindingListener* detectingBindingListener){ mDetectingBindingListener = detectingBindingListener; };
		DetectingBindingListener* getDetectingBindingListener(){ return mDetectingBindingListener; };

		// in seconds
		void update(float timeSinceLastFrame);

        inline Channel* getChannel(int i){ return mChannels[i]; };
		float getChannelValue(int i);
		inline int getChannelCount(){ return (int)mChannels.size(); };

		inline Control* getControl(int i){ return mControls[i]; };
		float getControlValue(int i);
		inline int getControlCount(){ return (int)mControls.size(); };
		inline void addControl(Control* control){ mControls.push_back(control); };

		Control* findControl(std::string name);

		inline void activate(){ this->mActive = true; };
		inline void deactivate(){ this->mActive = false; };

		void addJoystick(int deviceId);
		JoystickIDList& getJoystickIdList(){ return mJoystickIDList; };
		
		// MouseListener
        void mouseMoved(const SFO::MouseMotionEvent &evt);
        void mousePressed(const SDL_MouseButtonEvent &evt, Uint8);
        void mouseReleased(const SDL_MouseButtonEvent &evt, Uint8);
		
		// KeyListener
        void keyPressed(const SDL_KeyboardEvent &evt);
        void keyReleased(const SDL_KeyboardEvent &evt);
		
		// JoyStickListener
        void buttonPressed(const SDL_JoyButtonEvent &evt, int button);
        void buttonReleased(const SDL_JoyButtonEvent &evt, int button);
        void axisMoved(const SDL_JoyAxisEvent &evt, int axis);
        void povMoved(const SDL_JoyHatEvent &evt, int index);
		//TODO: does this have an SDL equivalent?
        //bool sliderMoved(const OIS::JoyStickEvent &evt, int index);

		void addKeyBinding(Control* control, SDL_Keycode key, Control::ControlChangingDirection direction);
		void addMouseAxisBinding(Control* control, NamedAxis axis, Control::ControlChangingDirection direction);
		void addMouseButtonBinding(Control* control, unsigned int button, Control::ControlChangingDirection direction);
		void addJoystickAxisBinding(Control* control, int deviceId, int axis, Control::ControlChangingDirection direction);
		void addJoystickButtonBinding(Control* control, int deviceId, unsigned int button, Control::ControlChangingDirection direction);
		void addJoystickPOVBinding(Control* control, int deviceId, int index, POVAxis axis, Control::ControlChangingDirection direction);
		void addJoystickSliderBinding(Control* control, int deviceId, int index, Control::ControlChangingDirection direction);
		void removeKeyBinding(SDL_Keycode key);
		void removeMouseAxisBinding(NamedAxis axis);
		void removeMouseButtonBinding(unsigned int button);
		void removeJoystickAxisBinding(int deviceId, int axis);
		void removeJoystickButtonBinding(int deviceId, unsigned int button);
		void removeJoystickPOVBinding(int deviceId, int index, POVAxis axis);
		void removeJoystickSliderBinding(int deviceId, int index);

		SDL_Keycode getKeyBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		NamedAxis getMouseAxisBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		unsigned int getMouseButtonBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		int getJoystickAxisBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction);
		unsigned int getJoystickButtonBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction);
		POVBindingPair getJoystickPOVBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction);
		int getJoystickSliderBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction);

		std::string keyCodeToString(SDL_Keycode key);
		SDL_Keycode stringToKeyCode(std::string key);

		void enableDetectingBindingState(Control* control, Control::ControlChangingDirection direction);
		void cancelDetectingBindingState();
        bool detectingBindingState();

		bool save(std::string fileName = "");

		void adjustMouseRegion (Uint16 width, Uint16 height);

	protected:

		void loadKeyBinders(TiXmlElement* xmlControlNode);
		void loadMouseAxisBinders(TiXmlElement* xmlControlNode);
		void loadMouseButtonBinders(TiXmlElement* xmlControlNode);
		void loadJoystickAxisBinders(TiXmlElement* xmlControlNode);
		void loadJoystickButtonBinders(TiXmlElement* xmlControlNode);
		void loadJoystickPOVBinders(TiXmlElement* xmlControlNode);
		void loadJoystickSliderBinders(TiXmlElement* xmlControlNode);

		void addMouseAxisBinding_(Control* control, int axis, Control::ControlChangingDirection direction);
		void removeMouseAxisBinding_(int axis);

	protected:

		typedef struct {
			Control::ControlChangingDirection direction;
			Control* control;
		} ControlKeyBinderItem;

		typedef ControlKeyBinderItem ControlAxisBinderItem;
		typedef ControlKeyBinderItem ControlButtonBinderItem;
		typedef ControlKeyBinderItem ControlPOVBinderItem;
		typedef ControlKeyBinderItem ControlSliderBinderItem;

		typedef struct {
			Control* control;
			Control::ControlChangingDirection direction;
		} PendingActionItem;

		std::list<PendingActionItem> mPendingActions;

		std::string mFileName;

		typedef std::map<SDL_Keycode, ControlKeyBinderItem> ControlsKeyBinderMapType;	// <KeyCode, [direction, control]>
		typedef std::map<int, ControlAxisBinderItem> ControlsAxisBinderMapType;			// <axis, [direction, control]>
		typedef std::map<int, ControlButtonBinderItem> ControlsButtonBinderMapType;		// <button, [direction, control]>
		typedef std::map<int, ControlPOVBinderItem> ControlsPOVBinderMapType;			// <index, [direction, control]>
		typedef std::map<int, ControlSliderBinderItem> ControlsSliderBinderMapType;		// <index, [direction, control]>

		typedef std::map<int, ControlsAxisBinderMapType> JoystickAxisBinderMapType;					// <joystick_id, <axis, [direction, control]> >
		typedef std::map<int, ControlsButtonBinderMapType> JoystickButtonBinderMapType;				// <joystick_id, <button, [direction, control]> > 
        typedef std::map<int, std::map<int, ControlsPOVBinderMapType> > JoystickPOVBinderMapType;	// <joystick_id, <index, <axis, [direction, control]> > >
		typedef std::map<int, ControlsSliderBinderMapType> JoystickSliderBinderMapType;				// <joystick_id, <index, [direction, control]> > 

		ControlsAxisBinderMapType mControlsMouseAxisBinderMap;			// <axis, [direction, control]>
		ControlsButtonBinderMapType mControlsMouseButtonBinderMap;		// <int, [direction, control]>
		JoystickAxisBinderMapType mControlsJoystickAxisBinderMap;		// <joystick_id, <axis, [direction, control]> >
		JoystickButtonBinderMapType mControlsJoystickButtonBinderMap;	// <joystick_id, <button, [direction, control]> > 
		JoystickPOVBinderMapType mControlsJoystickPOVBinderMap;			// <joystick_id, <index, <axis, [direction, control]> > > 
		JoystickSliderBinderMapType mControlsJoystickSliderBinderMap;	// <joystick_id, <index, [direction, control]> > 

		std::vector<Control *> mControls;
		std::vector<Channel *> mChannels;

		ControlsKeyBinderMapType mControlsKeyBinderMap;

		bool mActive;
		InputControlSystemLog* mLog;
		
		DetectingBindingListener* mDetectingBindingListener;
		Control* mDetectingBindingControl;
		Control::ControlChangingDirection mDetectingBindingDirection;

		bool mXmouseAxisBinded;
		bool mYmouseAxisBinded;

		JoystickIDList mJoystickIDList;

		int mMouseAxisBindingInitialValues[3];

	private:

		Uint16 mClientWidth;
		Uint16 mClientHeight;
	};

	class DllExport DetectingBindingListener
	{
	public:
		virtual void keyBindingDetected(InputControlSystem* ICS, Control* control
			, SDL_Keycode key, Control::ControlChangingDirection direction);

		virtual void mouseAxisBindingDetected(InputControlSystem* ICS, Control* control
			, InputControlSystem::NamedAxis axis, Control::ControlChangingDirection direction);

		virtual void mouseButtonBindingDetected(InputControlSystem* ICS, Control* control
			, unsigned int button, Control::ControlChangingDirection direction);

		virtual void joystickAxisBindingDetected(InputControlSystem* ICS, Control* control
			, int deviceId, int axis, Control::ControlChangingDirection direction);

		virtual void joystickButtonBindingDetected(InputControlSystem* ICS, Control* control
			, int deviceId, unsigned int button, Control::ControlChangingDirection direction);

		virtual void joystickPOVBindingDetected(InputControlSystem* ICS, Control* control
			, int deviceId, int pov, InputControlSystem::POVAxis axis, Control::ControlChangingDirection direction);

		virtual void joystickSliderBindingDetected(InputControlSystem* ICS, Control* control
			, int deviceId, int slider, Control::ControlChangingDirection direction);
	};

	static const float ICS_MAX = std::numeric_limits<float>::max();
}


#endif
