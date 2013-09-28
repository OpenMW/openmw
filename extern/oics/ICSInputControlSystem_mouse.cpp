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
				button = OIS::/*MouseButtonID::*/MB_Left;
			} 
			else if(std::string(xmlMouseButtonBinder->Attribute("button")) == "RIGHT")
			{
				button = OIS::/*MouseButtonID::*/MB_Right;
			} 
			else if(std::string(xmlMouseButtonBinder->Attribute("button")) == "MIDDLE")
			{
				button = OIS::/*MouseButtonID::*/MB_Middle;
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
			it++;
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
			it++;
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
	bool InputControlSystem::mouseMoved(const OIS::MouseEvent &evt) 
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
				if(mXmouseAxisBinded && evt.state.X.rel)
				{
					ControlAxisBinderItem mouseBinderItem = mControlsMouseAxisBinderMap[ /*NamedAxis::*/X ];
					Control* ctrl = mouseBinderItem.control;
					ctrl->setIgnoreAutoReverse(true);
					if(mouseBinderItem.direction == Control::INCREASE)
					{
						ctrl->setValue( float( (evt.state.X.abs) / float(evt.state.width) ) );
					}
					else if(mouseBinderItem.direction == Control::DECREASE)
					{
						ctrl->setValue( 1 - float( evt.state.X.abs / float(evt.state.width) ) );
					}
				}

				if(mYmouseAxisBinded && evt.state.Y.rel)
				{
					ControlAxisBinderItem mouseBinderItem = mControlsMouseAxisBinderMap[ /*NamedAxis::*/Y ];
					Control* ctrl = mouseBinderItem.control;
					ctrl->setIgnoreAutoReverse(true);
					if(mouseBinderItem.direction == Control::INCREASE)
					{
						ctrl->setValue( float( (evt.state.Y.abs) / float(evt.state.height) ) );
					}
					else if(mouseBinderItem.direction == Control::DECREASE)
					{
						ctrl->setValue( 1 - float( evt.state.Y.abs / float(evt.state.height) ) );
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
						ctrl->setValue( float( (evt.state.Z.abs) / float(evt.state.�width?) ) );
					}
					else if(mouseBinderItem.direction == Control::DECREASE)
					{
						ctrl->setValue( float( (1 - evt.state.Z.abs) / float(evt.state.�width?) ) );
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

					mMouseAxisBindingInitialValues[0] += evt.state.X.rel;
					mMouseAxisBindingInitialValues[1] += evt.state.Y.rel;
					mMouseAxisBindingInitialValues[2] += evt.state.Z.rel;

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

		return true;
	}

	bool InputControlSystem::mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID btn) 
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
			else if(mDetectingBindingListener)
			{
				mDetectingBindingListener->mouseButtonBindingDetected(this,
					mDetectingBindingControl, btn, mDetectingBindingDirection);
			}
		}

		return true;
	}

	bool InputControlSystem::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID btn) 
	{		
		if(mActive)
		{
			ControlsButtonBinderMapType::const_iterator it = mControlsMouseButtonBinderMap.find((int)btn);
			if(it != mControlsMouseButtonBinderMap.end())
			{
				it->second.control->setChangingDirection(Control::STOP);
			}
		}

		return true;
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

}