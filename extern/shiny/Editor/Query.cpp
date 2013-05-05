#include "Query.hpp"

#include "../Main/Factory.hpp"

namespace sh
{

void Query::execute()
{
	executeImpl();
	mDone = true;
}

ConfigurationQuery::ConfigurationQuery(const std::string &name)
	: mName(name)
{
}

void ConfigurationQuery::executeImpl()
{
	sh::Factory::getInstance().listConfigurationSettings(mName, mProperties);
}

void MaterialQuery::executeImpl()
{
	sh::MaterialInstance* instance = sh::Factory::getInstance().getMaterialInstance(mName);

	if (instance->getParent())
		mParent = static_cast<sh::MaterialInstance*>(instance->getParent())->getName();

	// add the inherited properties
	sh::PropertySetGet* parent = instance;
	std::vector<std::string> inheritedPropertiesVector;
	while (parent->getParent())
	{
		parent = parent->getParent();
		const sh::PropertyMap& parentProperties = parent->listProperties();

		for (PropertyMap::const_iterator it = parentProperties.begin(); it != parentProperties.end(); ++it)
		{
			MaterialProperty::Source source = MaterialProperty::Inherited_Unchanged;
			MaterialProperty::Type type = getType(it->first, parent->getProperty(it->first));
			mProperties[it->first] = MaterialProperty (
						retrieveValue<sh::StringValue>(parent->getProperty(it->first), NULL).get(),
						type, source);
			inheritedPropertiesVector.push_back(it->first);
		}
	}

	// add our properties
	const sh::PropertyMap& ourProperties = instance->listProperties();
	for (PropertyMap::const_iterator it = ourProperties.begin(); it != ourProperties.end(); ++it)
	{
		MaterialProperty::Source source =
				(std::find(inheritedPropertiesVector.begin(), inheritedPropertiesVector.end(), it->first)
				 != inheritedPropertiesVector.end()) ?
					MaterialProperty::Inherited_Changed : MaterialProperty::Normal;
		MaterialProperty::Type type = getType(it->first, instance->getProperty(it->first));
		mProperties[it->first] = MaterialProperty (
					retrieveValue<sh::StringValue>(instance->getProperty(it->first), NULL).get(),
					type, source);
	}

	std::vector<MaterialInstancePass>* passes = instance->getPasses();
	for (std::vector<MaterialInstancePass>::iterator it = passes->begin(); it != passes->end(); ++it)
	{
		mPasses.push_back(PassInfo());

		const sh::PropertyMap& passProperties = it->listProperties();
		for (PropertyMap::const_iterator pit = passProperties.begin(); pit != passProperties.end(); ++pit)
		{
			PropertyValuePtr property = it->getProperty(pit->first);
			MaterialProperty::Type type = getType(pit->first, property);
			if (typeid(*property).name() == typeid(sh::LinkedValue).name())
				mPasses.back().mProperties[pit->first] = MaterialProperty("$" + property->_getStringValue(), type);
			else
				mPasses.back().mProperties[pit->first] = MaterialProperty(
						retrieveValue<sh::StringValue>(property, NULL).get(), type);
		}

		const sh::PropertyMap& shaderProperties = it->mShaderProperties.listProperties();
		for (PropertyMap::const_iterator pit = shaderProperties.begin(); pit != shaderProperties.end(); ++pit)
		{
			PropertyValuePtr property = it->mShaderProperties.getProperty(pit->first);
			MaterialProperty::Type type = getType(pit->first, property);
			if (typeid(*property).name() == typeid(sh::LinkedValue).name())
				mPasses.back().mShaderProperties[pit->first] = MaterialProperty("$" + property->_getStringValue(), type);
			else
				mPasses.back().mShaderProperties[pit->first] = MaterialProperty(
						retrieveValue<sh::StringValue>(property, NULL).get(), type);
		}

		std::vector<MaterialInstanceTextureUnit>* texUnits = &it->mTexUnits;
		for (std::vector<MaterialInstanceTextureUnit>::iterator tIt = texUnits->begin(); tIt != texUnits->end(); ++tIt)
		{
			mPasses.back().mTextureUnits.push_back(TextureUnitInfo());
			mPasses.back().mTextureUnits.back().mName = tIt->getName();
			const sh::PropertyMap& unitProperties = tIt->listProperties();
			for (PropertyMap::const_iterator pit = unitProperties.begin(); pit != unitProperties.end(); ++pit)
			{
				PropertyValuePtr property = tIt->getProperty(pit->first);
				MaterialProperty::Type type = getType(pit->first, property);
				if (typeid(*property).name() == typeid(sh::LinkedValue).name())
					mPasses.back().mTextureUnits.back().mProperties[pit->first] = MaterialProperty(
								"$" + property->_getStringValue(), MaterialProperty::Linked);
				else
					mPasses.back().mTextureUnits.back().mProperties[pit->first] = MaterialProperty(
								retrieveValue<sh::StringValue>(property, NULL).get(), type);
			}
		}
	}
}

MaterialProperty::Type MaterialQuery::getType(const std::string &key, PropertyValuePtr value)
{
	if (typeid(*value).name() == typeid(sh::LinkedValue).name())
		return MaterialProperty::Linked;

	if (key == "vertex_program" || key == "fragment_program")
		return MaterialProperty::Shader;

	std::string valueStr = retrieveValue<sh::StringValue>(value, NULL).get();

	if (valueStr == "false" || valueStr == "true")
		return MaterialProperty::Boolean;
}

void MaterialPropertyQuery::executeImpl()
{
	sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
	mValue = retrieveValue<sh::StringValue>(m->getProperty(mPropertyName), m).get();
}

}
