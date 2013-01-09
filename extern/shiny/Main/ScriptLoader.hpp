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

		std::string mFileEnding;

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

		std::string mCurrentFileName;

	protected:

		float mLoadOrder;
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

		Token mToken, mLastToken;
		std::string mTokenValue;

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
			this->mName = name;
		}

		inline std::string &getName()
		{
			return mName;
		}

		inline void setValue(const std::string &value)
		{
			mValue = value;
		}

		inline std::string &getValue()
		{
			return mValue;
		}

		ScriptNode *addChild(const std::string &name = "untitled", bool replaceExisting = false);
		ScriptNode *findChild(const std::string &name, bool recursive = false);

		inline std::vector<ScriptNode*> &getChildren()
		{
			return mChildren;
		}

		inline ScriptNode *getChild(unsigned int index = 0)
		{
			assert(index < mChildren.size());
			return mChildren[index];
		}

		void setParent(ScriptNode *newParent);
 
		inline ScriptNode *getParent()
		{
			return mParent;
		}

		std::string mFileName;


	private:
		std::string mName;
		std::string mValue;
		std::vector<ScriptNode*> mChildren;
		ScriptNode *mParent;


		int mLastChildFound;  //The last child node's index found with a call to findChild()

		std::vector<ScriptNode*>::iterator mIter;
		bool mRemoveSelf;
	};
 
}
 
#endif
