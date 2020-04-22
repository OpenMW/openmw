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

namespace ICS
{
	// load xml
	void InputControlSystem::loadMouseAxisBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlMouseBinder = xmlControlNode->FirstChildElement("MouseBinder");    
		while(xmlMouseBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlMouseBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlMouseBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			NamedAxis axis = /*NamedAxis::*/ X;
			if((*xmlMouseBinder->Attribute("axis")) == 'Y')
			{
				axis = /*NamedAxis::*/ Y;
			} 
			else if((*xmlMouseBinder->Attribute("axis")) == 'Z')
			{
				axis = /*NamedAxis::*/ Z;
			} 

			addMouseAxisBinding(mControls.back(), axis, dir);

			xmlMouseBinder = xmlMouseBinder->NextSiblingElement("MouseBinder");
		}
	}

	void InputControlSystem::loadMouseButtonBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlMouseButtonBinder = xmlControlNode->FirstChildElement("MouseButtonBinder");    
		while(xmlMouseButtonBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlMouseButtonBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlMouseButtonBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			int button = 0;
			if(std::string(xmlMouseButtonBinder->Attribute("button")) == "LEFT")
			{
				button = SDL_BUTTON_LEFT;
			} 
			else if(std::string(xmlMouseButtonBinder->Attribute("button")) == "RIGHT")
			{
				button = SDL_BUTTON_RIGHT;
			} 
			else if(std::string(xmlMouseButtonBinder->Attribute("button")) == "MIDDLE")
			{
				button = SDL_BUTTON_MIDDLE;
			} 
			else
			{
				button = FromString<int>(xmlMouseButtonBinder->Attribute("button"));
			}

			addMouseButtonBinding(mControls.back(), button, dir);

			xmlMouseButtonBinder = xmlMouseButtonBinder->NextSiblingElement("MouseButtonBinder");
		}
	}


	// add bindings
	void InputControlSystem::addMouseAxisBinding(Control* control, NamedAxis axis, Control::ControlChangingDirection direction)
	{
		if(axis == /*NamedAxis::*/X)
		{
			mXmouseAxisBinded = true;
		}
		else if(axis == /*NamedAxis::*/Y)
		{
			mYmouseAxisBinded = true;
		}		

		addMouseAxisBinding_(control, axis, direction);
	}

	/*protected*/ void InputControlSystem::addMouseAxisBinding_(Control* control, int axis, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding AxisBinder [axis="
			+ ToString<int>(axis) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlAxisBinderItem controlAxisBinderItem;
		controlAxisBinderItem.control = control;
		controlAxisBinderItem.direction = direction;
		mControlsMouseAxisBinderMap[ axis ] = controlAxisBinderItem; 
	}

	void InputControlSystem::addMouseButtonBinding(Control* control, unsigned int button, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding MouseButtonBinder [button="
			+ ToString<int>(button) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlButtonBinderItem controlMouseButtonBinderItem;
		controlMouseButtonBinderItem.direction = direction;
		controlMouseButtonBinderItem.control = control;
		mControlsMouseButtonBinderMap[ button ] = controlMouseButtonBinderItem;
	}

	bool InputControlSystem::isMouseButtonBound(unsigned int button) const
	{
		return mControlsMouseButtonBinderMap.find(button) != mControlsMouseButtonBinderMap.end();
	}

	// get bindings
	InputControlSystem::NamedAxis InputControlSystem::getMouseAxisBinding(Control* control, ICS::Control::ControlChangingDirection direction)
	{
		ControlsAxisBinderMapType::iterator it = mControlsMouseAxisBinderMap.begin();
		while(it != mControlsMouseAxisBinderMap.end())
		{
			if(it->first < 0 && it->second.control == control && it->second.direction == direction)
			{
				return (InputControlSystem::NamedAxis)(it->first);
			}
			++it;
		}

		return /*NamedAxis::*/UNASSIGNED;
	}

	//int InputControlSystem::getMouseAxisBinding(Control* control, ICS::Control::ControlChangingDirection direction)
	//{
	//	ControlsAxisBinderMapType::iterator it = mControlsMouseAxisBinderMap.begin();
	//	while(it != mControlsMouseAxisBinderMap.end())
	//	{
	//		if(it->first >= 0 && it->second.control == control && it->second.direction == direction)
	//		{
	//			return it->first;
	//		}
	//		it++;
	//	}

	//	return /*NamedAxis::*/UNASSIGNED;
	//}

	unsigned int InputControlSystem::getMouseButtonBinding(Control* control, ICS::Control::ControlChangingDirection direction)
	{
		ControlsButtonBinderMapType::iterator it = mControlsMouseButtonBinderMap.begin();
		while(it != mControlsMouseButtonBinderMap.end())
		{
			if(it->second.control == control && it->second.direction == direction)
			{
				return it->first;
			}
			++it;
		}

		return ICS_MAX_DEVICE_BUTTONS;
	}

	// remove bindings
	void InputControlSystem::removeMouseAxisBinding(NamedAxis axis)
	{
		if(axis == /*NamedAxis::*/X)
		{
			mXmouseAxisBinded = false;
		}
		else if(axis == /*NamedAxis::*/Y)
		{
			mYmouseAxisBinded = false;
		}		

		removeMouseAxisBinding_(axis);
	}
	/*protected*/ void InputControlSystem::removeMouseAxisBinding_(int axis)
	{
		ControlsAxisBinderMapType::iterator it = mControlsMouseAxisBinderMap.find(axis);
		if(it != mControlsMouseAxisBinderMap.end())
		{
			mControlsMouseAxisBinderMap.erase(it);
		}
	}		


	void InputControlSystem::removeMouseButtonBinding(unsigned int button)
	{
		ControlsButtonBinderMapType::iterator it = mControlsMouseButtonBinderMap.find(button);
		if(it != mControlsMouseButtonBinderMap.end())
		{
			mControlsMouseButtonBinderMap.erase(it);
		}
	}

	// mouse Listeners
    void InputControlSystem::mouseMoved(const SDL_MouseMotionEvent& evt)
	{
		if(mActive)
		{
            if(!mDetectingBindingControl)
			{
				if(mXmouseAxisBinded && evt.xrel)
				{
					ControlAxisBinderItem mouseBinderItem = mControlsMouseAxisBinderMap[ /*NamedAxis::*/X ];
					Control* ctrl = mouseBinderItem.control;
					ctrl->setIgnoreAutoReverse(true);
					if(mouseBinderItem.direction == Control::INCREASE)
					{
						ctrl->setValue( float( (evt.x) / float(mClientWidth) ) );
					}
					else if(mouseBinderItem.direction == Control::DECREASE)
					{
						ctrl->setValue( 1 - float( evt.x / float(mClientWidth) ) );
					}
				}

				if(mYmouseAxisBinded && evt.yrel)
				{
					ControlAxisBinderItem mouseBinderItem = mControlsMouseAxisBinderMap[ /*NamedAxis::*/Y ];
					Control* ctrl = mouseBinderItem.control;
					ctrl->setIgnoreAutoReverse(true);
					if(mouseBinderItem.direction == Control::INCREASE)
					{
						ctrl->setValue( float( (evt.y) / float(mClientHeight) ) );
					}
					else if(mouseBinderItem.direction == Control::DECREASE)
					{
						ctrl->setValue( 1 - float( evt.y / float(mClientHeight) ) );
					}
				}

				//! @todo Whats the range of the Z axis?
				/*if(evt.state.Z.rel)
				{
					ControlAxisBinderItem mouseBinderItem = mControlsAxisBinderMap[ NamedAxis::Z ];
					Control* ctrl = mouseBinderItem.control;
					ctrl->setIgnoreAutoReverse(true);
					if(mouseBinderItem.direction == Control::INCREASE)
					{
						ctrl->setValue( float( (evt.state.Z.abs) / float(evt.state.¿width?) ) );
					}
					else if(mouseBinderItem.direction == Control::DECREASE)
					{
						ctrl->setValue( float( (1 - evt.state.Z.abs) / float(evt.state.¿width?) ) );
					}
				}*/
			}
			else if(mDetectingBindingListener)
			{
				if(mDetectingBindingControl->isAxisBindable())
				{
					if(mMouseAxisBindingInitialValues[0] == ICS_MOUSE_AXIS_BINDING_NULL_VALUE)
					{
						mMouseAxisBindingInitialValues[0] = 0;
						mMouseAxisBindingInitialValues[1] = 0;
						mMouseAxisBindingInitialValues[2] = 0;
					}

					mMouseAxisBindingInitialValues[0] += evt.xrel;
					mMouseAxisBindingInitialValues[1] += evt.yrel;
                    // mMouseAxisBindingInitialValues[2] += evt.zrel;

					if( abs(mMouseAxisBindingInitialValues[0]) > ICS_MOUSE_BINDING_MARGIN )
					{
						mDetectingBindingListener->mouseAxisBindingDetected(this,
							mDetectingBindingControl, X, mDetectingBindingDirection);
					}
					else if( abs(mMouseAxisBindingInitialValues[1]) > ICS_MOUSE_BINDING_MARGIN )
					{
						mDetectingBindingListener->mouseAxisBindingDetected(this,
							mDetectingBindingControl, Y, mDetectingBindingDirection);
					}
					else if( abs(mMouseAxisBindingInitialValues[2]) > ICS_MOUSE_BINDING_MARGIN )
					{
						mDetectingBindingListener->mouseAxisBindingDetected(this,
							mDetectingBindingControl, Z, mDetectingBindingDirection);		
					}
				}
			}
		}
	}

    void InputControlSystem::mousePressed(const SDL_MouseButtonEvent &evt, Uint8 btn)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
				ControlsButtonBinderMapType::const_iterator it = mControlsMouseButtonBinderMap.find((int)btn);
				if(it != mControlsMouseButtonBinderMap.end())
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

    void InputControlSystem::mouseReleased(const SDL_MouseButtonEvent &evt, Uint8 btn)
	{		
		if(mActive)
		{
            if (!mDetectingBindingControl)
            {
                ControlsButtonBinderMapType::const_iterator it = mControlsMouseButtonBinderMap.find((int)btn);
                if(it != mControlsMouseButtonBinderMap.end())
                {
                    it->second.control->setChangingDirection(Control::STOP);
                }
            }
            else if(mDetectingBindingListener)
            {
                mDetectingBindingListener->mouseButtonBindingDetected(this,
                    mDetectingBindingControl, btn, mDetectingBindingDirection);
            }
		}
	}

	// mouse auto bindings
	void DetectingBindingListener::mouseAxisBindingDetected(InputControlSystem* ICS, Control* control
			, InputControlSystem::NamedAxis axis, Control::ControlChangingDirection direction)
	{
		// if the mouse axis is used by another control, remove it
		ICS->removeMouseAxisBinding(axis);

		// if the control has an axis assigned, remove it
		InputControlSystem::NamedAxis oldAxis = ICS->getMouseAxisBinding(control, direction);
		if(oldAxis != InputControlSystem::UNASSIGNED) 
		{
			ICS->removeMouseAxisBinding(oldAxis);
		}

		ICS->addMouseAxisBinding(control, axis, direction);
		ICS->cancelDetectingBindingState();
	}

	void DetectingBindingListener::mouseButtonBindingDetected(InputControlSystem* ICS, Control* control
			, unsigned int button, Control::ControlChangingDirection direction)
	{
		// if the mouse button is used by another control, remove it
		ICS->removeMouseButtonBinding(button);

		// if the control has a mouse button assigned, remove it
		unsigned int oldButton = ICS->getMouseButtonBinding(control, direction);
		if(oldButton != ICS_MAX_DEVICE_BUTTONS)
		{
			ICS->removeMouseButtonBinding(oldButton);
		}

		ICS->addMouseButtonBinding(control, button, direction);
		ICS->cancelDetectingBindingState();
	}

	/* ----------------------------------------------------------------------------------------
	* OPENMW CODE STARTS HERE
	* Mouse Wheel support added by Michael Stopa (Stomy) */

	void InputControlSystem::loadMouseWheelBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlMouseWheelBinder = xmlControlNode->FirstChildElement("MouseWheelBinder");
		while (xmlMouseWheelBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if (std::string(xmlMouseWheelBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if (std::string(xmlMouseWheelBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

			MouseWheelClick click = MouseWheelClick::UNASSIGNED;
			if (std::string(xmlMouseWheelBinder->Attribute("button")) == "UP")
			{
				click = MouseWheelClick::UP;
			}
			else if (std::string(xmlMouseWheelBinder->Attribute("button")) == "DOWN")
			{
				click = MouseWheelClick::DOWN;
			}
			else if (std::string(xmlMouseWheelBinder->Attribute("button")) == "DOWN")
			{
				click = MouseWheelClick::RIGHT;
			}
			else if (std::string(xmlMouseWheelBinder->Attribute("button")) == "DOWN")
			{
				click = MouseWheelClick::LEFT;
			}

			addMouseWheelBinding(mControls.back(), click, dir);
			xmlMouseWheelBinder = xmlMouseWheelBinder->NextSiblingElement("MouseWheelBinder");
		}
	}

	void InputControlSystem::addMouseWheelBinding(
			Control* control,
			MouseWheelClick click,
			Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding MouseWheelBinder [button="
			+ ToString<int>(static_cast<int>(click)) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlButtonBinderItem controlButtonBinderItem;
		controlButtonBinderItem.control = control;
		controlButtonBinderItem.direction = direction;
		mControlsMouseWheelBinderMap[static_cast<int>(click)] = controlButtonBinderItem;
	}

	void InputControlSystem::removeMouseWheelBinding(MouseWheelClick click)
	{
		ControlsAxisBinderMapType::iterator it = mControlsMouseWheelBinderMap.find(static_cast<int>(click));
		if (it != mControlsMouseWheelBinderMap.end())
		{
			mControlsMouseWheelBinderMap.erase(it);
		}
	}

	InputControlSystem::MouseWheelClick InputControlSystem::getMouseWheelBinding(
			Control* control,
			ICS::Control::ControlChangingDirection direction)
	{
		ControlsAxisBinderMapType::iterator it = mControlsMouseWheelBinderMap.begin();
		while (it != mControlsMouseWheelBinderMap.end())
		{
			if (it->first > 0 && it->second.control == control && it->second.direction == direction)
			{
				return (MouseWheelClick)(it->first);
			}
			++it;
		}

		return MouseWheelClick::UNASSIGNED;
	}

	bool InputControlSystem::isMouseWheelBound(MouseWheelClick button) const
	{
		return mControlsMouseWheelBinderMap.find(static_cast<int>(button)) != mControlsMouseWheelBinderMap.end();
	}

	void InputControlSystem::mouseWheelMoved(const SDL_MouseWheelEvent &evt)
	{
		if (mActive)
		{
			MouseWheelClick click = MouseWheelClick::UNASSIGNED;
			int value;
			if (evt.y != 0)
			{
				value = evt.y;
				if (evt.direction == SDL_MOUSEWHEEL_FLIPPED)
					value *= -1;
				if (value > 0)
					click = MouseWheelClick::UP;
				else
					click = MouseWheelClick::DOWN;
			}
			else if (evt.x != 0)
			{
				value = evt.x;
				if (evt.direction == SDL_MOUSEWHEEL_FLIPPED)
					value *= -1;
				if (value > 0)
					click = MouseWheelClick::RIGHT;
				else
					click = MouseWheelClick::LEFT;
			}
			else
				return;

			if(!mDetectingBindingControl)
			{
				// This assumes a single event is sent for every single mouse wheel direction, if they are
				// buffered up then all bindings will have to be resolved on every invocation of this function

				ControlButtonBinderItem wheelBinderItem = mControlsMouseWheelBinderMap[static_cast<int>(click)];
				Control* control = wheelBinderItem.control;
				if (control)
				{
					control->setIgnoreAutoReverse(false);
					if (wheelBinderItem.direction == Control::INCREASE)
					{
						control->setValue(static_cast<float>(std::abs(value)));
						control->setChangingDirection(Control::STOP);
					}
					else if (wheelBinderItem.direction == Control::DECREASE)
					{
						control->setValue(static_cast<float>(std::abs(value)));
						control->setChangingDirection(Control::STOP);
					}
				}
			}
			else if(mDetectingBindingListener)
			{
				mDetectingBindingListener->mouseWheelBindingDetected(this,
					mDetectingBindingControl, click, mDetectingBindingDirection);
			}
		}
	}

	void DetectingBindingListener::mouseWheelBindingDetected(
			InputControlSystem* ICS,
			Control* control,
			InputControlSystem::MouseWheelClick click,
			Control::ControlChangingDirection direction)
	{
		ICS->removeMouseWheelBinding(click);

		InputControlSystem::MouseWheelClick oldClick = ICS->getMouseWheelBinding(control, direction);
		if (oldClick != InputControlSystem::MouseWheelClick::UNASSIGNED)
		{
			ICS->removeMouseWheelBinding(oldClick);
		}

		ICS->addMouseWheelBinding(control, click, direction);
		ICS->cancelDetectingBindingState();
	}

	/* OPENMW CODE ENDS HERE
	* ------------------------------------------------------------------------------------- */
}
