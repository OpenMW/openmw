#ifndef SH_ACTIONS_H
#define SH_ACTIONS_H

#include <string>

namespace sh
{

	class Action
	{
	public:
		virtual void execute() = 0;
		virtual ~Action() {}
	};

	class ActionDeleteMaterial : public Action
	{
	public:
		ActionDeleteMaterial(const std::string& name)
			: mName(name) {}

		virtual void execute();
	private:
		std::string mName;
	};

	class ActionCloneMaterial : public Action
	{
	public:
		ActionCloneMaterial(const std::string& sourceName, const std::string& destName)
			: mSourceName(sourceName), mDestName(destName) {}

		virtual void execute();
	private:
		std::string mSourceName;
		std::string mDestName;
	};

	class ActionSaveAll : public Action
	{
	public:
		virtual void execute();
	};

	class ActionChangeGlobalSetting : public Action
	{
	public:
		ActionChangeGlobalSetting(const std::string& name, const std::string& newValue)
			: mName(name), mNewValue(newValue) {}

		virtual void execute();
	private:
		std::string mName;
		std::string mNewValue;
	};

	// configuration

	class ActionCreateConfiguration : public Action
	{
	public:
		ActionCreateConfiguration(const std::string& name)
			: mName(name) {}

		virtual void execute();
	private:
		std::string mName;

	};

	class ActionDeleteConfiguration : public Action
	{
	public:
		ActionDeleteConfiguration(const std::string& name)
			: mName(name) {}

		virtual void execute();
	private:
		std::string mName;

	};

	class ActionChangeConfiguration : public Action
	{
	public:
		ActionChangeConfiguration (const std::string& name, const std::string& key, const std::string& value)
			: mName(name), mKey(key), mValue(value) {}

		virtual void execute();
	private:
		std::string mName;
		std::string mKey;
		std::string mValue;
	};

	class ActionDeleteConfigurationProperty : public Action
	{
	public:
		ActionDeleteConfigurationProperty (const std::string& name, const std::string& key)
			: mName(name), mKey(key) {}

		virtual void execute();
	private:
		std::string mName;
		std::string mKey;
	};

	// material

	class ActionSetMaterialProperty : public Action
	{
	public:
		ActionSetMaterialProperty (const std::string& name, const std::string& key, const std::string& value)
			: mName(name), mKey(key), mValue(value) {}

		virtual void execute();
	private:
		std::string mName;
		std::string mKey;
		std::string mValue;
	};

	class ActionDeleteMaterialProperty : public Action
	{
	public:
		ActionDeleteMaterialProperty (const std::string& name, const std::string& key)
			: mName(name), mKey(key) {}

		virtual void execute();
	private:
		std::string mName;
		std::string mKey;
	};

	// pass

	class ActionCreatePass : public Action
	{
	public:
		ActionCreatePass (const std::string& name)
			: mName(name) {}

		virtual void execute();
	private:
		std::string mName;
	};

	class ActionDeletePass : public Action
	{
	public:
		ActionDeletePass (const std::string& name, int passIndex)
			: mName(name), mPassIndex(passIndex) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
	};

	class ActionSetPassProperty : public Action
	{
	public:
		ActionSetPassProperty (const std::string& name, int passIndex, const std::string& key, const std::string& value)
			: mName(name), mPassIndex(passIndex), mKey(key), mValue(value) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
		std::string mKey;
		std::string mValue;
	};

	class ActionDeletePassProperty : public Action
	{
	public:
		ActionDeletePassProperty (const std::string& name, int passIndex, const std::string& key)
			: mName(name), mPassIndex(passIndex), mKey(key) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
		std::string mKey;
	};

	// shader

	class ActionSetShaderProperty : public Action
	{
	public:
		ActionSetShaderProperty (const std::string& name, int passIndex, const std::string& key, const std::string& value)
			: mName(name), mPassIndex(passIndex), mKey(key), mValue(value) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
		std::string mKey;
		std::string mValue;
	};

	class ActionDeleteShaderProperty : public Action
	{
	public:
		ActionDeleteShaderProperty (const std::string& name, int passIndex, const std::string& key)
			: mName(name), mPassIndex(passIndex), mKey(key) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
		std::string mKey;
	};

	// texture unit

	class ActionChangeTextureUnitName : public Action
	{
	public:
		ActionChangeTextureUnitName (const std::string& name, int passIndex, int textureIndex, const std::string& texUnitName)
			: mName(name), mPassIndex(passIndex), mTextureIndex(textureIndex), mTexUnitName(texUnitName) {}

		virtual void execute();

	private:
		std::string mName;
		int mPassIndex;
		int mTextureIndex;
		std::string mTexUnitName;
	};

	class ActionCreateTextureUnit : public Action
	{
	public:
		ActionCreateTextureUnit (const std::string& name, int passIndex, const std::string& texUnitName)
			: mName(name), mPassIndex(passIndex), mTexUnitName(texUnitName) {}

		virtual void execute();

	private:
		std::string mName;
		int mPassIndex;
		std::string mTexUnitName;
	};

	class ActionDeleteTextureUnit : public Action
	{
	public:
		ActionDeleteTextureUnit (const std::string& name, int passIndex, int textureIndex)
			: mName(name), mPassIndex(passIndex), mTextureIndex(textureIndex) {}

		virtual void execute();

	private:
		std::string mName;
		int mPassIndex;
		int mTextureIndex;
	};

	class ActionMoveTextureUnit : public Action
	{
	public:
		ActionMoveTextureUnit (const std::string& name, int passIndex, int textureIndex, bool moveUp)
			: mName(name), mPassIndex(passIndex), mTextureIndex(textureIndex), mMoveUp(moveUp) {}

		virtual void execute();

	private:
		std::string mName;
		int mPassIndex;
		int mTextureIndex;
		bool mMoveUp;
	};

	class ActionSetTextureProperty : public Action
	{
	public:
		ActionSetTextureProperty (const std::string& name, int passIndex, int textureIndex, const std::string& key, const std::string& value)
			: mName(name), mPassIndex(passIndex), mTextureIndex(textureIndex), mKey(key), mValue(value) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
		int mTextureIndex;
		std::string mKey;
		std::string mValue;
	};

	class ActionDeleteTextureProperty : public Action
	{
	public:
		ActionDeleteTextureProperty (const std::string& name, int passIndex, int textureIndex, const std::string& key)
			: mName(name), mPassIndex(passIndex), mTextureIndex(textureIndex), mKey(key) {}

		virtual void execute();
	private:
		std::string mName;
		int mPassIndex;
		int mTextureIndex;
		std::string mKey;
	};

}

#endif
