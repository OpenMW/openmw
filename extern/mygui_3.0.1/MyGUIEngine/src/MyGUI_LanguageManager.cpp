/*!
	@file
	@author		Albert Semenov
	@date		09/2008
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_LanguageManager.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_DataManager.h"
#include "MyGUI_FactoryManager.h"

namespace MyGUI
{

	const std::string XML_TYPE("Language");

	MYGUI_INSTANCE_IMPLEMENT( LanguageManager )

	void LanguageManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &LanguageManager::_load);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void LanguageManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().unregisterLoadXmlDelegate(XML_TYPE);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	bool LanguageManager::load(const std::string& _file)
	{
		return ResourceManager::getInstance()._loadImplement(_file, true, XML_TYPE, INSTANCE_TYPE_NAME);
	}

	void LanguageManager::_load(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
		std::string default_lang;
		bool event_change = false;

		// берем детей и крутимся, основной цикл
		xml::ElementEnumerator root = _node->getElementEnumerator();
		while (root.next(XML_TYPE))
		{
			// парсим атрибуты
			root->findAttribute("default", default_lang);

			// берем детей и крутимся
			xml::ElementEnumerator info = root->getElementEnumerator();
			while (info.next("Info"))
			{
				// парсим атрибуты
				std::string name(info->findAttribute("name"));

				// доюавляем в карту пользователя
				if (name.empty())
				{
					xml::ElementEnumerator source_info = info->getElementEnumerator();
					while (source_info.next("Source"))
					{
						loadLanguage(source_info->getContent(), true);
					}
				}
				// добавляем в карту языков
				else
				{
					xml::ElementEnumerator source_info = info->getElementEnumerator();
					while (source_info.next("Source"))
					{
						std::string file_source = source_info->getContent();
						// добавляем в карту
						mMapFile[name].push_back(file_source);

						// если добавляемый файл для текущего языка, то подгружаем и оповещаем
						if (name == mCurrentLanguageName)
						{
							loadLanguage(file_source, false);
							event_change = true;
						}
					}
				}

			}
		}

		if (!default_lang.empty())
			setCurrentLanguage(default_lang);
		else if (event_change)
			eventChangeLanguage(mCurrentLanguageName);
	}

	void LanguageManager::setCurrentLanguage(const std::string& _name)
	{
		mMapLanguage.clear();

		mCurrentLanguageName = _name;

		MapListString::iterator item = mMapFile.find(_name);
		if (item == mMapFile.end())
		{
			MYGUI_LOG(Error, "Language '" << _name << "' is not found");
			return;
		}

		for (VectorString::const_iterator iter=item->second.begin(); iter!=item->second.end(); ++iter)
		{
			loadLanguage(*iter, false);
		}

		eventChangeLanguage(mCurrentLanguageName);
	}

	bool LanguageManager::loadLanguage(const std::string& _file, bool _user)
	{
		IDataStream* data = DataManager::getInstance().getData(_file);
		if (data == nullptr)
		{
			MYGUI_LOG(Error, "file '" << _file << "' not found");
			return false;
		}

		if (_file.find(".xml") != std::string::npos)
			_loadLanguageXML(data, _user);
		else
			_loadLanguage(data, _user);

		delete data;
		return true;
	}

	void LanguageManager::_loadLanguageXML(IDataStream* _stream, bool _user)
	{
		xml::Document doc;
		// формат xml
		if (doc.open(_stream))
		{
			xml::ElementPtr root = doc.getRoot();
			if (root)
			{
				xml::ElementEnumerator tag = root->getElementEnumerator();
				while (tag.next("Tag"))
				{
					if (_user)
						mUserMapLanguage[tag->findAttribute("name")] = tag->getContent();
					else
						mMapLanguage[tag->findAttribute("name")] = tag->getContent();
				}
			}
		}
	}

	void LanguageManager::_loadLanguage(IDataStream* _stream, bool _user)
	{
		// формат txt
		std::string read;
		while (!_stream->eof())
		{
			_stream->readline(read, '\n');
			if (read.empty()) continue;

			// заголовок утф
			if ((uint8)read[0] == 0xEF && read.size() > 2)
			{
				read.erase(0, 3);
			}

			if (read[read.size()-1] == '\r') read.erase(read.size()-1, 1);
			if (read.empty()) continue;

			size_t pos = read.find_first_of(" \t");
			if (_user)
			{
				if (pos == std::string::npos) mUserMapLanguage[read] = "";
				else mUserMapLanguage[read.substr(0, pos)] = read.substr(pos+1, std::string::npos);
			}
			else
			{
				if (pos == std::string::npos) mMapLanguage[read] = "";
				else mMapLanguage[read.substr(0, pos)] = read.substr(pos+1, std::string::npos);
			}
		}
	}

	UString LanguageManager::replaceTags(const UString& _line)
	{
		// вот хз, что быстрее, итераторы или математика указателей,
		// для непонятно какого размера одного символа UTF8
		UString line(_line);

		if (mMapLanguage.empty() && mUserMapLanguage.empty())
			return _line;

		UString::iterator end = line.end();
		for (UString::iterator iter=line.begin(); iter!=end; )
		{
			if (*iter == '#')
			{
				++iter;
				if (iter == end)
				{
					return line;
				}
				else
				{
					if (*iter != '{')
					{
						++iter;
						continue;
					}
					UString::iterator iter2 = iter;
					++iter2;
					while (true)
					{
						if (iter2 == end) return line;
						if (*iter2 == '}')
						{
							size_t start = iter - line.begin();
							size_t len = (iter2 - line.begin()) - start - 1;
							const UString& tag = line.substr(start + 1, len);

							bool find = true;
							MapLanguageString::iterator replace = mMapLanguage.find(tag);
							if (replace == mMapLanguage.end())
							{
								replace = mUserMapLanguage.find(tag);
								find = replace != mUserMapLanguage.end();
							}

							if (!find)
							{
								iter = line.insert(iter, '#') + size_t(len + 2);
								end = line.end();
								break;
							}

							iter = line.erase(iter - size_t(1), iter2 + size_t(1));
							size_t pos = iter - line.begin();
							line.insert(pos, replace->second);
							iter = line.begin() + pos + replace->second.length();
							end = line.end();
							if (iter == end) return line;
							break;

						}
						++iter2;
					}
				}
			}
			else
			{
				++iter;
			}
		}

		return line;
	}

	UString LanguageManager::getTag(const UString& _tag)
	{
		MapLanguageString::iterator iter = mMapLanguage.find(_tag);
		if (iter == mMapLanguage.end())
		{
			iter = mUserMapLanguage.find(_tag);
			if (iter != mUserMapLanguage.end()) return iter->second;
			return _tag;
		}

		return iter->second;
	}

	const std::string& LanguageManager::getCurrentLanguage()
	{
		return mCurrentLanguageName;
	}

	void LanguageManager::addUserTag(const UString& _tag, const UString& _replace)
	{
		mUserMapLanguage[_tag] = _replace;
	}

	void LanguageManager::clearUserTags()
	{
		mUserMapLanguage.clear();
	}

	bool LanguageManager::loadUserTags(const std::string& _file)
	{
		return loadLanguage(_file, true);
	}

} // namespace MyGUI
