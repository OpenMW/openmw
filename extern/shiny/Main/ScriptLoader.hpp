#ifndef SH_CONFIG_LOADER_H__
#define SH_CONFIG_LOADER_H__

#include <map>
#include <vector>
#include <cassert>
#include <string>
 
namespace sh
{
    class ScriptNode;

	/**
	 * @brief The base class of loaders that read Ogre style script files to get configuration and settings.
	 * Heavily inspired by: http://www.ogre3d.org/tikiwiki/All-purpose+script+parser
	 * ( "Non-ogre version")
	 */
	class ScriptLoader
	{
	public:
		static void loadAllFiles(ScriptLoader* c, const std::string& path);

		ScriptLoader(const std::string& fileEnding);
		virtual ~ScriptLoader();

		std::string m_fileEnding;

		// For a line like
		// entity animals/dog
		// {
		//    ...
		// }
		// The type is "entity" and the name is "animals/dog"
		// Or if animal/dog was not there then name is ""
		ScriptNode *getConfigScript (const std::string &name);

		std::map <std::string, ScriptNode*> getAllConfigScripts ();

		void parseScript(std::ifstream &stream);

		std::string m_currentFileName;

	protected:

		float m_LoadOrder;
		// like "*.object"

		std::map <std::string, ScriptNode*> m_scriptList;

		enum Token
		{
			TOKEN_Text,
			TOKEN_NewLine,
			TOKEN_OpenBrace,
			TOKEN_CloseBrace,
			TOKEN_EOF
		};

		Token tok, lastTok;
		std::string tokVal;

		void _parseNodes(std::ifstream &stream, ScriptNode *parent);
		void _nextToken(std::ifstream &stream);
		void _skipNewLines(std::ifstream &stream);

		void clearScriptList();
	};

	class ScriptNode
	{
	public:
		ScriptNode(ScriptNode *parent, const std::string &name = "untitled");
		~ScriptNode();

		inline void setName(const std::string &name)
		{
			this->m_name = name;
		}

		inline std::string &getName()
		{
			return m_name;
		}

		inline void setValue(const std::string &value)
		{
			m_value = value;
		}

		inline std::string &getValue()
		{
			return m_value;
		}

		ScriptNode *addChild(const std::string &name = "untitled", bool replaceExisting = false);
		ScriptNode *findChild(const std::string &name, bool recursive = false);

		inline std::vector<ScriptNode*> &getChildren()
		{
			return m_children;
		}

		inline ScriptNode *getChild(unsigned int index = 0)
		{
			assert(index < m_children.size());
			return m_children[index];
		}

		void setParent(ScriptNode *newParent);
 
		inline ScriptNode *getParent()
		{
			return m_parent;
		}

		std::string m_fileName;


	private:
		std::string m_name;
		std::string m_value;
		std::vector<ScriptNode*> m_children;
		ScriptNode *m_parent;


		int m_lastChildFound;  //The last child node's index found with a call to findChild()

		std::vector<ScriptNode*>::iterator _iter;
		bool _removeSelf;
	};
 
}
 
#endif
