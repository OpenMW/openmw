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

			addJoystickAxisBinding(mControls.back(), FromString<int>(xmlJoystickBinder->Attribute("deviceId"))
				, FromString<int>(xmlJoystickBinder->Attribute("axis")), dir);

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

			addJoystickButtonBinding(mControls.back(), FromString<int>(xmlJoystickButtonBinder->Attribute("deviceId"))
				, FromString<int>(xmlJoystickButtonBinder->Attribute("button")), dir);

			xmlJoystickButtonBinder = xmlJoystickButtonBinder->NextSiblingElement("JoystickButtonBinder");
		}
	}

	void InputControlSystem::loadJoystickPOVBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlJoystickPOVBinder = xmlControlNode->FirstChildElement("JoystickPOVBinder");    
		while(xmlJoystickPOVBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlJoystickPOVBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlJoystickPOVBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			InputControlSystem::POVAxis axis = /*POVAxis::*/NorthSouth;
			if(std::string(xmlJoystickPOVBinder->Attribute("axis")) == "EastWest")
			{
				axis = /*POVAxis::*/EastWest;
			}

			addJoystickPOVBinding(mControls.back(), FromString<int>(xmlJoystickPOVBinder->Attribute("deviceId"))
				, FromString<int>(xmlJoystickPOVBinder->Attribute("pov")), axis, dir);

			xmlJoystickPOVBinder = xmlJoystickPOVBinder->NextSiblingElement("JoystickPOVBinder");
		}
	}

	void InputControlSystem::loadJoystickSliderBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlJoystickSliderBinder = xmlControlNode->FirstChildElement("JoystickSliderBinder");    
		while(xmlJoystickSliderBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlJoystickSliderBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlJoystickSliderBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			addJoystickSliderBinding(mControls.back(), FromString<int>(xmlJoystickSliderBinder->Attribute("deviceId"))
				, FromString<int>(xmlJoystickSliderBinder->Attribute("slider")), dir);

			xmlJoystickSliderBinder = xmlJoystickSliderBinder->NextSiblingElement("JoystickSliderBinder");
		}
	}

	// add bindings
	void InputControlSystem::addJoystickAxisBinding(Control* control, int deviceId, int axis, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding AxisBinder [deviceid="
			+ ToString<int>(deviceId) + ", axis="
			+ ToString<int>(axis) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlAxisBinderItem controlAxisBinderItem;
		controlAxisBinderItem.control = control;
		controlAxisBinderItem.direction = direction;
		mControlsJoystickAxisBinderMap[ deviceId ][ axis ] = controlAxisBinderItem; 
	}

	void InputControlSystem::addJoystickButtonBinding(Control* control, int deviceId, unsigned int button, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding JoystickButtonBinder [deviceId="
			+ ToString<int>(deviceId) + ", button="
			+ ToString<int>(button) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlButtonBinderItem controlJoystickButtonBinderItem;
		controlJoystickButtonBinderItem.direction = direction;
		controlJoystickButtonBinderItem.control = control;
		mControlsJoystickButtonBinderMap[ deviceId ][ button ] = controlJoystickButtonBinderItem;
	}

	void InputControlSystem::addJoystickPOVBinding(Control* control, int deviceId, int index, InputControlSystem::POVAxis axis, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding JoystickPOVBinder [deviceId="
			+ ToString<int>(deviceId) + ", pov="
			+ ToString<int>(index) + ", axis="
			+ ToString<int>(axis) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlPOVBinderItem ControlPOVBinderItem;
		ControlPOVBinderItem.direction = direction;
		ControlPOVBinderItem.control = control;
		mControlsJoystickPOVBinderMap[ deviceId ][ index ][ axis ] = ControlPOVBinderItem;
	}

	void InputControlSystem::addJoystickSliderBinding(Control* control, int deviceId, int index, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding JoystickSliderBinder [deviceId="
			+ ToString<int>(deviceId) + ", direction="
			+ ToString<int>(index) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlSliderBinderItem ControlSliderBinderItem;
		ControlSliderBinderItem.direction = direction;
		ControlSliderBinderItem.control = control;
		mControlsJoystickSliderBinderMap[ deviceId ][ index ] = ControlSliderBinderItem;
	}

	// get bindings
	int InputControlSystem::getJoystickAxisBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction)
	{
		if(mControlsJoystickAxisBinderMap.find(deviceId) != mControlsJoystickAxisBinderMap.end())
		{
			ControlsAxisBinderMapType::iterator it = mControlsJoystickAxisBinderMap[deviceId].begin();
			while(it != mControlsJoystickAxisBinderMap[deviceId].end())
			{
				if(it->first >= 0 && it->second.control == control && it->second.direction == direction)
				{
					return it->first;
				}
				it++;
			}
		}

		return /*NamedAxis::*/UNASSIGNED;
	}

	unsigned int InputControlSystem::getJoystickButtonBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction)
	{
		if(mControlsJoystickButtonBinderMap.find(deviceId) != mControlsJoystickButtonBinderMap.end())
		{
			ControlsButtonBinderMapType::iterator it = mControlsJoystickButtonBinderMap[deviceId].begin();
			while(it != mControlsJoystickButtonBinderMap[deviceId].end())
			{
				if(it->second.control == control && it->second.direction == direction)
				{
					return it->first;
				}
				it++;
			}
		}

		return ICS_MAX_DEVICE_BUTTONS;
	}

	InputControlSystem::POVBindingPair InputControlSystem::getJoystickPOVBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction)
	{
		POVBindingPair result;
		result.index = -1;

		if(mControlsJoystickPOVBinderMap.find(deviceId) != mControlsJoystickPOVBinderMap.end())
		{
			//ControlsAxisBinderMapType::iterator it = mControlsJoystickPOVBinderMap[deviceId].begin();
			std::map<int, ControlsPOVBinderMapType>::iterator it = mControlsJoystickPOVBinderMap[deviceId].begin();
			while(it != mControlsJoystickPOVBinderMap[deviceId].end())
			{
				ControlsPOVBinderMapType::const_iterator it2 = it->second.begin();
				while(it2 != it->second.end())
				{
					if(it2->second.control == control && it2->second.direction == direction)
					{
						result.index = it->first;
						result.axis = (POVAxis)it2->first;
						return result;
					}
					it2++;
				}
				
				it++;
			}
		}

		return result;
	}

	int InputControlSystem::getJoystickSliderBinding(Control* control, int deviceId, ICS::Control::ControlChangingDirection direction)
	{
		if(mControlsJoystickSliderBinderMap.find(deviceId) != mControlsJoystickSliderBinderMap.end())
		{
			ControlsButtonBinderMapType::iterator it = mControlsJoystickSliderBinderMap[deviceId].begin();
			while(it != mControlsJoystickSliderBinderMap[deviceId].end())
			{
				if(it->second.control == control && it->second.direction == direction)
				{
					return it->first;
				}
				it++;
			}
		}

		return /*NamedAxis::*/UNASSIGNED;
	}

	// remove bindings
	void InputControlSystem::removeJoystickAxisBinding(int deviceId, int axis)
	{
		if(mControlsJoystickAxisBinderMap.find(deviceId) != mControlsJoystickAxisBinderMap.end())
		{
			ControlsButtonBinderMapType::iterator it = mControlsJoystickAxisBinderMap[deviceId].find(axis);
			if(it != mControlsJoystickAxisBinderMap[deviceId].end())
			{
				mControlsJoystickAxisBinderMap[deviceId].erase(it);
			}
		}
	}

	void InputControlSystem::removeJoystickButtonBinding(int deviceId, unsigned int button)
	{
		if(mControlsJoystickButtonBinderMap.find(deviceId) != mControlsJoystickButtonBinderMap.end())
		{
			ControlsButtonBinderMapType::iterator it = mControlsJoystickButtonBinderMap[deviceId].find(button);
			if(it != mControlsJoystickButtonBinderMap[deviceId].end())
			{
				mControlsJoystickButtonBinderMap[deviceId].erase(it);
			}
		}
	}

	void InputControlSystem::removeJoystickPOVBinding(int deviceId, int index, POVAxis axis)
	{
		if(mControlsJoystickPOVBinderMap.find(deviceId) != mControlsJoystickPOVBinderMap.end())
		{
			std::map<int, ControlsPOVBinderMapType>::iterator it = mControlsJoystickPOVBinderMap[deviceId].find(index);
			if(it != mControlsJoystickPOVBinderMap[deviceId].end())
			{
				if(it->second.find(axis) != it->second.end())
				{
					mControlsJoystickPOVBinderMap[deviceId].find(index)->second.erase( it->second.find(axis) );
				}
			}
		}
	}

	void InputControlSystem::removeJoystickSliderBinding(int deviceId, int index)
	{
		if(mControlsJoystickSliderBinderMap.find(deviceId) != mControlsJoystickSliderBinderMap.end())
		{
			ControlsButtonBinderMapType::iterator it = mControlsJoystickSliderBinderMap[deviceId].find(index);
			if(it != mControlsJoystickSliderBinderMap[deviceId].end())
			{
				mControlsJoystickSliderBinderMap[deviceId].erase(it);
			}
		}
	}

	// joyStick listeners
    void InputControlSystem::buttonPressed(const SDL_JoyButtonEvent &evt, int button)
	{
		if(mActive) 
		{
			if(!mDetectingBindingControl)
			{
				if(mControlsJoystickButtonBinderMap.find(evt.which) != mControlsJoystickButtonBinderMap.end())
				{
					ControlsButtonBinderMapType::const_iterator it = mControlsJoystickButtonBinderMap[evt.which].find(button);
					if(it != mControlsJoystickButtonBinderMap[evt.which].end())
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
			else if(mDetectingBindingListener)
			{
				mDetectingBindingListener->joystickButtonBindingDetected(this,
					mDetectingBindingControl, evt.which, button, mDetectingBindingDirection);
			}
		}
	}

    void InputControlSystem::buttonReleased(const SDL_JoyButtonEvent &evt, int button)
	{
		if(mActive)
		{
			if(mControlsJoystickButtonBinderMap.find(evt.which) != mControlsJoystickButtonBinderMap.end())
			{
				ControlsButtonBinderMapType::const_iterator it = mControlsJoystickButtonBinderMap[evt.which].find(button);
				if(it != mControlsJoystickButtonBinderMap[evt.which].end())
				{
					it->second.control->setChangingDirection(Control::STOP);
				}
			}
		}
	}

    void InputControlSystem::axisMoved(const SDL_JoyAxisEvent &evt, int axis)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
				if(mControlsJoystickAxisBinderMap.find(evt.which) != mControlsJoystickAxisBinderMap.end())
				{
					ControlAxisBinderItem joystickBinderItem = mControlsJoystickAxisBinderMap[ evt.which ][ axis ]; // joystic axis start at 0 index
					Control* ctrl = joystickBinderItem.control;
					if(ctrl)
					{
						ctrl->setIgnoreAutoReverse(true);

						float axisRange = SDL_JOY_AXIS_MAX - SDL_JOY_AXIS_MIN;
						float valDisplaced = (float)(evt.value - SDL_JOY_AXIS_MIN);

						if(joystickBinderItem.direction == Control::INCREASE)
						{
							ctrl->setValue( valDisplaced / axisRange );
						}
						else if(joystickBinderItem.direction == Control::DECREASE)
						{
							ctrl->setValue( 1 - ( valDisplaced / axisRange ) );
						}
					}
				}
			}
			else if(mDetectingBindingListener)
			{
				//ControlAxisBinderItem joystickBinderItem = mControlsJoystickAxisBinderMap[ evt.which ][ axis ]; // joystic axis start at 0 index
				//Control* ctrl = joystickBinderItem.control;
				//if(ctrl && ctrl->isAxisBindable())
				if(mDetectingBindingControl && mDetectingBindingControl->isAxisBindable())
				{
					if( abs( evt.value ) > ICS_JOYSTICK_AXIS_BINDING_MARGIN)
					{
						mDetectingBindingListener->joystickAxisBindingDetected(this,
							mDetectingBindingControl, evt.which, axis, mDetectingBindingDirection);
					}
				}
			}
		}
	}

	//Here be dragons, apparently
    void InputControlSystem::povMoved(const SDL_JoyHatEvent &evt, int index)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
				if(mControlsJoystickPOVBinderMap.find(evt.which) != mControlsJoystickPOVBinderMap.end())
				{
					std::map<int, ControlsPOVBinderMapType>::const_iterator i = mControlsJoystickPOVBinderMap[ evt.which ].find(index);
					if(i != mControlsJoystickPOVBinderMap[ evt.which ].end())
					{
						if(evt.value != SDL_HAT_LEFT
							&& evt.value != SDL_HAT_RIGHT
							&& evt.value != SDL_HAT_CENTERED)
						{
							ControlsPOVBinderMapType::const_iterator it = i->second.find( /*POVAxis::*/NorthSouth );
							if(it != i->second.end())
							{
								it->second.control->setIgnoreAutoReverse(false);
								if(!it->second.control->getAutoChangeDirectionOnLimitsAfterStop())
								{
									if(evt.value == SDL_HAT_UP
										|| evt.value == SDL_HAT_LEFTUP
										|| evt.value == SDL_HAT_RIGHTUP)
									{
										it->second.control->setChangingDirection(it->second.direction);
									}
									else
									{
										it->second.control->setChangingDirection((Control::ControlChangingDirection)(-1 * it->second.direction));
									}
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

						if(evt.value != SDL_HAT_UP
							&& evt.value != SDL_HAT_DOWN
							&& evt.value != SDL_HAT_CENTERED)
						{
							ControlsPOVBinderMapType::const_iterator it = i->second.find( /*POVAxis::*/EastWest );
							if(it != i->second.end())
							{
								it->second.control->setIgnoreAutoReverse(false);
								if(!it->second.control->getAutoChangeDirectionOnLimitsAfterStop())
								{
									if(evt.value == SDL_HAT_RIGHT
										|| evt.value == SDL_HAT_RIGHTUP
										|| evt.value == SDL_HAT_RIGHTDOWN)
									{
										it->second.control->setChangingDirection(it->second.direction);
									}
									else
									{
										it->second.control->setChangingDirection((Control::ControlChangingDirection)(-1 * it->second.direction));
									}
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

						if(evt.value == SDL_HAT_CENTERED)
						{
							ControlsPOVBinderMapType::const_iterator it = i->second.find( /*POVAxis::*/NorthSouth );
							if(it != i->second.end())
							{
								it->second.control->setChangingDirection(Control::STOP);
							}

							it = i->second.find( /*POVAxis::*/EastWest );
							if(it != i->second.end())
							{
								it->second.control->setChangingDirection(Control::STOP);
							}
						}
					}
				}
			}
			else if(mDetectingBindingListener)
			{
				if(mDetectingBindingControl && mDetectingBindingControl->isAxisBindable())
				{
					if(evt.value == SDL_HAT_LEFT
						|| evt.value == SDL_HAT_RIGHT
						|| evt.value == SDL_HAT_UP
						|| evt.value == SDL_HAT_DOWN)
					{
						POVAxis povAxis = NorthSouth;
						if(evt.value == SDL_HAT_LEFT
							|| evt.value == SDL_HAT_RIGHT)
						{
							povAxis = EastWest;
						}

						mDetectingBindingListener->joystickPOVBindingDetected(this,
								mDetectingBindingControl, evt.which, index, povAxis, mDetectingBindingDirection);
					}
				}
			}
		}
	}

	//TODO: does this have an SDL equivalent?
	/*
    void InputControlSystem::sliderMoved(const OIS::JoyStickEvent &evt, int index)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
				if(mControlsJoystickSliderBinderMap.find(evt.device->getID()) != mControlsJoystickSliderBinderMap.end())
				{
					ControlSliderBinderItem joystickBinderItem = mControlsJoystickSliderBinderMap[ evt.device->getID() ][ index ];
					Control* ctrl = joystickBinderItem.control;
					if(ctrl)
					{
						ctrl->setIgnoreAutoReverse(true);
						if(joystickBinderItem.direction == Control::INCREASE)
						{
							float axisRange = OIS::JoyStick::MAX_AXIS - OIS::JoyStick::MIN_AXIS;
							float valDisplaced = (float)( evt.state.mSliders[index].abX - OIS::JoyStick::MIN_AXIS);

							ctrl->setValue( valDisplaced / axisRange );
						}
						else if(joystickBinderItem.direction == Control::DECREASE)
						{
							float axisRange = OIS::JoyStick::MAX_AXIS - OIS::JoyStick::MIN_AXIS;
							float valDisplaced = (float)(evt.state.mSliders[index].abX - OIS::JoyStick::MIN_AXIS);

							ctrl->setValue( 1 - ( valDisplaced / axisRange ) );
						}
					}
				}
			}
			else if(mDetectingBindingListener)
			{
				if(mDetectingBindingControl && mDetectingBindingControl->isAxisBindable())
				{
					if( abs( evt.state.mSliders[index].abX ) > ICS_JOYSTICK_SLIDER_BINDING_MARGIN)
					{
						mDetectingBindingListener->joystickSliderBindingDetected(this,
							mDetectingBindingControl, evt.device->getID(), index, mDetectingBindingDirection);
					}
				}
			}
		}
	}
	*/

	// joystick auto bindings
	void DetectingBindingListener::joystickAxisBindingDetected(InputControlSystem* ICS, Control* control
		, int deviceId, int axis, Control::ControlChangingDirection direction)
	{
		// if the joystick axis is used by another control, remove it
		ICS->removeJoystickAxisBinding(deviceId, axis);

		// if the control has an axis assigned, remove it
		int oldAxis = ICS->getJoystickAxisBinding(control, deviceId, direction);
		if(oldAxis != InputControlSystem::UNASSIGNED) 
		{
			ICS->removeJoystickAxisBinding(deviceId, oldAxis);
		}

		ICS->addJoystickAxisBinding(control, deviceId, axis, direction);
		ICS->cancelDetectingBindingState();
	}
	void DetectingBindingListener::joystickButtonBindingDetected(InputControlSystem* ICS, Control* control
		, int deviceId, unsigned int button, Control::ControlChangingDirection direction)
	{
		// if the joystick button is used by another control, remove it
		ICS->removeJoystickButtonBinding(deviceId, button);

		// if the control has a joystick button assigned, remove it
		unsigned int oldButton = ICS->getJoystickButtonBinding(control, deviceId, direction);
		if(oldButton != ICS_MAX_DEVICE_BUTTONS)
		{
			ICS->removeJoystickButtonBinding(deviceId, oldButton);
		}

		ICS->addJoystickButtonBinding(control, deviceId, button, direction);
		ICS->cancelDetectingBindingState();
	}


	void DetectingBindingListener::joystickPOVBindingDetected(InputControlSystem* ICS, Control* control
		, int deviceId, int pov, InputControlSystem::POVAxis axis, Control::ControlChangingDirection direction)
	{
		// if the joystick slider is used by another control, remove it
		ICS->removeJoystickPOVBinding(deviceId, pov, axis);

		// if the control has a joystick button assigned, remove it
		ICS::InputControlSystem::POVBindingPair oldPOV = ICS->getJoystickPOVBinding(control, deviceId, direction);
		if(oldPOV.index >= 0 && oldPOV.axis == axis)
		{
			ICS->removeJoystickPOVBinding(deviceId, oldPOV.index, oldPOV.axis);
		}

		ICS->addJoystickPOVBinding(control, deviceId, pov, axis, direction);
		ICS->cancelDetectingBindingState();
	}

	void DetectingBindingListener::joystickSliderBindingDetected(InputControlSystem* ICS, Control* control
		, int deviceId, int slider, Control::ControlChangingDirection direction)
	{
		// if the joystick slider is used by another control, remove it
		ICS->removeJoystickSliderBinding(deviceId, slider);

		// if the control has a joystick slider assigned, remove it
		int oldSlider = ICS->getJoystickSliderBinding(control, deviceId, direction);
		if(oldSlider != InputControlSystem::/*NamedAxis::*/UNASSIGNED)
		{
			ICS->removeJoystickSliderBinding(deviceId, oldSlider);
		}

		ICS->addJoystickSliderBinding(control, deviceId, slider, direction);
		ICS->cancelDetectingBindingState();
	}
}
