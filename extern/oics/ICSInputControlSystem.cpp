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
	InputControlSystem::InputControlSystem(std::string file, bool active
		, DetectingBindingListener* detectingBindingListener
		, InputControlSystemLog* log, size_t channelCount)
		: mFileName(file)
		, mDetectingBindingListener(detectingBindingListener)
		, mDetectingBindingControl(NULL)
		, mLog(log)
		, mXmouseAxisBinded(false), mYmouseAxisBinded(false)
	{
		ICS_LOG(" - Creating InputControlSystem - ");

		this->mActive = active;

		this->fillOISKeysMap();

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
	                controlChannelCount = std::max(channelCount, FromString<size_t>(xmlChannel->Attribute("number")));

					xmlChannel = xmlChannel->NextSiblingElement("Channel");
				}

	            xmlControl = xmlControl->NextSiblingElement("Control");
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
				int ch = FromString<int>(xmlChannelFilter->Attribute("number"));

				TiXmlElement* xmlInterval = xmlChannelFilter->FirstChildElement("Interval");
				while(xmlInterval)
				{
					std::string type = xmlInterval->Attribute("type");

					if(type == "bezier")
					{
						float step = 0.1;

						float startX = FromString<float>(xmlInterval->Attribute("startX"));
						float startY = FromString<float>(xmlInterval->Attribute("startY"));
						float midX = FromString<float>(xmlInterval->Attribute("midX"));
						float midY = FromString<float>(xmlInterval->Attribute("midY"));
						float endX = FromString<float>(xmlInterval->Attribute("endX"));
						float endY = FromString<float>(xmlInterval->Attribute("endY"));

						step = FromString<float>(xmlInterval->Attribute("step"));

						ICS_LOG("Applying Bezier filter to channel [number="
							+ ToString<int>(ch) + ", startX=" 
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

				loadJoystickPOVBinders(xmlControl);

				loadJoystickSliderBinders(xmlControl);

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

					int chNumber = FromString<int>(xmlChannel->Attribute("number"));
					if(std::string(xmlChannel->Attribute("direction")) == "DIRECT")
					{
						mControls.back()->attachChannel(mChannels[ chNumber ],Channel::DIRECT, percentage);
					}
					else if(std::string(xmlChannel->Attribute("direction")) == "INVERSE")
					{
						mControls.back()->attachChannel(mChannels[ chNumber ],Channel::INVERSE, percentage);
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

		mKeys.clear();
		mKeyCodes.clear();

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

		for(std::vector<Channel*>::const_iterator o = mChannels.begin() ; o != mChannels.end(); o++)
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
					
					interval++;
				}

				Controller.InsertEndChild(ChannelFilter);
			}
		}

		for(std::vector<Control*>::const_iterator o = mControls.begin() ; o != mControls.end(); o++)
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

			if(getKeyBinding(*o, Control/*::ControlChangingDirection*/::INCREASE) != OIS::KC_UNASSIGNED)
			{
				TiXmlElement keyBinder( "KeyBinder" );

				keyBinder.SetAttribute( "key", keyCodeToString(
					getKeyBinding(*o, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );
				keyBinder.SetAttribute( "direction", "INCREASE" );
				control.InsertEndChild(keyBinder);
			}

			if(getKeyBinding(*o, Control/*::ControlChangingDirection*/::DECREASE) != OIS::KC_UNASSIGNED)
			{
				TiXmlElement keyBinder( "KeyBinder" );

				keyBinder.SetAttribute( "key", keyCodeToString(
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

			if(getMouseButtonBinding(*o, Control/*::ControlChangingDirection*/::INCREASE) 
				!= ICS_MAX_DEVICE_BUTTONS)
			{
				TiXmlElement binder( "MouseButtonBinder" );

				unsigned int button = getMouseButtonBinding(*o, Control/*::ControlChangingDirection*/::INCREASE);
				if(button == OIS::/*MouseButtonID::*/MB_Left)
				{
					binder.SetAttribute( "button", "LEFT" );
				}
				else if(button == OIS::/*MouseButtonID::*/MB_Middle)
				{
					binder.SetAttribute( "button", "MIDDLE" );
				}
				else if(button == OIS::/*MouseButtonID::*/MB_Right)
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
				if(button == OIS::/*MouseButtonID::*/MB_Left)
				{
					binder.SetAttribute( "button", "LEFT" );
				}
				else if(button == OIS::/*MouseButtonID::*/MB_Middle)
				{
					binder.SetAttribute( "button", "MIDDLE" );
				}
				else if(button == OIS::/*MouseButtonID::*/MB_Right)
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
			while(it != mJoystickIDList.end())
			{
				int deviceId = *it;

				if(getJoystickAxisBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE) 
					!= /*NamedAxis::*/UNASSIGNED)
				{
					TiXmlElement binder( "JoystickAxisBinder" );

					binder.SetAttribute( "axis", ToString<int>(
						getJoystickAxisBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );				

					binder.SetAttribute( "direction", "INCREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );
					
					control.InsertEndChild(binder);
				}

				if(getJoystickAxisBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE) 
					!= /*NamedAxis::*/UNASSIGNED)
				{
					TiXmlElement binder( "JoystickAxisBinder" );

					binder.SetAttribute( "axis", ToString<int>(
						getJoystickAxisBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE)).c_str() );				

					binder.SetAttribute( "direction", "DECREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );
					
					control.InsertEndChild(binder);
				}

				if(getJoystickButtonBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE) 
					!= ICS_MAX_DEVICE_BUTTONS)
				{
					TiXmlElement binder( "JoystickButtonBinder" );

					binder.SetAttribute( "button", ToString<unsigned int>(
						getJoystickButtonBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );				

					binder.SetAttribute( "direction", "INCREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );
					
					control.InsertEndChild(binder);
				}

				if(getJoystickButtonBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE) 
					!= ICS_MAX_DEVICE_BUTTONS)
				{
					TiXmlElement binder( "JoystickButtonBinder" );

					binder.SetAttribute( "button", ToString<unsigned int>(
						getJoystickButtonBinding(*o, *it, Control/*::ControlChangingDirection*/::DECREASE)).c_str() );				

					binder.SetAttribute( "direction", "DECREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );
					
					control.InsertEndChild(binder);
				}

				if(getJoystickPOVBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE).index >= 0)
				{
					TiXmlElement binder( "JoystickPOVBinder" );

					POVBindingPair POVPair = getJoystickPOVBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE);
					
					binder.SetAttribute( "pov", ToString<int>(POVPair.index).c_str() );

					binder.SetAttribute( "direction", "INCREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );

					if(POVPair.axis == ICS::InputControlSystem::EastWest)
					{
						binder.SetAttribute( "axis", "EastWest" );
					}
					else
					{
						binder.SetAttribute( "axis", "NorthSouth" );
					}
					
					control.InsertEndChild(binder);
				}

				if(getJoystickPOVBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE).index >= 0)
				{
					TiXmlElement binder( "JoystickPOVBinder" );

					POVBindingPair POVPair = getJoystickPOVBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE);
					
					binder.SetAttribute( "pov", ToString<int>(POVPair.index).c_str() );

					binder.SetAttribute( "direction", "DECREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );

					if(POVPair.axis == ICS::InputControlSystem::EastWest)
					{
						binder.SetAttribute( "axis", "EastWest" );
					}
					else
					{
						binder.SetAttribute( "axis", "NorthSouth" );
					}
					
					control.InsertEndChild(binder);
				}

				if(getJoystickSliderBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE) 
					!= /*NamedAxis::*/UNASSIGNED)
				{
					TiXmlElement binder( "JoystickSliderBinder" );

					binder.SetAttribute( "slider", ToString<int>(
						getJoystickSliderBinding(*o, deviceId, Control/*::ControlChangingDirection*/::INCREASE)).c_str() );				

					binder.SetAttribute( "direction", "INCREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );
					
					control.InsertEndChild(binder);
				}

				if(getJoystickSliderBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE) 
					!= /*NamedAxis::*/UNASSIGNED)
				{
					TiXmlElement binder( "JoystickSliderBinder" );

					binder.SetAttribute( "slider", ToString<int>(
						getJoystickSliderBinding(*o, deviceId, Control/*::ControlChangingDirection*/::DECREASE)).c_str() );				

					binder.SetAttribute( "direction", "DECREASE" );

					binder.SetAttribute( "deviceId", ToString<int>(deviceId).c_str() );
					
					control.InsertEndChild(binder);
				}

				it++;
			}


			std::list<Channel*> channels = (*o)->getAttachedChannels();
			for(std::list<Channel*>::iterator it = channels.begin() ;
				it != channels.end() ; it++)
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

	void InputControlSystem::addJoystick(int deviceId)
	{
		ICS_LOG("Adding joystick (device id: " + ToString<int>(deviceId) + ")");

		for(int j = 0 ; j < ICS_MAX_JOYSTICK_AXIS ; j++)
		{
			if(mControlsJoystickAxisBinderMap[deviceId].find(j) == mControlsJoystickAxisBinderMap[deviceId].end())
			{
				ControlAxisBinderItem controlJoystickBinderItem;
				controlJoystickBinderItem.direction = Control::STOP;
				controlJoystickBinderItem.control = NULL;
				mControlsJoystickAxisBinderMap[deviceId][j] = controlJoystickBinderItem;
			}
		}

		mJoystickIDList.push_back(deviceId);
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

	void InputControlSystem::cancelDetectingBindingState()
	{
		mDetectingBindingControl = NULL;
	}

	void InputControlSystem::fillOISKeysMap()
	{
		mKeys["UNASSIGNED"]= OIS::KC_UNASSIGNED;
		mKeys["ESCAPE"]= OIS::KC_ESCAPE;
		mKeys["1"]= OIS::KC_1;
		mKeys["2"]= OIS::KC_2;
		mKeys["3"]= OIS::KC_3;
		mKeys["4"]= OIS::KC_4;
		mKeys["5"]= OIS::KC_5;
		mKeys["6"]= OIS::KC_6;
		mKeys["7"]= OIS::KC_7;
		mKeys["8"]= OIS::KC_8;
		mKeys["9"]= OIS::KC_9;
		mKeys["0"]= OIS::KC_0;
		mKeys["MINUS"]= OIS::KC_MINUS;
		mKeys["EQUALS"]= OIS::KC_EQUALS;
		mKeys["BACK"]= OIS::KC_BACK;
		mKeys["TAB"]= OIS::KC_TAB;
		mKeys["Q"]= OIS::KC_Q;
		mKeys["W"]= OIS::KC_W;
		mKeys["E"]= OIS::KC_E;
		mKeys["R"]= OIS::KC_R;
		mKeys["T"]= OIS::KC_T;
		mKeys["Y"]= OIS::KC_Y;
		mKeys["U"]= OIS::KC_U;
		mKeys["I"]= OIS::KC_I;
		mKeys["O"]= OIS::KC_O;
		mKeys["P"]= OIS::KC_P;
		mKeys["LBRACKET"]= OIS::KC_LBRACKET;
		mKeys["RBRACKET"]= OIS::KC_RBRACKET;
		mKeys["RETURN"]= OIS::KC_RETURN;
		mKeys["LCONTROL"]= OIS::KC_LCONTROL;
		mKeys["A"]= OIS::KC_A;
		mKeys["S"]= OIS::KC_S;
		mKeys["D"]= OIS::KC_D;
		mKeys["F"]= OIS::KC_F;
		mKeys["G"]= OIS::KC_G;
		mKeys["H"]= OIS::KC_H;
		mKeys["J"]= OIS::KC_J;
		mKeys["K"]= OIS::KC_K;
		mKeys["L"]= OIS::KC_L;
		mKeys["SEMICOLON"]= OIS::KC_SEMICOLON;
		mKeys["APOSTROPHE"]= OIS::KC_APOSTROPHE;
		mKeys["GRAVE"]= OIS::KC_GRAVE;
		mKeys["LSHIFT"]= OIS::KC_LSHIFT;
		mKeys["BACKSLASH"]= OIS::KC_BACKSLASH;
		mKeys["Z"]= OIS::KC_Z;
		mKeys["X"]= OIS::KC_X;
		mKeys["C"]= OIS::KC_C;
		mKeys["V"]= OIS::KC_V;
		mKeys["B"]= OIS::KC_B;
		mKeys["N"]= OIS::KC_N;
		mKeys["M"]= OIS::KC_M;
		mKeys["COMMA"]= OIS::KC_COMMA;
		mKeys["PERIOD"]= OIS::KC_PERIOD;
		mKeys["SLASH"]= OIS::KC_SLASH;
		mKeys["RSHIFT"]= OIS::KC_RSHIFT;
		mKeys["MULTIPLY"]= OIS::KC_MULTIPLY;
		mKeys["LMENU"]= OIS::KC_LMENU;
		mKeys["SPACE"]= OIS::KC_SPACE;
		mKeys["CAPITAL"]= OIS::KC_CAPITAL;
		mKeys["F1"]= OIS::KC_F1;
		mKeys["F2"]= OIS::KC_F2;
		mKeys["F3"]= OIS::KC_F3;
		mKeys["F4"]= OIS::KC_F4;
		mKeys["F5"]= OIS::KC_F5;
		mKeys["F6"]= OIS::KC_F6;
		mKeys["F7"]= OIS::KC_F7;
		mKeys["F8"]= OIS::KC_F8;
		mKeys["F9"]= OIS::KC_F9;
		mKeys["F10"]= OIS::KC_F10;
		mKeys["F11"]= OIS::KC_F11;
		mKeys["F12"]= OIS::KC_F12;
		mKeys["NUMLOCK"]= OIS::KC_NUMLOCK;
		mKeys["SCROLL"]= OIS::KC_SCROLL;
		mKeys["NUMPAD7"]= OIS::KC_NUMPAD7;
		mKeys["NUMPAD8"]= OIS::KC_NUMPAD8;
		mKeys["NUMPAD9"]= OIS::KC_NUMPAD9;
		mKeys["SUBTRACT"]= OIS::KC_SUBTRACT;
		mKeys["NUMPAD4"]= OIS::KC_NUMPAD4;
		mKeys["NUMPAD5"]= OIS::KC_NUMPAD5;
		mKeys["NUMPAD6"]= OIS::KC_NUMPAD6;
		mKeys["ADD"]= OIS::KC_ADD;
		mKeys["NUMPAD1"]= OIS::KC_NUMPAD1;
		mKeys["NUMPAD2"]= OIS::KC_NUMPAD2;
		mKeys["NUMPAD3"]= OIS::KC_NUMPAD3;
		mKeys["NUMPAD0"]= OIS::KC_NUMPAD0;
		mKeys["DECIMAL"]= OIS::KC_DECIMAL;
		mKeys["RCONTROL"]= OIS::KC_RCONTROL;
		mKeys["DIVIDE"]= OIS::KC_DIVIDE;
		mKeys["SYSRQ"]= OIS::KC_SYSRQ;
		mKeys["RMENU"]= OIS::KC_RMENU;
		mKeys["PAUSE"]= OIS::KC_PAUSE;
		mKeys["HOME"]= OIS::KC_HOME;
		mKeys["UP"]= OIS::KC_UP;
		mKeys["PGUP"]= OIS::KC_PGUP;
		mKeys["LEFT"]= OIS::KC_LEFT;
		mKeys["RIGHT"]= OIS::KC_RIGHT;
		mKeys["END"]= OIS::KC_END;
		mKeys["DOWN"]= OIS::KC_DOWN;
		mKeys["PGDOWN"]= OIS::KC_PGDOWN;
		mKeys["INSERT"]= OIS::KC_INSERT;
		mKeys["DELETE"]= OIS::KC_DELETE;
		mKeys["LWIN"]= OIS::KC_LWIN;
		mKeys["RWIN"]= OIS::KC_RWIN;
		mKeys["APPS"]= OIS::KC_APPS;

		mKeys["NUMPADENTER"]= OIS::KC_NUMPADENTER;

		for(std::map<std::string, OIS::KeyCode>::iterator it = mKeys.begin()
			; it != mKeys.end() ; it++)
		{
			mKeyCodes[ it->second ] = it->first;
		}
	}

	std::string InputControlSystem::keyCodeToString(OIS::KeyCode key)
	{
		return mKeyCodes[key];
	}

	OIS::KeyCode InputControlSystem::stringToKeyCode(std::string key)
	{
		return mKeys[key];
	}
}