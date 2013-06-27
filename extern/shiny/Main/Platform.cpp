#include "Platform.hpp"

#include <stdexcept>

#include "Factory.hpp"

namespace sh
{
	Platform::Platform (const std::string& basePath)
		: mBasePath(basePath)
		, mCacheFolder("./")
		, mFactory(NULL)
	{
	}

	Platform::~Platform ()
	{
	}

	void Platform::setFactory (Factory* factory)
	{
		mFactory = factory;
	}

	std::string Platform::getBasePath ()
	{
		return mBasePath;
	}

	bool Platform::supportsMaterialQueuedListener ()
	{
		return false;
	}

	bool Platform::supportsShaderSerialization ()
	{
		return false;
	}

	MaterialInstance* Platform::fireMaterialRequested (const std::string& name, const std::string& configuration, unsigned short lodIndex)
	{
		return mFactory->requestMaterial (name, configuration, lodIndex);
	}

	void Platform::serializeShaders (const std::string& file)
	{
		throw std::runtime_error ("Shader serialization not supported by this platform");
	}

	void Platform::deserializeShaders (const std::string& file)
	{
		throw std::runtime_error ("Shader serialization not supported by this platform");
	}

	void Platform::setCacheFolder (const std::string& folder)
	{
		mCacheFolder = folder;
	}

	std::string Platform::getCacheFolder() const
	{
		return mCacheFolder;
	}

	// ------------------------------------------------------------------------------

	bool TextureUnitState::setPropertyOverride (const std::string& name, PropertyValuePtr& value, PropertySetGet *context)
	{
		if (name == "texture_alias")
		{
			std::string aliasName = retrieveValue<StringValue>(value, context).get();

			Factory::getInstance().addTextureAliasInstance (aliasName, this);

			setTextureName (Factory::getInstance().retrieveTextureAlias (aliasName));

			return true;
		}
		else
			return false;
	}

	TextureUnitState::~TextureUnitState()
	{
		Factory* f = Factory::getInstancePtr ();
		if (f)
			f->removeTextureAliasInstances (this);
	}
}
