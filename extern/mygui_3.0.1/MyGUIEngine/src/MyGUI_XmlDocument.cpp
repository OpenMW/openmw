/*!
	@file
	@author		Albert Semenov
	@date		11/2007
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
#include "MyGUI_XmlDocument.h"
#include "MyGUI_DataManager.h"

namespace MyGUI
{
	namespace xml
	{

		namespace utility
		{
			std::string convert_from_xml(const std::string& _string, bool& _ok)
			{
				std::string ret;
				_ok = true;

				size_t pos = _string.find("&");
				if (pos == std::string::npos) return _string;

				ret.reserve(_string.size());
				size_t old = 0;
				while (pos != std::string::npos)
				{
					ret += _string.substr(old, pos - old);

					size_t end = _string.find(";", pos + 1);
					if (end == std::string::npos)
					{
						_ok = false;
						return ret;
					}
					else
					{
						std::string tag = _string.substr(pos, end - pos + 1);
						if (tag == "&amp;") ret += '&';
						else if (tag == "&lt;") ret += '<';
						else if (tag == "&gt;") ret += '>';
						else if (tag == "&apos;") ret += '\'';
						else if (tag == "&quot;") ret += '\"';
						else
						{
							_ok = false;
							return ret;
						}
					}

					old = end + 1;
					pos = _string.find("&", old);
				}
				ret += _string.substr(old, std::string::npos);

				return ret;
			}

			std::string convert_to_xml(const std::string& _string)
			{
				std::string ret;

				size_t pos = _string.find_first_of("&<>'\"");
				if (pos == std::string::npos) return _string;

				ret.reserve(_string.size() * 2);
				size_t old = 0;
				while (pos != std::string::npos)
				{
					ret += _string.substr(old, pos - old);

					if (_string[pos] == '&') ret += "&amp;";
					else if (_string[pos] == '<') ret += "&lt;";
					else if (_string[pos] == '>') ret += "&gt;";
					else if (_string[pos] == '\'') ret += "&apos;";
					else if (_string[pos] == '\"') ret += "&quot;";

					old = pos + 1;
					pos = _string.find_first_of("&<>'\"", old);
				}
				ret += _string.substr(old, std::string::npos);

				return ret;
			}

		}

		//----------------------------------------------------------------------//
		// class ElementEnumerator
		//----------------------------------------------------------------------//
		ElementEnumerator::ElementEnumerator(VectorElement::iterator _begin, VectorElement::iterator _end) :
			m_first(true),
			m_current(_begin),
			m_end(_end)
		{
		}

		bool ElementEnumerator::next()
		{
			if (m_current == m_end) return false;
			else if (m_first)
			{
				m_first = false;
				return true;
			}
			++ m_current;
			if (m_current == m_end) return false;
			return true;
		}

		bool ElementEnumerator::next(const std::string& _name)
		{
			while (next())
			{
				if ((*m_current)->getName() == _name) return true;
			}
			return false;
		}

		//----------------------------------------------------------------------//
		// class Element
		//----------------------------------------------------------------------//
		Element::Element(const std::string &_name, ElementPtr _parent, ElementType _type, const std::string& _content) :
			mName(_name),
			mContent(_content),
			mParent(_parent),
			mType(_type)
		{
		}

		Element::~Element()
		{
			for (VectorElement::iterator iter=mChilds.begin(); iter!=mChilds.end(); ++iter)
			{
				delete *iter;
			}
			mChilds.clear();
		}

		void Element::save(std::ostream& _stream, size_t _level)
		{
			// сначала табуляции намутим
			for (size_t tab=0; tab<_level; ++tab) _stream  << "    ";

			// теперь заголовок тега
			if (mType == ElementType::Declaration) _stream << "<?";
			else _stream << "<";
			_stream << mName;

			for (VectorAttributes::iterator iter = mAttributes.begin(); iter != mAttributes.end(); ++iter)
			{
				_stream << " " << iter->first << "=\"" << utility::convert_to_xml(iter->second) << "\"";
			}

			bool empty = mChilds.empty();
			// если детей нет то закрываем
			if (empty && mContent.empty())
			{
				if (mType == ElementType::Declaration) _stream << "?>\n";
				else _stream << "/>\n";
			}
			else
			{
				_stream << ">";
				if (!empty) _stream << "\n";
				// если есть тело то сначало оно
				if (!mContent.empty())
				{
					if (!empty)
					{
						for (size_t tab=0; tab<=_level; ++tab) _stream  << "    ";
					}
					_stream << utility::convert_to_xml(mContent);

					if (!empty) _stream << "\n";
				}
				// если есть детишки путь сохранятся
				for (size_t child=0; child<mChilds.size(); child++)
				{
					mChilds[child]->save(_stream, _level + 1);
				}

				if (!empty)
				{
					for (size_t tab=0; tab<_level; ++tab)
						_stream  << "    ";
				}
				_stream << "</" << mName << ">\n";
			}

		}

		ElementPtr Element::createChild(const std::string& _name, const std::string& _content)
		{
			ElementPtr node = new Element(_name, this, ElementType::Normal, _content);
			mChilds.push_back(node);
			return node;
		}

		void Element::clear()
		{
			for (VectorElement::iterator iter = mChilds.begin(); iter != mChilds.end(); ++iter) delete *iter;
			mChilds.clear();
			mContent.clear();
			mAttributes.clear();
		}

		bool Element::findAttribute(const std::string& _name, std::string& _value)
		{
			for (VectorAttributes::iterator iter=mAttributes.begin(); iter!=mAttributes.end(); ++iter)
			{
				if ( (*iter).first == _name)
				{
					_value = (*iter).second;
					return true;
				}
			}
			return false;
		}

		std::string Element::findAttribute(const std::string& _name)
		{
			for (VectorAttributes::iterator iter=mAttributes.begin(); iter!=mAttributes.end(); ++iter)
			{
				if ( (*iter).first == _name) return (*iter).second;
			}
			return "";
		}

		void Element::addAttribute(const std::string& _key, const std::string& _value)
		{
			mAttributes.push_back(PairAttribute(_key, _value));
		}

		void Element::removeAttribute(const std::string& _key)
		{
			for (size_t index=0; index<mAttributes.size(); ++index)
			{
				if (mAttributes[index].first == _key)
				{
					mAttributes.erase(mAttributes.begin() + index);
					return;
				}
			}
		}

		ElementPtr Element::createCopy()
		{
			Element* elem = new Element(mName, nullptr, mType, mContent);
			elem->mAttributes = mAttributes;

			for (VectorElement::iterator iter=mChilds.begin(); iter!=mChilds.end(); ++iter)
			{
				Element* child = (*iter)->createCopy();
				child->mParent = elem;
				elem->mChilds.push_back(child);
			}

			return elem;
		}

		void Element::setAttribute(const std::string& _key, const std::string& _value)
		{
			for (size_t index=0; index<mAttributes.size(); ++index)
			{
				if (mAttributes[index].first == _key)
				{
					mAttributes[index].second = _value;
					return;
				}
			}
			mAttributes.push_back(PairAttribute(_key, _value));
		}

		void Element::addContent(const std::string& _content)
		{
			if (mContent.empty()) mContent = _content;
			else
			{
				mContent += " ";
				mContent += _content;
			}
		}

#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC && !defined(STLPORT)
		inline void open_stream(std::ofstream& _stream, const std::wstring& _wide) { _stream.open(_wide.c_str()); }
		inline void open_stream(std::ifstream& _stream, const std::wstring& _wide) { _stream.open(_wide.c_str()); }
#else
		inline void open_stream(std::ofstream& _stream, const std::wstring& _wide) { _stream.open(UString(_wide).asUTF8_c_str()); }
		inline void open_stream(std::ifstream& _stream, const std::wstring& _wide) { _stream.open(UString(_wide).asUTF8_c_str()); }
#endif

		//----------------------------------------------------------------------//
		// class Document
		//----------------------------------------------------------------------//
		Document::Document():
			mRoot(0),
			mDeclaration(0),
			mLastErrorFile(""),
			mLine(0),
			mCol(0)
		{
		}

		Document::~Document()
		{
			clear();
		}

		// открывает обычным файлом, имя файла в utf8
		bool Document::open(const std::string& _filename)
		{
			std::ifstream stream;
			stream.open(_filename.c_str());

			if (!stream.is_open())
			{
				mLastError = ErrorType::OpenFileFail;
				setLastFileError(_filename);
				return false;
			}

			bool result = open(stream);

			stream.close();
			return result;
		}

		// открывает обычным файлом, имя файла в utf16 или utf32
		bool Document::open(const std::wstring& _filename)
		{
			std::ifstream stream;
			open_stream(stream, _filename);

			if (!stream.is_open())
			{
				mLastError = ErrorType::OpenFileFail;
				setLastFileError(_filename);
				return false;
			}

			bool result = open(stream);

			stream.close();
			return result;
		}

		bool Document::open(std::istream& _stream)
		{
			DataStream* data = new DataStream(&_stream);

			bool result = open(data);
			delete data;

			return result;
		}

		// сохраняет файл, имя файла в кодировке utf8
		bool Document::save(const std::string& _filename)
		{
			std::ofstream stream;
			stream.open(_filename.c_str());

			if (!stream.is_open())
			{
				mLastError = ErrorType::CreateFileFail;
				setLastFileError(_filename);
				return false;
			}

			bool result = save(stream);

			if (!result)
			{
				setLastFileError(_filename);
			}

			stream.close();
			return result;
		}

		// сохраняет файл, имя файла в кодировке utf16 или utf32
		bool Document::save(const std::wstring& _filename)
		{
			std::ofstream stream;
			open_stream(stream, _filename);

			if (!stream.is_open())
			{
				mLastError = ErrorType::CreateFileFail;
				setLastFileError(_filename);
				return false;
			}

			bool result = save(stream);

			if (!result)
			{
				setLastFileError(_filename);
			}

			stream.close();
			return result;
		}

		// открывает обычным потоком
		bool Document::open(IDataStream* _stream)
		{
			clear();

			// это текущая строка для разбора
			std::string line;
			// это строка из файла
			std::string read;
			// текущий узел для разбора
			ElementPtr currentNode = 0;

			while (!_stream->eof())
			{
				// берем новую строку
				_stream->readline(read, '\n');
				if (read.empty()) continue;
				if (read[read.size()-1] == '\r') read.erase(read.size()-1, 1);
				if (read.empty()) continue;

				mLine ++;
				mCol = 0; // потом проверить на многострочных тэгах
				if (read.empty()) continue;
				// текущая строка для разбора и то что еще прочитали
				line += read;

				if (!parseLine(line, currentNode))
				{
					return false;
				}

			} // while (!stream.eof())

			if (currentNode)
			{
				mLastError = ErrorType::NotClosedElements;
				return false;
			}

			return true;
		}

		bool Document::save(std::ostream& _stream)
		{
			if (!mDeclaration)
			{
				mLastError = ErrorType::NoXMLDeclaration;
				return false;
			}

			// заголовок utf8
			_stream << (char)0xEFu;
			_stream << (char)0xBBu;
			_stream << (char)0xBFu;

			mDeclaration->save(_stream, 0);
			if (mRoot) mRoot->save(_stream, 0);

			return true;
		}

		void Document::clear()
		{
			clearDeclaration();
			clearRoot();
			mLine = 0;
			mCol = 0;
		}

		bool Document::parseTag(ElementPtr &_currentNode, std::string _content)
		{

			// убераем лишнее
			MyGUI::utility::trim(_content);

			if (_content.empty())
			{
				// создаем пустой тег
				if (_currentNode) _currentNode = _currentNode->createChild("");
				else
				{
					_currentNode = new Element("", 0);
					// если это первый то запоминаем
					if (!mRoot) mRoot = _currentNode;
				}
				return true;
			}

			char simbol = _content[0];
			bool tag_info = false;

			if (simbol == '!') return true; // проверяем на коментарии

			// проверяем на информационный тег
			if (simbol == '?')
			{
				tag_info = true;
				_content.erase(0, 1); // удаляем первый символ
			}

			size_t start, end;
			// проверяем на закрытие тега
			if (simbol == '/')
			{
				if (_currentNode == 0)
				{
					// чета мы закрывам а ниче даже и не открыто
					if (!mRoot)
					{
						mLastError = ErrorType::CloseNotOpenedElement;
						return false;
					}
				}
				// обрезаем имя тэга
				start = _content.find_first_not_of(" \t", 1);
				if (start == _content.npos)
				{
					// тег пустой
					_content.clear();
				}
				else
				{
					end = _content.find_last_not_of(" \t");
					_content = _content.substr(start, end - start+1);
				}
				// проверяем соответствие открывающего и закрывающего тегов
				if (_currentNode->getName() != _content)
				{
					mLastError = ErrorType::InconsistentOpenCloseElements;
					return false;
				}
				// а теперь снижаем текущий узел вниз
				_currentNode = _currentNode->getParent();
			}
			else
			{
				// выделяем имя до первого пробела или закрывающего тега
				std::string cut = _content;
				start = _content.find_first_of(" \t/?", 1); // << превед
				if (start != _content.npos)
				{
					cut = _content.substr(0, start);
					_content = _content.substr(start);
				}
				else
				{
					_content.clear();
				}

				if (_currentNode) _currentNode = _currentNode->createChild(cut);
				else
				{
					if (tag_info)
					{
						// информационный тег
						if (mDeclaration)
						{
							mLastError = ErrorType::MoreThanOneXMLDeclaration;
							return false;
						}
						_currentNode = new Element(cut, 0, ElementType::Comment);
						mDeclaration = _currentNode;
					}
					else
					{
						// рутовый тег
						if (mRoot)
						{
							mLastError = ErrorType::MoreThanOneRootElement;
							return false;
						}
						_currentNode = new Element(cut, 0, ElementType::Normal);
						mRoot = _currentNode;
					}
				}

				// проверим на пустоту
				start = _content.find_last_not_of(" \t");
				if (start == _content.npos) return true;

				// сразу отделим закрывающийся тэг
				bool close = false;
				if ((_content[start] == '/') || (_content[start] == '?'))
				{
					close = true;
					// не будем резать строку, просто поставим пробел
					_content[start] = ' ';
					// проверим на пустоту
					start = _content.find_last_not_of(" \t");
					if (start == _content.npos)
					{
						// возвращаем все назад и уходим
						_currentNode = _currentNode->getParent();
						return true;
					}
				}

				// а вот здесь уже в цикле разбиваем на атрибуты
				while (true)
				{
					// ищем равно
					start = _content.find('=');
					if (start == _content.npos)
					{
						mLastError = ErrorType::IncorrectAttribute;
						return false;
					}
					// ищем вторые ковычки
					end = _content.find_first_of("\"\'", start+1);
					if (end == _content.npos)
					{
						mLastError = ErrorType::IncorrectAttribute;
						return false;
					}
					end = _content.find_first_of("\"\'", end+1);
					if (end == _content.npos)
					{
						mLastError = ErrorType::IncorrectAttribute;
						return false;
					}

					std::string key = _content.substr(0, start);
					std::string value = _content.substr(start+1, end-start);

					// проверка на валидность
					if (! checkPair(key, value))
					{
						mLastError = ErrorType::IncorrectAttribute;
						return false;
					}

					// добавляем пару в узел
					_currentNode->addAttribute(key, value);

					// следующий кусок
					_content = _content.substr(end+1);

					// в строке не осталось символов
					start = _content.find_first_not_of(" \t");
					if (start == _content.npos) break;

					mCol += start;
				}

				// был закрывающий тег для текущего тега
				if (close)
				{
					// не проверяем имена, потому что это наш тэг
					_currentNode = _currentNode->getParent();
				}

			}
			return true;
		}

		bool Document::checkPair(std::string &_key, std::string &_value)
		{
			// в ключе не должно быть ковычек и пробелов
			MyGUI::utility::trim(_key);
			if (_key.empty()) return false;
			size_t start = _key.find_first_of(" \t\"\'&");
			if (start != _key.npos) return false;

			// в значении, ковычки по бокам
			MyGUI::utility::trim(_value);
			if (_value.size() < 2) return false;
			if (((_value[0] != '"') || (_value[_value.length()-1] != '"')) &&
				((_value[0] != '\'') || (_value[_value.length()-1] != '\''))) return false;
			bool ok = true;
			_value = utility::convert_from_xml(_value.substr(1, _value.length() - 2), ok);
			return ok;
		}

		// ищет символ без учета ковычек
		size_t Document::find(const std::string& _text, char _char, size_t _start)
		{
			// ковычки
			bool kov = false;

			// буфер для поиска
			char buff[16] = "\"_\0";
			buff[1] = _char;

			size_t pos = _start;

			while (true)
			{
				pos = _text.find_first_of(buff, pos);

				// если уже конец, то досвидания
				if (pos == _text.npos) break;

				// нашли ковычку
				else if (_text[pos] == '"')
				{
					kov = !kov;
					pos ++;
				}
				// если мы в ковычках, то идем дальше
				else if (kov) pos ++;

				// мы не в ковычках
				else break;

			}

			return pos;
		}

		void Document::clearDeclaration()
		{
			if (mDeclaration)
			{
				delete mDeclaration;
				mDeclaration = 0;
			}
		}

		void Document::clearRoot()
		{
			if (mRoot)
			{
				delete mRoot;
				mRoot = 0;
			}
		}

		ElementPtr Document::createDeclaration(const std::string& _version, const std::string& _encoding)
		{
			clearDeclaration();
			mDeclaration = new Element("xml", 0, ElementType::Declaration);
			mDeclaration->addAttribute("version", _version);
			mDeclaration->addAttribute("encoding", _encoding);
			return mDeclaration;
		}

		ElementPtr Document::createRoot(const std::string& _name)
		{
			clearRoot();
			mRoot = new Element(_name, 0, ElementType::Normal);
			return mRoot;
		}

		bool Document::parseLine(std::string& _line, ElementPtr& _element)
		{
			// крутимся пока в строке есть теги
			while (true)
			{
				// сначала ищем по угловым скобкам
				size_t start = find(_line, '<');
				if (start == _line.npos) break;
				size_t end = _line.npos;

				// пытаемся вырезать многострочный коментарий
				if ((start + 3 < _line.size()) && (_line[start + 1] == '!') && (_line[start + 2] == '-') && (_line[start + 3] == '-'))
				{
					end = _line.find("-->", start + 4);
					if (end == _line.npos) break;
					end += 2;
				}
				else
				{
					end = find(_line, '>', start+1);
					if (end == _line.npos) break;
				}
				// проверяем на наличее тела
				size_t body = _line.find_first_not_of(" \t<");
				if (body < start)
				{
					std::string body_str = _line.substr(0, start);
					// текущий символ
					mCol = 0;

					if (_element != 0)
					{
						bool ok = true;
						_element->setContent(utility::convert_from_xml(body_str, ok));
						if (!ok)
						{
							mLastError = ErrorType::IncorrectContent;
							return false;
						}
					}
				}
				// вырезаем наш тэг и парсим
				if (!parseTag(_element, _line.substr(start+1, end-start-1)))
				{
					return false;
				}
				// и обрезаем текущую строку разбора
				_line = _line.substr(end+1);
			}
			return true;
		}

		std::string Document::getLastError()
		{
			const std::string& error = mLastError.print();
			if (error.empty()) return error;
			return MyGUI::utility::toString("'", error, "' ,  file='", mLastErrorFile, "' ,  line=", mLine, " ,  col=", mCol);
		}

		/*Document Document::createCopyFromElement(ElementPtr _node)
		{
			Document doc;
			doc.mRoot = _node->createCopy();
			return doc;
		}*/

	} // namespace xml

} // namespace MyGUI
