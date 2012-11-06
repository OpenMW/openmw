#include "ScriptLoader.hpp"

#include <vector>
#include <map>
#include <exception>
#include <fstream>

#include <boost/filesystem.hpp>

namespace sh
{
	void ScriptLoader::loadAllFiles(ScriptLoader* c, const std::string& path)
	{
		for ( boost::filesystem::recursive_directory_iterator end, dir(path); dir != end; ++dir )
		{
			boost::filesystem::path p(*dir);
			if(p.extension() == c->m_fileEnding)
			{
				c->m_currentFileName = (*dir).path().string();
				std::ifstream in((*dir).path().string().c_str(), std::ios::binary);
				c->parseScript(in);
			}
		}
	}

	ScriptLoader::ScriptLoader(const std::string& fileEnding)
	{
		m_fileEnding = fileEnding;
	}

	ScriptLoader::~ScriptLoader()
	{
		clearScriptList();
	}

	void ScriptLoader::clearScriptList()
	{
		std::map <std::string, ScriptNode *>::iterator i;
		for (i = m_scriptList.begin(); i != m_scriptList.end(); i++)
		{
			delete i->second;
		}
		m_scriptList.clear();
	}

	ScriptNode *ScriptLoader::getConfigScript(const std::string &name)
	{
		std::map <std::string, ScriptNode*>::iterator i;

		std::string key = name;
		i = m_scriptList.find(key);

		//If found..
		if (i != m_scriptList.end())
		{
			return i->second;
		}
		else
		{
			return NULL;
		}
	}

	std::map <std::string, ScriptNode*> ScriptLoader::getAllConfigScripts ()
	{
		return m_scriptList;
	}

	void ScriptLoader::parseScript(std::ifstream &stream)
	{
		//Get first token
		_nextToken(stream);
		if (tok == TOKEN_EOF)
		{
			stream.close();
			return;
		}

		//Parse the script
		_parseNodes(stream, 0);

		stream.close();
	}

	void ScriptLoader::_nextToken(std::ifstream &stream)
	{
		//EOF token
		if (!stream.good())
		{
			tok = TOKEN_EOF;
			return;
		}

		//(Get next character)
		int ch = stream.get();

		while ((ch == ' ' || ch == 9) && !stream.eof())
		{    //Skip leading spaces / tabs
			ch = stream.get();
		}

		if (!stream.good())
		{
			tok = TOKEN_EOF;
			return;
		}

		//Newline token
		if (ch == '\r' || ch == '\n')
		{
			do
			{
				ch = stream.get();
			} while ((ch == '\r' || ch == '\n') && !stream.eof());

			stream.unget();

			tok = TOKEN_NewLine;
			return;
		}

		//Open brace token
		else if (ch == '{')
		{
			tok = TOKEN_OpenBrace;
			return;
		}

		//Close brace token
		else if (ch == '}')
		{
			tok = TOKEN_CloseBrace;
			return;
		}

		//Text token
		if (ch < 32 || ch > 122)    //Verify valid char
		{
			throw std::runtime_error("Parse Error: Invalid character, ConfigLoader::load()");
		}

		tokVal = "";
		tok = TOKEN_Text;
		do
		{
			//Skip comments
			if (ch == '/')
			{
				int ch2 = stream.peek();

				//C++ style comment (//)
				if (ch2 == '/')
				{
					stream.get();
					do
					{
						ch = stream.get();
					} while (ch != '\r' && ch != '\n' && !stream.eof());

					tok = TOKEN_NewLine;
					return;
				}
			}

			//Add valid char to tokVal
			tokVal += (char)ch;

			//Next char
			ch = stream.get();

		} while (ch > 32 && ch <= 122 && !stream.eof());

		stream.unget();

		return;
	}

	void ScriptLoader::_skipNewLines(std::ifstream &stream)
	{
		while (tok == TOKEN_NewLine)
		{
			_nextToken(stream);
		}
	}

	void ScriptLoader::_parseNodes(std::ifstream &stream, ScriptNode *parent)
	{
		typedef std::pair<std::string, ScriptNode*> ScriptItem;

		while (true)
		{
			switch (tok)
			{
				//Node
				case TOKEN_Text:
				{
					//Add the new node
					ScriptNode *newNode;
					if (parent)
					{
						newNode = parent->addChild(tokVal);
					}
					else
					{
						newNode = new ScriptNode(0, tokVal);
					}

					//Get values
					_nextToken(stream);
					std::string valueStr;
					int i=0;
					while (tok == TOKEN_Text)
					{
						if (i == 0)
							valueStr += tokVal;
						else
							valueStr += " " + tokVal;
						_nextToken(stream);
						++i;
					}
					newNode->setValue(valueStr);

					//Add root nodes to scriptList
					if (!parent)
					{
						std::string key;

						if (newNode->getValue() == "")
								throw std::runtime_error("Root node must have a name (\"" + newNode->getName() + "\")");
						key = newNode->getValue();

						m_scriptList.insert(ScriptItem(key, newNode));
					}

					_skipNewLines(stream);

					//Add any sub-nodes
					if (tok == TOKEN_OpenBrace)
					{
						//Parse nodes
						_nextToken(stream);
						_parseNodes(stream, newNode);
						//Check for matching closing brace
						if (tok != TOKEN_CloseBrace)
						{
							throw std::runtime_error("Parse Error: Expecting closing brace");
						}
						_nextToken(stream);
						_skipNewLines(stream);
					}

					newNode->m_fileName = m_currentFileName;

					break;
				}

				//Out of place brace
				case TOKEN_OpenBrace:
					throw std::runtime_error("Parse Error: Opening brace out of plane");
					break;

				//Return if end of nodes have been reached
				case TOKEN_CloseBrace:
					return;

				//Return if reached end of file
				case TOKEN_EOF:
					return;

				case TOKEN_NewLine:
					_nextToken(stream);
					break;
			}
		};
	}

	ScriptNode::ScriptNode(ScriptNode *parent, const std::string &name)
	{
		m_name = name;
		m_parent = parent;
		_removeSelf = true;    //For proper destruction
		m_lastChildFound = -1;

		//Add self to parent's child list (unless this is the root node being created)
		if (parent != NULL)
		{
			m_parent->m_children.push_back(this);
			_iter = --(m_parent->m_children.end());
		}
	}

	ScriptNode::~ScriptNode()
	{
		//Delete all children
		std::vector<ScriptNode*>::iterator i;
		for (i = m_children.begin(); i != m_children.end(); i++)
		{
			ScriptNode *node = *i;
			node->_removeSelf = false;
			delete node;
		}
		m_children.clear();

		//Remove self from parent's child list
		if (_removeSelf && m_parent != NULL)
		{
			m_parent->m_children.erase(_iter);
		}
	}

	ScriptNode *ScriptNode::addChild(const std::string &name, bool replaceExisting)
	{
		if (replaceExisting)
		{
			ScriptNode *node = findChild(name, false);
			if (node)
			{
				return node;
			}
		}
		return new ScriptNode(this, name);
	}

	ScriptNode *ScriptNode::findChild(const std::string &name, bool recursive)
	{
		int indx, prevC, nextC;
		int childCount = (int)m_children.size();

		if (m_lastChildFound != -1)
		{
			//If possible, try checking the nodes neighboring the last successful search
			//(often nodes searched for in sequence, so this will boost search speeds).
			prevC = m_lastChildFound-1; if (prevC < 0) prevC = 0; else if (prevC >= childCount) prevC = childCount-1;
			nextC = m_lastChildFound+1; if (nextC < 0) nextC = 0; else if (nextC >= childCount) nextC = childCount-1;
			for (indx = prevC; indx <= nextC; ++indx)
			{
				ScriptNode *node = m_children[indx];
				if (node->m_name == name)
				{
					m_lastChildFound = indx;
					return node;
				}
			}

			//If not found that way, search for the node from start to finish, avoiding the
			//already searched area above.
			for (indx = nextC + 1; indx < childCount; ++indx)
			{
				ScriptNode *node = m_children[indx];
				if (node->m_name == name) {
					m_lastChildFound = indx;
					return node;
				}
			}
			for (indx = 0; indx < prevC; ++indx)
			{
				ScriptNode *node = m_children[indx];
				if (node->m_name == name) {
					m_lastChildFound = indx;
					return node;
				}
			}
		}
		else
		{
			//Search for the node from start to finish
			for (indx = 0; indx < childCount; ++indx){
				ScriptNode *node = m_children[indx];
				if (node->m_name == name) {
					m_lastChildFound = indx;
					return node;
				}
			}
		}

		//If not found, search child nodes (if recursive == true)
		if (recursive)
		{
			for (indx = 0; indx < childCount; ++indx)
			{
				m_children[indx]->findChild(name, recursive);
			}
		}

		//Not found anywhere
		return NULL;
	}

	void ScriptNode::setParent(ScriptNode *newParent)
	{
		//Remove self from current parent
		m_parent->m_children.erase(_iter);

		//Set new parent
		m_parent = newParent;

		//Add self to new parent
		m_parent->m_children.push_back(this);
		_iter = --(m_parent->m_children.end());
	}
}
