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

		virtual ~InputControlSystemLog() = default;
	};

    class DllExport InputControlSystem
    {

	public:

		enum NamedAxis { X = -1, Y = -2, Z = -3, UNASSIGNED = -4 };
		enum POVAxis { NorthSouth = 0, EastWest = 1 };

		typedef NamedAxis MouseAxis; // MouseAxis is deprecated. It will be removed in future versions

		typedef std::map<int, SDL_GameController*> JoystickInstanceMap;
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

		void controllerAdded  (int deviceID, const SDL_ControllerDeviceEvent &args);
		void controllerRemoved(const SDL_ControllerDeviceEvent &args);
		JoystickIDList& getJoystickIdList(){ return mJoystickIDList; };
		JoystickInstanceMap& getJoystickInstanceMap(){ return mJoystickInstanceMap; };

		// MouseListener
        void mouseMoved(const SDL_MouseMotionEvent &evt);
        void mousePressed(const SDL_MouseButtonEvent &evt, Uint8);
        void mouseReleased(const SDL_MouseButtonEvent &evt, Uint8);

		// KeyListener
        void keyPressed(const SDL_KeyboardEvent &evt);
        void keyReleased(const SDL_KeyboardEvent &evt);

		// ControllerListener
        void buttonPressed(int deviceID, const SDL_ControllerButtonEvent &evt);
        void buttonReleased(int deviceID, const SDL_ControllerButtonEvent &evt);
        void axisMoved(int deviceID, const SDL_ControllerAxisEvent &evt);

        void addKeyBinding(Control* control, SDL_Scancode key, Control::ControlChangingDirection direction);
        bool isKeyBound(SDL_Scancode key) const;
		void addMouseAxisBinding(Control* control, NamedAxis axis, Control::ControlChangingDirection direction);
		void addMouseButtonBinding(Control* control, unsigned int button, Control::ControlChangingDirection direction);
        bool isMouseButtonBound(unsigned int button) const;
        void addJoystickAxisBinding(Control* control, int deviceID, int axis, Control::ControlChangingDirection direction);
		void addJoystickButtonBinding(Control* control, int deviceID, unsigned int button, Control::ControlChangingDirection direction);
		bool isJoystickButtonBound(int deviceID, unsigned int button) const;
		bool isJoystickAxisBound(int deviceID, unsigned int axis) const;
        void removeKeyBinding(SDL_Scancode key);
		void removeMouseAxisBinding(NamedAxis axis);
		void removeMouseButtonBinding(unsigned int button);
		void removeJoystickAxisBinding(int deviceID, int axis);
		void removeJoystickButtonBinding(int deviceID, unsigned int button);

        SDL_Scancode getKeyBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		NamedAxis getMouseAxisBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		unsigned int getMouseButtonBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		int getJoystickAxisBinding(Control* control, int deviceID, ICS::Control::ControlChangingDirection direction);
		unsigned int getJoystickButtonBinding(Control* control, int deviceID, ICS::Control::ControlChangingDirection direction);

        std::string scancodeToString(SDL_Scancode key);

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

        typedef std::map<SDL_Scancode, ControlKeyBinderItem> ControlsKeyBinderMapType;	// <Scancode, [direction, control]>
		typedef std::map<int, ControlAxisBinderItem> ControlsAxisBinderMapType;			// <axis, [direction, control]>
		typedef std::map<int, ControlButtonBinderItem> ControlsButtonBinderMapType;		// <button, [direction, control]>

		typedef std::map<int, ControlsAxisBinderMapType> JoystickAxisBinderMapType;					// <joystick_id, <axis, [direction, control]> >
		typedef std::map<int, ControlsButtonBinderMapType> JoystickButtonBinderMapType;				// <joystick_id, <button, [direction, control]> >

		ControlsAxisBinderMapType mControlsMouseAxisBinderMap;			// <axis, [direction, control]>
		ControlsButtonBinderMapType mControlsMouseButtonBinderMap;		// <int, [direction, control]>
		JoystickAxisBinderMapType mControlsJoystickAxisBinderMap;		// <axis, [direction, control]>
		JoystickButtonBinderMapType mControlsJoystickButtonBinderMap;	// <button, [direction, control]>

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
		JoystickInstanceMap mJoystickInstanceMap;

		int mMouseAxisBindingInitialValues[3];

	private:

		Uint16 mClientWidth;
		Uint16 mClientHeight;

		/* ----------------------------------------------------------------------------------------
		 * OPENMW CODE STARTS HERE
		 * Mouse Wheel support added by Michael Stopa (Stomy) */

	public:
		enum class MouseWheelClick : int { UNASSIGNED = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4};

		void mouseWheelMoved(const SDL_MouseWheelEvent &evt);
		void addMouseWheelBinding(Control* control, MouseWheelClick click, Control::ControlChangingDirection direction);
		void removeMouseWheelBinding(MouseWheelClick click);
		MouseWheelClick getMouseWheelBinding(Control* control, ICS::Control::ControlChangingDirection direction);
		bool isMouseWheelBound(MouseWheelClick button) const;

	protected:
		void loadMouseWheelBinders(TiXmlElement* xmlControlNode);
		ControlsButtonBinderMapType mControlsMouseWheelBinderMap;

		/* OPENMW CODE ENDS HERE
		 * ------------------------------------------------------------------------------------- */
	};

	class DllExport DetectingBindingListener
	{
	public:
		virtual void keyBindingDetected(InputControlSystem* ICS, Control* control
            , SDL_Scancode key, Control::ControlChangingDirection direction);

		virtual void mouseAxisBindingDetected(InputControlSystem* ICS, Control* control
			, InputControlSystem::NamedAxis axis, Control::ControlChangingDirection direction);

		virtual void mouseButtonBindingDetected(InputControlSystem* ICS, Control* control
			, unsigned int button, Control::ControlChangingDirection direction);

		virtual void joystickAxisBindingDetected(InputControlSystem* ICS, int deviceID, Control* control
			, int axis, Control::ControlChangingDirection direction);

		virtual void joystickButtonBindingDetected(InputControlSystem* ICS, int deviceID, Control* control
			, unsigned int button, Control::ControlChangingDirection direction);

		/* ----------------------------------------------------------------------------------------
		 * OPENMW CODE STARTS HERE
		 * Mouse Wheel support added by Michael Stopa (Stomy) */

		virtual void mouseWheelBindingDetected(InputControlSystem* ICS, Control* control,
		                                       InputControlSystem::MouseWheelClick click,
		                                       Control::ControlChangingDirection direction);

        virtual ~DetectingBindingListener() = default;

		/* OPENMW CODE ENDS HERE
		 * ------------------------------------------------------------------------------------- */
	};

	extern const float ICS_MAX;
}


#endif
