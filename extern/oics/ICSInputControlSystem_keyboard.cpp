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
	void InputControlSystem::loadKeyBinders(TiXmlElement* xmlControlNode)
	{
		TiXmlElement* xmlKeyBinder = xmlControlNode->FirstChildElement("KeyBinder");    
		while(xmlKeyBinder)
		{
			Control::ControlChangingDirection dir = Control::STOP;
			if(std::string(xmlKeyBinder->Attribute("direction")) == "INCREASE")
			{
				dir = Control::INCREASE;
			}
			else if(std::string(xmlKeyBinder->Attribute("direction")) == "DECREASE")
			{
				dir = Control::DECREASE;
			}

            addKeyBinding(mControls.back(), SDL_Scancode(FromString<int>(xmlKeyBinder->Attribute("key"))), dir);

			xmlKeyBinder = xmlKeyBinder->NextSiblingElement("KeyBinder");
		}
	}

    void InputControlSystem::addKeyBinding(Control* control, SDL_Scancode key, Control::ControlChangingDirection direction)
	{
		ICS_LOG("\tAdding KeyBinder [key="
            + scancodeToString(key) + ", direction="
			+ ToString<int>(direction) + "]");

		ControlKeyBinderItem controlKeyBinderItem;        
		controlKeyBinderItem.control = control;
		controlKeyBinderItem.direction = direction;
		mControlsKeyBinderMap[ key ] = controlKeyBinderItem;
	}

    bool InputControlSystem::isKeyBound(SDL_Scancode key) const
    {
        return mControlsKeyBinderMap.find(key) != mControlsKeyBinderMap.end();
    }

    void InputControlSystem::removeKeyBinding(SDL_Scancode key)
	{
		ControlsKeyBinderMapType::iterator it = mControlsKeyBinderMap.find(key);
		if(it != mControlsKeyBinderMap.end())
		{
			mControlsKeyBinderMap.erase(it);
		}
	}

    SDL_Scancode InputControlSystem::getKeyBinding(Control* control
		, ICS::Control::ControlChangingDirection direction)
	{
		ControlsKeyBinderMapType::iterator it = mControlsKeyBinderMap.begin();
		while(it != mControlsKeyBinderMap.end())
		{
			if(it->second.control == control && it->second.direction == direction)
			{
				return it->first;
			}
            ++it;
		}

        return SDL_SCANCODE_UNKNOWN;
	}
    void InputControlSystem::keyPressed(const SDL_KeyboardEvent &evt)
	{
		if(mActive)
		{
			if(!mDetectingBindingControl)
			{
                ControlsKeyBinderMapType::const_iterator it = mControlsKeyBinderMap.find(evt.keysym.scancode);
				if(it != mControlsKeyBinderMap.end())
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
				mDetectingBindingListener->keyBindingDetected(this,
                    mDetectingBindingControl, evt.keysym.scancode, mDetectingBindingDirection);
			}
		}
    }

    void InputControlSystem::keyReleased(const SDL_KeyboardEvent &evt)
	{
		if(mActive)
		{
            ControlsKeyBinderMapType::const_iterator it = mControlsKeyBinderMap.find(evt.keysym.scancode);
			if(it != mControlsKeyBinderMap.end())
			{
				it->second.control->setChangingDirection(Control::STOP);
			}
		}
	}

	void DetectingBindingListener::keyBindingDetected(InputControlSystem* ICS, Control* control
        , SDL_Scancode key, Control::ControlChangingDirection direction)
	{
		// if the key is used by another control, remove it
		ICS->removeKeyBinding(key);

		// if the control has a key assigned, remove it
        SDL_Scancode oldKey = ICS->getKeyBinding(control, direction);
        if(oldKey != SDL_SCANCODE_UNKNOWN)
		{
			ICS->removeKeyBinding(oldKey);
		}

		ICS->addKeyBinding(control, key, direction);
		ICS->cancelDetectingBindingState();
	}

}
