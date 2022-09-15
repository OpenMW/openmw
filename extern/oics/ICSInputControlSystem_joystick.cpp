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

#define SDL_JOY_AXIS_MIN -32768
#define SDL_JOY_AXIS_MAX 32767

namespace ICS
{
	// load xml
	void InputControlSystem::loadJoystickAxisBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlJoystickBinder = xmlControlNode->FirstChildElement("JoystickAxisBinder");
		while(xmlJoystickBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlJoystickBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlJoystickBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			addJoystickAxisBinding(mControls.back(), FromString<int>(xmlJoystickBinder->Attribute("deviceId")), FromString<int>(xmlJoystickBinder->Attribute("axis")), dir);

			xmlJoystickBinder = xmlJoystickBinder->NextSiblingElement("JoystickAxisBinder");
		}
	}

	void InputControlSystem::loadJoystickButtonBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlJoystickButtonBinder = xmlControlNode->FirstChildElement("JoystickButtonBinder");
		while(xmlJoystickButtonBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlJoystickButtonBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlJoystickButtonBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			addJoystickButtonBinding(mControls.back(), FromString<int>(xmlJoystickButtonBinder->Attribute("deviceId")), FromString<int>(xmlJoystickButtonBinder->Attribute("button")), dir);

			xmlJoystickButtonBinder = xmlJoystickButtonBinder->NextSiblingElement("JoystickButtonBinder");
		}
	}

	// add bindings
	void InputControlSystem::addJoystickAxisBinding(Control* control, int deviceID, int axis, Control::ControlChangingDirection direction)
	{
		if (std::find(mJoystickIDList.begin(), mJoystickIDList.end(), deviceID) == mJoystickIDList.end())
			mJoystickIDList.push_back(deviceID);

		ICS_LOG("\tAdding AxisBinder [axis="
			+ ToString<int>(axis)       + ", deviceID="
			+ ToString<int>(deviceID)   + ", direction="
			+ ToString<int>(direction)  + "]");

        control->setValue(0.5f); //all joystick axis start at .5, so do that

		ControlAxisBinderItem controlAxisBinderItem;
		controlAxisBinderItem.control = control;
		controlAxisBinderItem.direction = direction;
		mControlsJoystickAxisBinderMap[deviceID][axis] = controlAxisBinderItem;
	}

	void InputControlSystem::addJoystickButtonBinding(Control* control, int deviceID, unsigned int button, Control::ControlChangingDirection direction)
	{
		if (std::find(mJoystickIDList.begin(), mJoystickIDList.end(), deviceID) == mJoystickIDList.end())
			mJoystickIDList.push_back(deviceID); // Hack: add the device to the list so bindings are saved in save() even when joystick is not connected

		ICS_LOG("\tAdding JoystickButtonBinder [button="
			+ ToString<int>(button)     + ", deviceID="
			+ ToString<int>(deviceID)   + ", direction="
			+ ToString<int>(direction)  + "]");

		ControlButtonBinderItem controlJoystickButtonBinderItem;
		controlJoystickButtonBinderItem.direction = direction;
		controlJoystickButtonBinderItem.control = control;
		mControlsJoystickButtonBinderMap[deviceID][button] = controlJoystickButtonBinderItem;
	}

	bool InputControlSystem::isJoystickButtonBound(int deviceID, unsigned int button) const
	{
		JoystickButtonBinderMapType::const_iterator found = mControlsJoystickButtonBinderMap.find(deviceID);
		if (found == mControlsJoystickButtonBinderMap.end())
			return false;

		return (found->second.find(button) != found->second.end());
	}

	bool InputControlSystem::isJoystickAxisBound(int deviceID, unsigned int axis) const
	{
		JoystickAxisBinderMapType::const_iterator found = mControlsJoystickAxisBinderMap.find(deviceID);
		if (found == mControlsJoystickAxisBinderMap.end())
			return false;

		return (found->second.find(axis) != found->second.end());
	}

	// get bindings
	int InputControlSystem::getJoystickAxisBinding(Control* control, int deviceID, ICS::Control::ControlChangingDirection direction)
	{
        if(mControlsJoystickAxisBinderMap.find(deviceID) != mControlsJoystickAxisBinderMap.end())
		{
            ControlsAxisBinderMapType::iterator it = mControlsJoystickAxisBinderMap[deviceID].begin();
            while(it != mControlsJoystickAxisBinderMap[deviceID].end())
            {
                if(it->first >= 0 && it->second.control == control && it->second.direction == direction)
                {
                    return it->first;
                }
                ++it;
            }
        }

		return /*NamedAxis::*/UNASSIGNED;
	}

	unsigned int InputControlSystem::getJoystickButtonBinding(Control* control, int deviceID, ICS::Control::ControlChangingDirection direction)
	{
        if(mControlsJoystickButtonBinderMap.find(deviceID) != mControlsJoystickButtonBinderMap.end())
		{
            ControlsButtonBinderMapType::iterator it = mControlsJoystickButtonBinderMap[deviceID].begin();
            while(it != mControlsJoystickButtonBinderMap[deviceID].end())
            {
                if(it->second.control == control && it->second.direction == direction)
                {
                    return it->first;
                }
                ++it;
            }
        }

		return ICS_MAX_DEVICE_BUTTONS;
	}

	// remove bindings
	void InputControlSystem::removeJoystickAxisBinding(int deviceID, int axis)
	{
        if(mControlsJoystickAxisBinderMap.find(deviceID) != mControlsJoystickAxisBinderMap.end())
		{
            ControlsAxisBinderMapType::iterator it = mControlsJoystickAxisBinderMap[deviceID].find(axis);
            if(it != mControlsJoystickAxisBinderMap[deviceID].end())
            {
                mControlsJoystickAxisBinderMap[deviceID].erase(it);
            }
        }
	}

	void InputControlSystem::removeJoystickButtonBinding(int deviceID, unsigned int button)
	{
        if(mControlsJoystickButtonBinderMap.find(deviceID) != mControlsJoystickButtonBinderMap.end())
		{
            ControlsButtonBinderMapType::iterator it = mControlsJoystickButtonBinderMap[deviceID].find(button);
            if(it != mControlsJoystickButtonBinderMap[deviceID].end())
            {
                mControlsJoystickButtonBinderMap[deviceID].erase(it);
            }
        }
	}

	// joyStick listeners
    void InputControlSystem::buttonPressed(int deviceID, const SDL_ControllerButtonEvent &evt)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
                if(mControlsJoystickButtonBinderMap.find(deviceID) != mControlsJoystickButtonBinderMap.end())
                {
                    ControlsButtonBinderMapType::const_iterator it = mControlsJoystickButtonBinderMap[deviceID].find(evt.button);
                    if(it != mControlsJoystickButtonBinderMap[deviceID].end())
                    {
                        it->second.control->setIgnoreAutoReverse(false);
                        if(!it->second.control->getAutoChangeDirectionOnLimitsAfterStop())
                        {
                            it->second.control->setChangingDirection(it->second.direction);
                        }
                        else
                        {
                            if(it->second.control->getValue() == 1)
                            {
                                it->second.control->setChangingDirection(Control::DECREASE);
                            }
                            else if(it->second.control->getValue() == 0)
                            {
                                it->second.control->setChangingDirection(Control::INCREASE);
                            }
                        }
                    }
                }
			}
		}
	}

    void InputControlSystem::buttonReleased(int deviceID, const SDL_ControllerButtonEvent &evt)
	{
		if(mActive)
		{
            if(!mDetectingBindingControl)
            {
                if(mControlsJoystickButtonBinderMap.find(deviceID) != mControlsJoystickButtonBinderMap.end())
                {
                    ControlsButtonBinderMapType::const_iterator it = mControlsJoystickButtonBinderMap[deviceID].find(evt.button);
                    if(it != mControlsJoystickButtonBinderMap[deviceID].end())
                    {
                        it->second.control->setChangingDirection(Control::STOP);
                    }
                }
            }
            else if(mDetectingBindingListener)
			{
				mDetectingBindingListener->joystickButtonBindingDetected(this, deviceID,
					mDetectingBindingControl, evt.button, mDetectingBindingDirection);
			}
		}
	}

    void InputControlSystem::axisMoved(int deviceID, const SDL_ControllerAxisEvent &evt)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
                if(mControlsJoystickAxisBinderMap.find(deviceID) != mControlsJoystickAxisBinderMap.end())
                {
                    ControlAxisBinderItem joystickBinderItem = mControlsJoystickAxisBinderMap[deviceID][evt.axis]; // joystic axis start at 0 index
                    Control* ctrl = joystickBinderItem.control;
                    if(ctrl)
                    {
                        ctrl->setIgnoreAutoReverse(true);

                        float axisRange = SDL_JOY_AXIS_MAX - SDL_JOY_AXIS_MIN;
                        float valDisplaced = (float)(evt.value - SDL_JOY_AXIS_MIN);
                        float percent = valDisplaced / axisRange * (1+mDeadZone*2) - mDeadZone; //Assures all values, 0 through 1, are seen
                        if(percent > .5-mDeadZone && percent < .5+mDeadZone) //close enough to center
                            percent = .5;
                        else if(percent > .5)
                            percent -= mDeadZone;
                        else
                            percent += mDeadZone;

                        if(joystickBinderItem.direction == Control::INCREASE)
                        {
                            ctrl->setValue( percent );
                        }
                        else if(joystickBinderItem.direction == Control::DECREASE)
                        {
                            ctrl->setValue( 1 - ( percent ) );
                        }
                    }
                }
			}
			else if(mDetectingBindingListener)
			{
				//ControlAxisBinderItem joystickBinderItem = mControlsJoystickAxisBinderMap[ evt.which ][ axis ]; // joystic axis start at 0 index
				//Control* ctrl = joystickBinderItem.control;
				//if(ctrl && ctrl->isAxisBindable())
				if(mDetectingBindingControl->isAxisBindable())
				{
					if( abs( evt.value ) > ICS_JOYSTICK_AXIS_BINDING_MARGIN)
					{
						mDetectingBindingListener->joystickAxisBindingDetected(this, deviceID,
							mDetectingBindingControl, evt.axis, mDetectingBindingDirection);
					}
				}
			}
		}
	}

    void InputControlSystem::controllerAdded(int deviceID, const SDL_ControllerDeviceEvent &args)
	{
		ICS_LOG("Adding joystick (index: " + ToString<int>(args.which) + ")");
		SDL_GameController* cntrl = SDL_GameControllerOpen(args.which);
        int instanceID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(cntrl));
        if(std::find(mJoystickIDList.begin(), mJoystickIDList.end(), deviceID)==mJoystickIDList.end())
        {
            for(int j = 0 ; j < ICS_MAX_JOYSTICK_AXIS ; j++)
            {
                if(mControlsJoystickAxisBinderMap[deviceID].find(j) == mControlsJoystickAxisBinderMap[deviceID].end())
                {
                    ControlAxisBinderItem controlJoystickBinderItem;
                    controlJoystickBinderItem.direction = Control::STOP;
                    controlJoystickBinderItem.control = NULL;
                    mControlsJoystickAxisBinderMap[deviceID][j] = controlJoystickBinderItem;
                }
            }
            mJoystickIDList.push_front(deviceID);
		}

		mJoystickInstanceMap[instanceID] = cntrl;
	}
	void InputControlSystem::controllerRemoved(const SDL_ControllerDeviceEvent &args)
	{
        ICS_LOG("Removing joystick (instance id: " + ToString<int>(args.which) + ")");
        if(mJoystickInstanceMap.count(args.which)!=0)
        {
            SDL_GameControllerClose(mJoystickInstanceMap.at(args.which));
            mJoystickInstanceMap.erase(args.which);
        }
	}

	// joystick auto bindings
	void DetectingBindingListener::joystickAxisBindingDetected(InputControlSystem* ICS, int deviceID, Control* control, int axis, Control::ControlChangingDirection direction)
	{
		// if the joystick axis is used by another control, remove it
		ICS->removeJoystickAxisBinding(deviceID, axis);

		// if the control has an axis assigned, remove it
		int oldAxis = ICS->getJoystickAxisBinding(control, deviceID, direction);
		if(oldAxis != InputControlSystem::UNASSIGNED)
		{
			ICS->removeJoystickAxisBinding(deviceID, oldAxis);
		}

		ICS->addJoystickAxisBinding(control, deviceID, axis, direction);
		ICS->cancelDetectingBindingState();
	}
	void DetectingBindingListener::joystickButtonBindingDetected(InputControlSystem* ICS, int deviceID, Control* control
		, unsigned int button, Control::ControlChangingDirection direction)
	{
		// if the joystick button is used by another control, remove it
		ICS->removeJoystickButtonBinding(deviceID, button);

		// if the control has a joystick button assigned, remove it
		unsigned int oldButton = ICS->getJoystickButtonBinding(control, deviceID, direction);
		if(oldButton != ICS_MAX_DEVICE_BUTTONS)
		{
			ICS->removeJoystickButtonBinding(deviceID, oldButton);
		}

		ICS->addJoystickButtonBinding(control, deviceID, button, direction);
		ICS->cancelDetectingBindingState();
	}
}
