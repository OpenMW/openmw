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
	const float ICS_MAX = std::numeric_limits<float>::max();

	InputControlSystem::InputControlSystem(std::string file, bool active
		, DetectingBindingListener* detectingBindingListener
		, InputControlSystemLog* log, size_t channelCount)
		: mFileName(file)
		, mLog(log)
		, mDetectingBindingListener(detectingBindingListener)
		, mDetectingBindingControl(NULL)
        , mDetectingBindingDirection(Control::STOP)
        , mXmouseAxisBinded(false), mYmouseAxisBinded(false)
        , mClientWidth(1)
        , mClientHeight(1)
	{
		ICS_LOG(" - Creating InputControlSystem - ");

		this->mActive = active;

		ICS_LOG("Channel count = " + ToString<size_t>(channelCount) );
		for(size_t i=0;i<channelCount;i++)
		{
			mChannels.push_back(new Channel((int)i));
		}

		if(file != "")
		{
			TiXmlDocument* xmlDoc;
			TiXmlElement* xmlRoot;

			ICS_LOG("Loading file \""+file+"\"");

			xmlDoc = new TiXmlDocument(file.c_str());
			xmlDoc->LoadFile();

			if(xmlDoc->Error())
			{
				std::ostringstream message;
				message << "TinyXml reported an error reading \""+ file + "\". Row " <<
					(int)xmlDoc->ErrorRow() << ", Col " << (int)xmlDoc->ErrorCol() << ": " <<
					xmlDoc->ErrorDesc() ;
				ICS_LOG(message.str());

				delete xmlDoc;
				return;
			}

			xmlRoot = xmlDoc->RootElement();
			if(std::string(xmlRoot->Value()) != "Controller") {
				ICS_LOG("Error: Invalid Controller file. Missing <Controller> element.");
				delete xmlDoc;
				return;
			}

			TiXmlElement* xmlControl = xmlRoot->FirstChildElement("Control");

	        size_t controlChannelCount = 0;
			while(xmlControl)
	        {
	            TiXmlElement* xmlChannel = xmlControl->FirstChildElement("Channel");
				while(xmlChannel)
				{
					controlChannelCount = std::max(channelCount, FromString<size_t>(xmlChannel->Attribute("number"))+1);

					xmlChannel = xmlChannel->NextSiblingElement("Channel");
				}

	            xmlControl = xmlControl->NextSiblingElement("Control");
	        }

			static const size_t channelsCountLimit = 65536;
			if (controlChannelCount > channelsCountLimit)
			{
				ICS_LOG("Warning: requested channels count (" + ToString<size_t>(controlChannelCount) + ") exceeds allowed maximum (" + ToString<size_t>(channelsCountLimit) + "), clamping it");
				controlChannelCount = channelsCountLimit;
			}

			if(controlChannelCount > channelCount)
			{
				size_t dif = controlChannelCount - channelCount;
				ICS_LOG("Warning: default channel count exceeded. Adding " + ToString<size_t>(dif) + " channels" );
				for(size_t i = channelCount ; i < controlChannelCount ; i++)
				{
					mChannels.push_back(new Channel((int)i));
				}
			}

			ICS_LOG("Applying filters to channels");
			//<ChannelFilter number="0">
			//	<interval type="bezier" startX="0.0" startY="0.0" midX="0.25" midY="0.5" endX="0.5" endY="0.5" step="0.1" />
			//	<interval type="bezier" startX="0.5" startY="0.5" midX="0.75" midY="0.5" endX="1.0" endY="1.0" step="0.1" />
			//</ChannelFilter>

			TiXmlElement* xmlChannelFilter = xmlRoot->FirstChildElement("ChannelFilter");
			while(xmlChannelFilter)
			{
				size_t ch = FromString<size_t>(xmlChannelFilter->Attribute("number"));
				if(ch >= controlChannelCount)
				{
					ICS_LOG("ERROR: channel number (ch) is out of range");
					xmlChannelFilter = xmlChannelFilter->NextSiblingElement("ChannelFilter");
					continue;
				}

				TiXmlElement* xmlInterval = xmlChannelFilter->FirstChildElement("Interval");
				while(xmlInterval)
				{
					std::string type = xmlInterval->Attribute("type");

					if(type == "bezier")
					{
						float startX = FromString<float>(xmlInterval->Attribute("startX"));
						float startY = FromString<float>(xmlInterval->Attribute("startY"));
						float midX = FromString<float>(xmlInterval->Attribute("midX"));
						float midY = FromString<float>(xmlInterval->Attribute("midY"));
						float endX = FromString<float>(xmlInterval->Attribute("endX"));
						float endY = FromString<float>(xmlInterval->Attribute("endY"));

                        float step = FromString<float>(xmlInterval->Attribute("step"));

						ICS_LOG("Applying Bezier filter to channel [number="
							+ ToString<size_t>(ch) + ", startX="
							+ ToString<float>(startX) + ", startY="
							+ ToString<float>(startY) + ", midX="
							+ ToString<float>(midX) + ", midY="
							+ ToString<float>(midY) + ", endX="
							+ ToString<float>(endX) + ", endY="
							+ ToString<float>(endY) + ", step="
							+ ToString<float>(step) + "]");

						mChannels.at(ch)->addBezierInterval(startX, startY, midX, midY, endX, endY, step);
					}

					xmlInterval = xmlInterval->NextSiblingElement("Interval");
				}

				xmlChannelFilter = xmlChannelFilter->NextSiblingElement("ChannelFilter");
			}

			xmlControl = xmlRoot->FirstChildElement("Control");
			while(xmlControl)
			{
				bool axisBindable = true;
				if(xmlControl->Attribute("axisBindable"))
				{
					axisBindable = (std::string( xmlControl->Attribute("axisBindable") ) == "true");
				}

				ICS_LOG("Adding Control [name="
					+ std::string( xmlControl->Attribute("name") ) + ", autoChangeDirectionOnLimitsAfterStop="
					+ std::string( xmlControl->Attribute("autoChangeDirectionOnLimitsAfterStop") ) + ", autoReverseToInitialValue="
					+ std::string( xmlControl->Attribute("autoReverseToInitialValue") ) + ", initialValue="
					+ std::string( xmlControl->Attribute("initialValue") ) + ", stepSize="
					+ std::string( xmlControl->Attribute("stepSize") ) + ", stepsPerSeconds="
					+ std::string( xmlControl->Attribute("stepsPerSeconds") ) + ", axisBindable="
					+ std::string( (axisBindable)? "true" : "false" ) + "]");

				float _stepSize = 0;
				if(xmlControl->Attribute("stepSize"))
				{
					std::string value(xmlControl->Attribute("stepSize"));
					if(value == "MAX")
					{
						_stepSize = ICS_MAX;
					}
					else
					{
						_stepSize = FromString<float>(value.c_str());
					}
				}
				else
				{
					ICS_LOG("Warning: no stepSize value found. Default value is 0.");
				}

				float _stepsPerSeconds = 0;
				if(xmlControl->Attribute("stepsPerSeconds"))
				{
					std::string value(xmlControl->Attribute("stepsPerSeconds"));
					if(value == "MAX")
					{
						_stepsPerSeconds = ICS_MAX;
					}
					else
					{
						_stepsPerSeconds = FromString<float>(value.c_str());
					}
				}
				else
				{
					ICS_LOG("Warning: no stepSize value found. Default value is 0.");
				}

				addControl( new Control(xmlControl->Attribute("name")
					, std::string( xmlControl->Attribute("autoChangeDirectionOnLimitsAfterStop") ) == "true"
					, std::string( xmlControl->Attribute("autoReverseToInitialValue") ) == "true"
					, FromString<float>(xmlControl->Attribute("initialValue"))
					, _stepSize
					, _stepsPerSeconds
					, axisBindable) );

				loadKeyBinders(xmlControl);

				loadMouseAxisBinders(xmlControl);

				loadMouseButtonBinders(xmlControl);

				loadJoystickAxisBinders(xmlControl);

				loadJoystickButtonBinders(xmlControl);

				/* ----------------------------------------------------------------------------------------
				 * OPENMW CODE STARTS HERE
				 * Mouse Wheel support added by Michael Stopa (Stomy) */

				loadMouseWheelBinders(xmlControl);

				/* OPENMW CODE ENDS HERE
				 * ------------------------------------------------------------------------------------- */

				// Attach controls to channels
				TiXmlElement* xmlChannel = xmlControl->FirstChildElement("Channel");
				while(xmlChannel)
				{
					ICS_LOG("\tAttaching control to channel [number="
						+ std::string( xmlChannel->Attribute("number") ) + ", direction="
						+ std::string( xmlChannel->Attribute("direction") ) + "]");

					float percentage = 1;
					if(xmlChannel->Attribute("percentage"))
					{
						if(StringIsNumber<float>(xmlChannel->Attribute("percentage")))
						{
							float val = FromString<float>(xmlChannel->Attribute("percentage"));
							if(val > 1 || val < 0)
							{
								ICS_LOG("ERROR: attaching percentage value range is [0,1]");
							}
							else
							{
								percentage = val;
							}
						}
						else
						{
							ICS_LOG("ERROR: attaching percentage value range is [0,1]");
						}
					}

					size_t chNumber = FromString<size_t>(xmlChannel->Attribute("number"));
					if(chNumber >= controlChannelCount)
					{
						ICS_LOG("ERROR: channel number (chNumber) is out of range");
					}
					else
					{
						if(std::string(xmlChannel->Attribute("direction")) == "DIRECT")
						{
							mControls.back()->attachChannel(mChannels[ chNumber ],Channel::DIRECT, percentage);
						}
						else if(std::string(xmlChannel->Attribute("direction")) == "INVERSE")
						{
							mControls.back()->attachChannel(mChannels[ chNumber ],Channel::INVERSE, percentage);
						}
					}

					xmlChannel = xmlChannel->NextSiblingElement("Channel");
				}

				xmlControl = xmlControl->NextSiblingElement("Control");
			}

			std::vector<Channel *>::const_iterator o;
			for(o = mChannels.begin(); o != mChannels.end(); ++o)
			{
				(*o)->update();
			}

			delete xmlDoc;
		}

		ICS_LOG(" - InputControlSystem Created - ");
	}

	InputControlSystem::~InputControlSystem()
	{
		ICS_LOG(" - Deleting InputControlSystem (" + mFileName + ") - ");

		mJoystickIDList.clear();

		std::vector<Channel *>::const_iterator o;
		for(o = mChannels.begin(); o != mChannels.end(); ++o)
		{
			delete (*o);
		}
		mChannels.clear();

		std::vector<Control *>::const_iterator o2;
		for(o2 = mControls.begin(); o2 != mControls.end(); ++o2)
		{
			delete (*o2);
		}
		mControls.clear();

		mControlsKeyBinderMap.clear();
		mControlsMouseButtonBinderMap.clear();
		mControlsJoystickButtonBinderMap.clear();

		/* ----------------------------------------------------------------------------------------
		 * OPENMW CODE STARTS HERE
		 * Mouse Wheel support added by Michael Stopa (Stomy) */

		mControlsMouseWheelBinderMap.clear();

		/* OPENMW CODE ENDS HERE
		 * ------------------------------------------------------------------------------------- */

		ICS_LOG(" - InputControlSystem deleted - ");
	}

	std::string InputControlSystem::getBaseFileName()
	{
		size_t found = mFileName.find_last_of("/\\");
		std::string file = mFileName.substr(found+1);

		return file.substr(0, file.find_last_of("."));
	}

	bool InputControlSystem::save(std::string fileName)
	{
		if(fileName != "")
		{
			mFileName = fileName;
		}

		TiXmlDocument doc(  mFileName.c_str() );

		TiXmlDeclaration dec;
		dec.Parse( "<?xml version='1.0' encoding='utf-8'?>", 0, TIXML_ENCODING_UNKNOWN );
		doc.InsertEndChild(dec);

		TiXmlElement Controller( "Controller" );

        for(std::vector<Channel*>::const_iterator o = mChannels.begin() ; o != mChannels.end(); ++o)
		{
			ICS::IntervalList intervals = (*o)->getIntervals();

			if(intervals.size() > 1) // all channels have a default linear filter
			{
				TiXmlElement ChannelFilter( "ChannelFilter" );

				ChannelFilter.SetAttribute("number", ToString<int>((*o)->getNumber()).c_str());

				ICS::IntervalList::const_iterator interval = intervals.begin();
				while( interval != intervals.end() )
				{
					// if not default linear filter
					if(!( interval->step == 0.2f
						&& interval->startX == 0.0f
						&& interval->startY == 0.0f
						&& interval->midX == 0.5f
						&& interval->midY == 0.5f
						&& interval->endX == 1.0f
						&& interval->endY == 1.0f ))
					{
						TiXmlElement XMLInterval( "Interval" );

						XMLInterval.SetAttribute("type", "bezier");
						XMLInterval.SetAttribute("step", ToString<float>(interval->step).c_str());

						XMLInterval.SetAttribute("startX", ToString<float>(interval->startX).c_str());
						XMLInterval.SetAttribute("startY", ToString<float>(interval->startY).c_str());
						XMLInterval.SetAttribute("midX", ToString<float>(interval->midX).c_str());
						XMLInterval.SetAttribute("midY", ToString<float>(interval->midY).c_str());
						XMLInterval.SetAttribute("endX", ToString<float>(interval->endX).c_str());
						XMLInterval.SetAttribute("endY", ToString<float>(interval->endY).c_str());

						ChannelFilter.InsertEndChild(XMLInterval);
					}

                    ++interval;
				}

				Controller.InsertEndChild(ChannelFilter);
			}
		}

        for(std::vector<Control*>::const_iterator o = mControls.begin() ; o != mControls.end(); ++o)
		{
			TiXmlElement control( "Control" );

			control.SetAttribute( "name", (*o)->getName().c_str() );
			if((*o)->getAutoChangeDirectionOnLimitsAfterStop())
			{
				control.SetAttribute( "autoChangeDirectionOnLimitsAfterStop", "true" );
			}
			else
			{
				control.SetAttribute( "autoChangeDirectionOnLimitsAfterStop", "false" );
			}
			if((*o)->getAutoReverse())
			{
				control.SetAttribute( "autoReverseToInitialValue", "true" );
			}
			else
			{
				control.SetAttribute( "autoReverseToInitialValue", "false" );
			}
			control.SetAttribute( "initialValue", ToString<float>((*o)->getInitialValue()).c_str() );

			if((*o)->getStepSize() == ICS_MAX)
			{
				control.SetAttribute( "stepSize", "MAX" );
			}
			else
			{
				control.SetAttribute( "stepSize", ToString<float>((*o)->getStepSize()).c_str() );
			}

			if((*o)->getStepsPerSeconds() == ICS_MAX)
			{
				control.SetAttribute( "stepsPerSeconds", "MAX" );
			}
			else
			{
				control.SetAttribute( "stepsPerSeconds", ToString<float>((*o)->getStepsPerSeconds()).c_str() );
			}

			if(!(*o)->isAxisBindable())
			{
				control.SetAttribute( "axisBindable", "false" );
			}

            if(getKeyBinding(*o, Control/*::ControlChangingDirection*/::INCREASE) != SDL_SCANCODE_UNKNOWN)
			{
				TiXmlElement keyBinder( "KeyBinder" );

                keyBinder.SetAttribute( "key", ToString<int>(
					getKeyBinding(*o, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );
				keyBinder.SetAttribute( "direction", "INCREASE" );
				control.InsertEndChild(keyBinder);
			}

            if(getKeyBinding(*o, Control/*::ControlChangingDirection*/::DECREASE) != SDL_SCANCODE_UNKNOWN)
			{
				TiXmlElement keyBinder( "KeyBinder" );

                keyBinder.SetAttribute( "key", ToString<int>(
					getKeyBinding(*o, Control/*::ControlChangingDirection*/::DECREASE)).c_str() );
				keyBinder.SetAttribute( "direction", "DECREASE" );
				control.InsertEndChild(keyBinder);
			}

			if(getMouseAxisBinding(*o, Control/*::ControlChangingDirection*/::INCREASE)
				!= InputControlSystem/*::NamedAxis*/::UNASSIGNED)
			{
				TiXmlElement binder( "MouseBinder" );

				InputControlSystem::NamedAxis axis =
					getMouseAxisBinding(*o, Control/*::ControlChangingDirection*/::INCREASE);
				if(axis == InputControlSystem/*::NamedAxis*/::X)
				{
					binder.SetAttribute( "axis", "X" );
				}
				else if(axis == InputControlSystem/*::NamedAxis*/::Y)
				{
					binder.SetAttribute( "axis", "Y" );
				}
				else if(axis == InputControlSystem/*::NamedAxis*/::Z)
				{
					binder.SetAttribute( "axis", "Z" );
				}

				binder.SetAttribute( "direction", "INCREASE" );
				control.InsertEndChild(binder);
			}

			if(getMouseAxisBinding(*o, Control/*::ControlChangingDirection*/::DECREASE)
				!= InputControlSystem/*::NamedAxis*/::UNASSIGNED)
			{
				TiXmlElement binder( "MouseBinder" );

				InputControlSystem::NamedAxis axis =
					getMouseAxisBinding(*o, Control/*::ControlChangingDirection*/::DECREASE);
				if(axis == InputControlSystem/*::NamedAxis*/::X)
				{
					binder.SetAttribute( "axis", "X" );
				}
				else if(axis == InputControlSystem/*::NamedAxis*/::Y)
				{
					binder.SetAttribute( "axis", "Y" );
				}
				else if(axis == InputControlSystem/*::NamedAxis*/::Z)
				{
					binder.SetAttribute( "axis", "Z" );
				}

				binder.SetAttribute( "direction", "DECREASE" );
				control.InsertEndChild(binder);
			}

			/* ----------------------------------------------------------------------------------------
			 * OPENMW CODE STARTS HERE
			 * Mouse Wheel support added by Stomy */

			if(getMouseWheelBinding(*o, Control::INCREASE) != MouseWheelClick::UNASSIGNED)
			{
				TiXmlElement binder( "MouseWheelBinder" );
				MouseWheelClick click = getMouseWheelBinding(*o, Control::INCREASE);
				bool skip = false;
				switch (click) {
					case MouseWheelClick::UP: {
						binder.SetAttribute("button", "UP");
					} break;
					case MouseWheelClick::DOWN: {
						binder.SetAttribute("button", "DOWN");
					} break;
					case MouseWheelClick::RIGHT: {
						binder.SetAttribute("button", "RIGHT");
					} break;
					case MouseWheelClick::LEFT: {
						binder.SetAttribute("button", "LEFT");
					} break;
					default: {
						skip = true;
					} break;
				}
				binder.SetAttribute( "direction", "INCREASE" );
				if (!skip)
					control.InsertEndChild(binder);
			}

			if(getMouseWheelBinding(*o, Control::DECREASE) != MouseWheelClick::UNASSIGNED)
			{
				TiXmlElement binder( "MouseWheelBinder" );
				MouseWheelClick click = getMouseWheelBinding(*o, Control::INCREASE);
				bool skip = false;
				switch (click) {
					case MouseWheelClick::UP: {
						binder.SetAttribute("button", "UP");
					} break;
					case MouseWheelClick::DOWN: {
						binder.SetAttribute("button", "DOWN");
					} break;
					case MouseWheelClick::RIGHT: {
						binder.SetAttribute("button", "RIGHT");
					} break;
					case MouseWheelClick::LEFT: {
						binder.SetAttribute("button", "LEFT");
					} break;
					default: {
						skip = true;
					} break;
				}
				binder.SetAttribute( "direction", "DECREASE" );
				if (!skip)
					control.InsertEndChild(binder);
			}

			/* OPENMW CODE ENDS HERE
			 * ------------------------------------------------------------------------------------- */

			if(getMouseButtonBinding(*o, Control/*::ControlChangingDirection*/::INCREASE)
				!= ICS_MAX_DEVICE_BUTTONS)
			{
				TiXmlElement binder( "MouseButtonBinder" );

				unsigned int button = getMouseButtonBinding(*o, Control/*::ControlChangingDirection*/::INCREASE);
				if(button == SDL_BUTTON_LEFT)
				{
					binder.SetAttribute( "button", "LEFT" );
				}
				else if(button == SDL_BUTTON_MIDDLE)
				{
					binder.SetAttribute( "button", "MIDDLE" );
				}
				else if(button == SDL_BUTTON_RIGHT)
				{
					binder.SetAttribute( "button", "RIGHT" );
				}
				else
				{
					binder.SetAttribute( "button", ToString<unsigned int>(button).c_str() );
				}
				binder.SetAttribute( "direction", "INCREASE" );
				control.InsertEndChild(binder);
			}

			if(getMouseButtonBinding(*o, Control/*::ControlChangingDirection*/::DECREASE)
				!= ICS_MAX_DEVICE_BUTTONS)
			{
				TiXmlElement binder( "MouseButtonBinder" );

				unsigned int button = getMouseButtonBinding(*o, Control/*::ControlChangingDirection*/::DECREASE);
				if(button == SDL_BUTTON_LEFT)
				{
					binder.SetAttribute( "button", "LEFT" );
				}
				else if(button == SDL_BUTTON_MIDDLE)
				{
					binder.SetAttribute( "button", "MIDDLE" );
				}
				else if(button == SDL_BUTTON_RIGHT)
				{
					binder.SetAttribute( "button", "RIGHT" );
				}
				else
				{
					binder.SetAttribute( "button", ToString<unsigned int>(button).c_str() );
				}
				binder.SetAttribute( "direction", "DECREASE" );
				control.InsertEndChild(binder);
			}
			JoystickIDList::const_iterator it = mJoystickIDList.begin();
			while(it!=mJoystickIDList.end())
			{
                int deviceID = *it;
                if(getJoystickAxisBinding(*o, deviceID, Control/*::ControlChangingDirection*/::INCREASE)
                    != /*NamedAxis::*/UNASSIGNED)
                {
                    TiXmlElement binder( "JoystickAxisBinder" );

                    binder.SetAttribute( "axis", ToString<int>(
                        getJoystickAxisBinding(*o, deviceID, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );

                    binder.SetAttribute( "direction", "INCREASE" );

                    binder.SetAttribute( "deviceId", deviceID ); //completely useless, but required for backwards compatability

                    control.InsertEndChild(binder);
                }

                if(getJoystickAxisBinding(*o, deviceID, Control/*::ControlChangingDirection*/::DECREASE)
                    != /*NamedAxis::*/UNASSIGNED)
                {
                    TiXmlElement binder( "JoystickAxisBinder" );

                    binder.SetAttribute( "axis", ToString<int>(
                        getJoystickAxisBinding(*o, deviceID, Control/*::ControlChangingDirection*/::DECREASE)).c_str() );

                    binder.SetAttribute( "direction", "DECREASE" );

                    binder.SetAttribute( "deviceId", deviceID ); //completely useless, but required for backwards compatability

                    control.InsertEndChild(binder);
                }

                if(getJoystickButtonBinding(*o, deviceID, Control/*::ControlChangingDirection*/::INCREASE)
                    != ICS_MAX_DEVICE_BUTTONS)
                {
                    TiXmlElement binder( "JoystickButtonBinder" );

                    binder.SetAttribute( "button", ToString<unsigned int>(
                        getJoystickButtonBinding(*o, deviceID, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );

                    binder.SetAttribute( "direction", "INCREASE" );

                    binder.SetAttribute( "deviceId", deviceID ); //completely useless, but required for backwards compatability

                    control.InsertEndChild(binder);
                }

                if(getJoystickButtonBinding(*o, deviceID, Control/*::ControlChangingDirection*/::DECREASE)
                    != ICS_MAX_DEVICE_BUTTONS)
                {
                    TiXmlElement binder( "JoystickButtonBinder" );

                    binder.SetAttribute( "button", ToString<unsigned int>(
                        getJoystickButtonBinding(*o, deviceID, Control/*::ControlChangingDirection*/::DECREASE)).c_str() );

                    binder.SetAttribute( "direction", "DECREASE" );

                    binder.SetAttribute( "deviceId", deviceID ); //completely useless, but required for backwards compatability

                    control.InsertEndChild(binder);
                }
                it++;
            }


			std::list<Channel*> channels = (*o)->getAttachedChannels();
			for(std::list<Channel*>::iterator it = channels.begin() ;
                it != channels.end() ; ++it)
			{
				TiXmlElement binder( "Channel" );

				binder.SetAttribute( "number", ToString<int>((*it)->getNumber()).c_str() );

				Channel::ChannelDirection direction = (*it)->getAttachedControlBinding(*o).direction;
				if(direction == Channel/*::ChannelDirection*/::DIRECT)
				{
					binder.SetAttribute( "direction", "DIRECT" );
				}
				else
				{
					binder.SetAttribute( "direction", "INVERSE" );
				}

				float percentage = (*it)->getAttachedControlBinding(*o).percentage;
				binder.SetAttribute( "percentage", ToString<float>(percentage).c_str() );

				control.InsertEndChild(binder);
			}

			Controller.InsertEndChild(control);
		}

		doc.InsertEndChild(Controller);
		return doc.SaveFile();
	}

	void InputControlSystem::update(float lTimeSinceLastFrame)
	{
		if(mActive)
		{
			std::vector<Control *>::const_iterator it;
			for(it=mControls.begin(); it!=mControls.end(); ++it)
			{
				(*it)->update(lTimeSinceLastFrame);
			}
		}

		//! @todo Future versions should consider channel exponentials and mixtures, so
		// after updating Controls, Channels should be updated according to their values
	}

	float InputControlSystem::getChannelValue(int i)
	{
		return std::max<float>(0.0,std::min<float>(1.0,mChannels[i]->getValue()));
	}

	float InputControlSystem::getControlValue(int i)
	{
		return mControls[i]->getValue();
	}

	Control* InputControlSystem::findControl(std::string name)
	{
		if(mActive)
		{
			std::vector<Control *>::const_iterator it;
			for(it = mControls.begin(); it != mControls.end(); ++it)
			{
				if( ((Control*)(*it))->getName() == name)
				{
					return (Control*)(*it);
				}
			}
		}

		return NULL;
	}

	void InputControlSystem::enableDetectingBindingState(Control* control
		, Control::ControlChangingDirection direction)
	{
		mDetectingBindingControl = control;
		mDetectingBindingDirection = direction;

		mMouseAxisBindingInitialValues[0] = ICS_MOUSE_AXIS_BINDING_NULL_VALUE;
	}

    bool InputControlSystem::detectingBindingState()
    {
        return mDetectingBindingControl != NULL;
    }

	void InputControlSystem::cancelDetectingBindingState()
	{
		mDetectingBindingControl = NULL;
	}

    std::string InputControlSystem::scancodeToString(SDL_Scancode key)
	{
        SDL_Keycode code = SDL_GetKeyFromScancode(key);
        if (code == SDLK_UNKNOWN)
            return std::string(SDL_GetScancodeName(key));
        else
            return std::string(SDL_GetKeyName(code));
	}

    void InputControlSystem::adjustMouseRegion(Uint16 width, Uint16 height)
    {
        mClientWidth = width;
        mClientHeight = height;
    }
}
