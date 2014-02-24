#ifndef SH_QUERY_H
#define SH_QUERY_H

#include <string>
#include <map>
#include <vector>

#include "../Main/PropertyBase.hpp"

namespace sh
{

class Query
{
public:
	Query()
		: mDone(false) {}
	virtual ~Query()  {}

	void execute();

	bool mDone;

protected:
	virtual void executeImpl() = 0;
};

class ConfigurationQuery : public Query
{
public:
	ConfigurationQuery(const std::string& name);

	std::map<std::string, std::string> mProperties;
protected:
	std::string mName;
	virtual void executeImpl();
};


struct MaterialProperty
{

	enum Type
	{
		Texture,
		Color,
		Boolean,
		Shader,
		Misc,
		Linked,
		Object // child object, i.e. pass, texture unit, shader properties
	};

	enum Source
	{
		Normal,
		Inherited_Changed,
		Inherited_Unchanged,
		None // there is no property source (e.g. a pass, which does not have a name)
	};

	MaterialProperty() {}
	MaterialProperty (const std::string& value, Type type, Source source=Normal)
		: mValue(value), mType(type), mSource(source) {}

	std::string mValue;
	Type mType;
	Source mSource;
};


struct TextureUnitInfo
{
	std::string mName;
	std::map<std::string, MaterialProperty> mProperties;
};

struct PassInfo
{
	std::map<std::string, MaterialProperty> mShaderProperties;

	std::map<std::string, MaterialProperty> mProperties;
	std::vector<TextureUnitInfo> mTextureUnits;
};

class MaterialQuery : public Query
{
public:
	MaterialQuery(const std::string& name)
		: mName(name) {}

	std::string mParent;
	std::vector<PassInfo> mPasses;
	std::map<std::string, MaterialProperty> mProperties;

protected:
	std::string mName;
	virtual void executeImpl();

	MaterialProperty::Type getType (const std::string& key, PropertyValuePtr value);
};

class MaterialPropertyQuery : public Query
{
public:
	MaterialPropertyQuery(const std::string& name, const std::string& propertyName)
		: mName(name), mPropertyName(propertyName)
	{
	}

	std::string mValue;

	std::string mName;
	std::string mPropertyName;
protected:
	virtual void executeImpl();
};

}

#endif
