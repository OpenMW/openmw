#include "Factory.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "Platform.hpp"
#include "ScriptLoader.hpp"
#include "ShaderSet.hpp"
#include "MaterialInstanceTextureUnit.hpp"

namespace sh
{
	Factory* Factory::sThis = 0;
	const std::string Factory::mBinaryCacheName = "binaryCache";

	Factory& Factory::getInstance()
	{
		assert (sThis);
		return *sThis;
	}

	Factory* Factory::getInstancePtr()
	{
		return sThis;
	}

	Factory::Factory (Platform* platform)
		: mPlatform(platform)
		, mShadersEnabled(true)
		, mShaderDebugOutputEnabled(false)
		, mCurrentLanguage(Language_None)
		, mListener(NULL)
		, mCurrentConfiguration(NULL)
		, mCurrentLodConfiguration(NULL)
		, mReadMicrocodeCache(false)
		, mWriteMicrocodeCache(false)
		, mReadSourceCache(false)
		, mWriteSourceCache(false)
	{
		assert (!sThis);
		sThis = this;

		mPlatform->setFactory(this);
	}

	void Factory::loadAllFiles()
	{
		assert(mCurrentLanguage != Language_None);

		try
		{
			if (boost::filesystem::exists (mPlatform->getCacheFolder () + "/lastModified.txt"))
			{
				std::ifstream file;
				file.open(std::string(mPlatform->getCacheFolder () + "/lastModified.txt").c_str());

				std::string line;
				while (getline(file, line))
				{
					std::string sourceFile = line;

					if (!getline(file, line))
						assert(0);

					int modified = boost::lexical_cast<int>(line);

					mShadersLastModified[sourceFile] = modified;
				}
			}
		}
		catch (std::exception& e)
		{
			std::cerr << "Failed to load shader modification index: " << e.what() << std::endl;
			mShadersLastModified.clear();
		}

		// load configurations
		{
			ScriptLoader shaderSetLoader(".configuration");
			ScriptLoader::loadAllFiles (&shaderSetLoader, mPlatform->getBasePath());
			std::map <std::string, ScriptNode*> nodes = shaderSetLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "configuration"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .configuration" << std::endl;
					break;
				}

				Configuration newConfiguration;
				newConfiguration.setParent(&mGlobalSettings);
				newConfiguration.setSourceFile (it->second->mFileName);

				std::vector<ScriptNode*> props = it->second->getChildren();
				for (std::vector<ScriptNode*>::const_iterator propIt = props.begin(); propIt != props.end(); ++propIt)
				{
					std::string name = (*propIt)->getName();
					std::string val = (*propIt)->getValue();

					newConfiguration.setProperty (name, makeProperty(val));
				}

				mConfigurations[it->first] = newConfiguration;
			}
		}

		// load lod configurations
		{
			ScriptLoader lodLoader(".lod");
			ScriptLoader::loadAllFiles (&lodLoader, mPlatform->getBasePath());
			std::map <std::string, ScriptNode*> nodes = lodLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "lod_configuration"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .lod" << std::endl;
					break;
				}

				if (it->first == "0")
				{
					throw std::runtime_error("lod level 0 (max lod) can't have a configuration");
				}

				PropertySetGet newLod;

				std::vector<ScriptNode*> props = it->second->getChildren();
				for (std::vector<ScriptNode*>::const_iterator propIt = props.begin(); propIt != props.end(); ++propIt)
				{
					std::string name = (*propIt)->getName();
					std::string val = (*propIt)->getValue();

					newLod.setProperty (name, makeProperty(val));
				}

				mLodConfigurations[boost::lexical_cast<int>(it->first)] = newLod;
			}
		}

		// load shader sets
		bool removeBinaryCache = reloadShaders();

		// load materials
		{
			ScriptLoader materialLoader(".mat");
			ScriptLoader::loadAllFiles (&materialLoader, mPlatform->getBasePath());

			std::map <std::string, ScriptNode*> nodes = materialLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "material"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .mat" << std::endl;
					break;
				}

				MaterialInstance newInstance(it->first, this);
				newInstance.create(mPlatform);
				if (!mShadersEnabled)
					newInstance.setShadersEnabled (false);

				newInstance.setSourceFile (it->second->mFileName);

				std::vector<ScriptNode*> props = it->second->getChildren();
				for (std::vector<ScriptNode*>::const_iterator propIt = props.begin(); propIt != props.end(); ++propIt)
				{
					std::string name = (*propIt)->getName();

					std::string val = (*propIt)->getValue();

					if (name == "pass")
					{
						MaterialInstancePass* newPass = newInstance.createPass();
						std::vector<ScriptNode*> props2 = (*propIt)->getChildren();
						for (std::vector<ScriptNode*>::const_iterator propIt2 = props2.begin(); propIt2 != props2.end(); ++propIt2)
						{
							std::string name2 = (*propIt2)->getName();
							std::string val2 = (*propIt2)->getValue();

							if (name2 == "shader_properties")
							{
								std::vector<ScriptNode*> shaderProps = (*propIt2)->getChildren();
								for (std::vector<ScriptNode*>::const_iterator shaderPropIt = shaderProps.begin(); shaderPropIt != shaderProps.end(); ++shaderPropIt)
								{
									std::string val = (*shaderPropIt)->getValue();
									newPass->mShaderProperties.setProperty((*shaderPropIt)->getName(), makeProperty(val));
								}
							}
							else if (name2 == "texture_unit")
							{
								MaterialInstanceTextureUnit* newTex = newPass->createTextureUnit(val2);
								std::vector<ScriptNode*> texProps = (*propIt2)->getChildren();
								for (std::vector<ScriptNode*>::const_iterator texPropIt = texProps.begin(); texPropIt != texProps.end(); ++texPropIt)
								{
									std::string val = (*texPropIt)->getValue();
									newTex->setProperty((*texPropIt)->getName(), makeProperty(val));
								}
							}
							else
								newPass->setProperty((*propIt2)->getName(), makeProperty(val2));
						}
					}
					else if (name == "parent")
						newInstance.setParentInstance(val);
					else
						newInstance.setProperty((*propIt)->getName(), makeProperty(val));
				}

				if (newInstance.hasProperty("create_configuration"))
				{
					std::string config = retrieveValue<StringValue>(newInstance.getProperty("create_configuration"), NULL).get();
					newInstance.createForConfiguration (config, 0);
				}

				mMaterials.insert (std::make_pair(it->first, newInstance));
			}

			// now that all materials are loaded, replace the parent names with the actual pointers to parent
			for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
			{
				std::string parent = it->second.getParentInstance();
				if (parent != "")
				{
					if (mMaterials.find (it->second.getParentInstance()) == mMaterials.end())
						throw std::runtime_error ("Unable to find parent for material instance \"" + it->first + "\"");
					it->second.setParent(&mMaterials.find(parent)->second);
				}
			}
		}

		if (mPlatform->supportsShaderSerialization () && mReadMicrocodeCache && !removeBinaryCache)
		{
			std::string file = mPlatform->getCacheFolder () + "/" + mBinaryCacheName;
			if (boost::filesystem::exists(file))
			{
				mPlatform->deserializeShaders (file);
			}
		}
	}

	Factory::~Factory ()
	{
		mShaderSets.clear();

		if (mPlatform->supportsShaderSerialization () && mWriteMicrocodeCache)
		{
			std::string file = mPlatform->getCacheFolder () + "/" + mBinaryCacheName;
			mPlatform->serializeShaders (file);
		}

		if (mReadSourceCache)
		{
			// save the last modified time of shader sources (as of when they were loaded)
			std::ofstream file;
			file.open(std::string(mPlatform->getCacheFolder () + "/lastModified.txt").c_str());

			for (LastModifiedMap::const_iterator it = mShadersLastModifiedNew.begin(); it != mShadersLastModifiedNew.end(); ++it)
			{
				file << it->first << "\n" << it->second << std::endl;
			}

			file.close();
		}

		delete mPlatform;
		sThis = 0;
	}

	MaterialInstance* Factory::searchInstance (const std::string& name)
	{
		MaterialMap::iterator it = mMaterials.find(name);
		if (it != mMaterials.end())
			return &(it->second);
		else
			return NULL;
	}

	MaterialInstance* Factory::findInstance (const std::string& name)
	{
		MaterialInstance* m = searchInstance(name);
		assert (m);
		return m;
	}

	MaterialInstance* Factory::requestMaterial (const std::string& name, const std::string& configuration, unsigned short lodIndex)
	{
		MaterialInstance* m = searchInstance (name);

		if (configuration != "Default" && mConfigurations.find(configuration) == mConfigurations.end())
			return NULL;

		if (m)
		{
			if (m->createForConfiguration (configuration, 0))
			{
				if (mListener)
					mListener->materialCreated (m, configuration, 0);
			}
			else
				return NULL;

			for (LodConfigurationMap::iterator it = mLodConfigurations.begin(); it != mLodConfigurations.end(); ++it)
			{
				if (m->createForConfiguration (configuration, it->first))
				{
					if (mListener)
						mListener->materialCreated (m, configuration, it->first);
				}
				else
					return NULL;
			}
		}
		return m;
	}

	MaterialInstance* Factory::createMaterialInstance (const std::string& name, const std::string& parentInstance)
	{
		if (parentInstance != "" && mMaterials.find(parentInstance) == mMaterials.end())
			throw std::runtime_error ("trying to clone material that does not exist");

		MaterialInstance newInstance(name, this);

		if (!mShadersEnabled)
			newInstance.setShadersEnabled(false);

		if (parentInstance != "")
			newInstance.setParent (&mMaterials.find(parentInstance)->second);

		newInstance.create(mPlatform);

		mMaterials.insert (std::make_pair(name, newInstance));

		return &mMaterials.find(name)->second;
	}

	void Factory::destroyMaterialInstance (const std::string& name)
	{
		if (mMaterials.find(name) != mMaterials.end())
			mMaterials.erase(name);
	}

	void Factory::setShadersEnabled (bool enabled)
	{
		mShadersEnabled = enabled;
		for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
		{
			it->second.setShadersEnabled(enabled);
		}
	}

	void Factory::setGlobalSetting (const std::string& name, const std::string& value)
	{
		bool changed = true;
		if (mGlobalSettings.hasProperty(name))
			changed = (retrieveValue<StringValue>(mGlobalSettings.getProperty(name), NULL).get() != value);

		mGlobalSettings.setProperty (name, makeProperty<StringValue>(new StringValue(value)));

		if (changed)
		{
			for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
			{
				it->second.destroyAll();
			}
		}
	}

	void Factory::setSharedParameter (const std::string& name, PropertyValuePtr value)
	{
		mPlatform->setSharedParameter(name, value);
	}

	ShaderSet* Factory::getShaderSet (const std::string& name)
	{
		if (mShaderSets.find(name) == mShaderSets.end())
		{
			std::stringstream msg;
			msg << "Shader '" << name << "' not found";
			throw std::runtime_error(msg.str());
		}
		return &mShaderSets.find(name)->second;
	}

	Platform* Factory::getPlatform ()
	{
		return mPlatform;
	}

	Language Factory::getCurrentLanguage ()
	{
		return mCurrentLanguage;
	}

	void Factory::setCurrentLanguage (Language lang)
	{
		bool changed = (mCurrentLanguage != lang);
		mCurrentLanguage = lang;

		if (changed)
		{
			for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
			{
				it->second.destroyAll();
			}
		}
	}

	void Factory::notifyConfigurationChanged()
	{
		for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
		{
			it->second.destroyAll();
		}
	}

	MaterialInstance* Factory::getMaterialInstance (const std::string& name)
	{
		return findInstance(name);
	}

	void Factory::setTextureAlias (const std::string& alias, const std::string& realName)
	{
		mTextureAliases[alias] = realName;

		// update the already existing texture units
		for (std::map<TextureUnitState*, std::string>::iterator it = mTextureAliasInstances.begin(); it != mTextureAliasInstances.end(); ++it)
		{
			if (it->second == alias)
			{
				it->first->setTextureName(realName);
			}
		}
	}

	std::string Factory::retrieveTextureAlias (const std::string& name)
	{
		TextureAliasMap::iterator it = mTextureAliases.find(name);
		if (it != mTextureAliases.end())
			return it->second;
		else
			return "";
	}

	Configuration* Factory::getConfiguration (const std::string& name)
	{
		return &mConfigurations[name];
	}

	void Factory::createConfiguration (const std::string& name)
	{
		mConfigurations[name].setParent (&mGlobalSettings);
	}

	void Factory::destroyConfiguration(const std::string &name)
	{
		mConfigurations.erase(name);
	}

	void Factory::registerLodConfiguration (int index, PropertySetGet configuration)
	{
		mLodConfigurations[index] = configuration;
	}

	void Factory::setMaterialListener (MaterialListener* listener)
	{
		mListener = listener;
	}

	void Factory::addTextureAliasInstance (const std::string& name, TextureUnitState* t)
	{
		mTextureAliasInstances[t] = name;
	}

	void Factory::removeTextureAliasInstances (TextureUnitState* t)
	{
		mTextureAliasInstances.erase(t);
	}

	void Factory::setActiveConfiguration (const std::string& configuration)
	{
		if (configuration == "Default")
			mCurrentConfiguration = 0;
		else
		{
			assert (mConfigurations.find(configuration) != mConfigurations.end());
			mCurrentConfiguration = &mConfigurations[configuration];
		}
	}

	void Factory::setActiveLodLevel (int level)
	{
		if (level == 0)
			mCurrentLodConfiguration = 0;
		else
		{
			assert (mLodConfigurations.find(level) != mLodConfigurations.end());
			mCurrentLodConfiguration = &mLodConfigurations[level];
		}
	}

	void Factory::setShaderDebugOutputEnabled (bool enabled)
	{
		mShaderDebugOutputEnabled = enabled;
	}

	PropertySetGet* Factory::getCurrentGlobalSettings()
	{
		PropertySetGet* p = &mGlobalSettings;

		// current global settings are affected by active configuration & active lod configuration

		if (mCurrentConfiguration)
		{
			p = mCurrentConfiguration;
		}

		if (mCurrentLodConfiguration)
		{
			mCurrentLodConfiguration->setParent(p);
			p = mCurrentLodConfiguration;
		}

		return p;
	}

	void Factory::saveAll ()
	{
		std::map<std::string, std::ofstream*> files;
		for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
		{
			if (it->second.getSourceFile().empty())
				continue;
			if (files.find(it->second.getSourceFile()) == files.end())
			{
				/// \todo check if this is actually the same file, since there can be different paths to the same file
				std::ofstream* stream = new std::ofstream();
				stream->open (it->second.getSourceFile().c_str());

				files[it->second.getSourceFile()] = stream;
			}
			it->second.save (*files[it->second.getSourceFile()]);
		}

		for (std::map<std::string, std::ofstream*>::iterator it = files.begin(); it != files.end(); ++it)
		{
			delete it->second;
		}
		files.clear();

		for (ConfigurationMap::iterator it = mConfigurations.begin(); it != mConfigurations.end(); ++it)
		{
			if (it->second.getSourceFile().empty())
				continue;
			if (files.find(it->second.getSourceFile()) == files.end())
			{
				/// \todo check if this is actually the same file, since there can be different paths to the same file
				std::ofstream* stream = new std::ofstream();
				stream->open (it->second.getSourceFile().c_str());

				files[it->second.getSourceFile()] = stream;
			}
			it->second.save (it->first, *files[it->second.getSourceFile()]);
		}

		for (std::map<std::string, std::ofstream*>::iterator it = files.begin(); it != files.end(); ++it)
		{
			delete it->second;
		}
	}

	void Factory::listMaterials(std::vector<std::string> &out)
	{
		for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
		{
			out.push_back(it->first);
		}
	}

	void Factory::listGlobalSettings(std::map<std::string, std::string> &out)
	{
		const PropertyMap& properties = mGlobalSettings.listProperties();

		for (PropertyMap::const_iterator it = properties.begin(); it != properties.end(); ++it)
		{
			out[it->first] = retrieveValue<StringValue>(mGlobalSettings.getProperty(it->first), NULL).get();
		}
	}

	void Factory::listConfigurationSettings(const std::string& name, std::map<std::string, std::string> &out)
	{
		const PropertyMap& properties = mConfigurations[name].listProperties();

		for (PropertyMap::const_iterator it = properties.begin(); it != properties.end(); ++it)
		{
			out[it->first] = retrieveValue<StringValue>(mConfigurations[name].getProperty(it->first), NULL).get();
		}
	}

	void Factory::listConfigurationNames(std::vector<std::string> &out)
	{
		for (ConfigurationMap::const_iterator it = mConfigurations.begin(); it != mConfigurations.end(); ++it)
		{
			out.push_back(it->first);
		}
	}

	void Factory::listShaderSets(std::vector<std::string> &out)
	{
		for (ShaderSetMap::const_iterator it = mShaderSets.begin(); it != mShaderSets.end(); ++it)
		{
			out.push_back(it->first);
		}
	}

	void Factory::_ensureMaterial(const std::string& name, const std::string& configuration)
	{
		MaterialInstance* m = searchInstance (name);
		assert(m);

		m->createForConfiguration (configuration, 0);

		for (LodConfigurationMap::iterator it = mLodConfigurations.begin(); it != mLodConfigurations.end(); ++it)
		{
			m->createForConfiguration (configuration, it->first);
		}
   	}

	bool Factory::removeCache(const std::string& pattern)
	{
		bool ret = false;
		if ( boost::filesystem::exists(mPlatform->getCacheFolder())
			 && boost::filesystem::is_directory(mPlatform->getCacheFolder()))
		{
			boost::filesystem::directory_iterator end_iter;
			for( boost::filesystem::directory_iterator dir_iter(mPlatform->getCacheFolder()) ; dir_iter != end_iter ; ++dir_iter)
			{
				if (boost::filesystem::is_regular_file(dir_iter->status()) )
				{
					boost::filesystem::path file = dir_iter->path();

					std::string pathname = file.filename().string();

					// get first part of filename, e.g. main_fragment_546457654 -> main_fragment
					// there is probably a better method for this...
					std::vector<std::string> tokens;
					boost::split(tokens, pathname, boost::is_any_of("_"));
					tokens.erase(--tokens.end());
					std::string shaderName;
					for (std::vector<std::string>::const_iterator vector_iter = tokens.begin(); vector_iter != tokens.end();)
					{
						shaderName += *(vector_iter++);
						if (vector_iter != tokens.end())
							shaderName += "_";
					}

					if (shaderName == pattern)
					{
						boost::filesystem::remove(file);
						ret = true;
						std::cout << "Removing outdated shader: " << file << std::endl;
					}
				}
			}
		}
		return ret;
	}

	bool Factory::reloadShaders()
	{
		mShaderSets.clear();
		notifyConfigurationChanged();

		bool removeBinaryCache = false;
		ScriptLoader shaderSetLoader(".shaderset");
		ScriptLoader::loadAllFiles (&shaderSetLoader, mPlatform->getBasePath());
		std::map <std::string, ScriptNode*> nodes = shaderSetLoader.getAllConfigScripts();
		for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
			it != nodes.end(); ++it)
		{
			if (!(it->second->getName() == "shader_set"))
			{
				std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .shaderset" << std::endl;
				break;
			}

			if (!it->second->findChild("profiles_cg"))
				throw std::runtime_error ("missing \"profiles_cg\" field for \"" + it->first + "\"");
			if (!it->second->findChild("profiles_hlsl"))
				throw std::runtime_error ("missing \"profiles_hlsl\" field for \"" + it->first + "\"");
			if (!it->second->findChild("source"))
				throw std::runtime_error ("missing \"source\" field for \"" + it->first + "\"");
			if (!it->second->findChild("type"))
				throw std::runtime_error ("missing \"type\" field for \"" + it->first + "\"");

			std::vector<std::string> profiles_cg;
			boost::split (profiles_cg, it->second->findChild("profiles_cg")->getValue(), boost::is_any_of(" "));
			std::string cg_profile;
			for (std::vector<std::string>::iterator it2 = profiles_cg.begin(); it2 != profiles_cg.end(); ++it2)
			{
				if (mPlatform->isProfileSupported(*it2))
				{
					cg_profile = *it2;
					break;
				}
			}

			std::vector<std::string> profiles_hlsl;
			boost::split (profiles_hlsl, it->second->findChild("profiles_hlsl")->getValue(), boost::is_any_of(" "));
			std::string hlsl_profile;
			for (std::vector<std::string>::iterator it2 = profiles_hlsl.begin(); it2 != profiles_hlsl.end(); ++it2)
			{
				if (mPlatform->isProfileSupported(*it2))
				{
					hlsl_profile = *it2;
					break;
				}
			}

			std::string sourceAbsolute = mPlatform->getBasePath() + "/" + it->second->findChild("source")->getValue();
			std::string sourceRelative = it->second->findChild("source")->getValue();

			ShaderSet newSet (it->second->findChild("type")->getValue(), cg_profile, hlsl_profile,
							  sourceAbsolute,
							  mPlatform->getBasePath(),
							  it->first,
							  &mGlobalSettings);

			int lastModified = boost::filesystem::last_write_time (boost::filesystem::path(sourceAbsolute));
			mShadersLastModifiedNew[sourceRelative] = lastModified;
			if (mShadersLastModified.find(sourceRelative) != mShadersLastModified.end())
			{
				if (mShadersLastModified[sourceRelative] != lastModified)
				{
					// delete any outdated shaders based on this shader set
					if (removeCache (it->first))
						removeBinaryCache = true;
				}
			}
			else
			{
				// if we get here, this is either the first run or a new shader file was added
				// in both cases we can safely delete
				if (removeCache (it->first))
					removeBinaryCache = true;
			}
			mShaderSets.insert(std::make_pair(it->first, newSet));
		}

		// new is now current
		mShadersLastModified = mShadersLastModifiedNew;

		return removeBinaryCache;
	}

	void Factory::doMonitorShaderFiles()
	{
		bool reload=false;
		ScriptLoader shaderSetLoader(".shaderset");
		ScriptLoader::loadAllFiles (&shaderSetLoader, mPlatform->getBasePath());
		std::map <std::string, ScriptNode*> nodes = shaderSetLoader.getAllConfigScripts();
		for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
			it != nodes.end(); ++it)
		{

			std::string sourceAbsolute = mPlatform->getBasePath() + "/" + it->second->findChild("source")->getValue();
			std::string sourceRelative = it->second->findChild("source")->getValue();

			int lastModified = boost::filesystem::last_write_time (boost::filesystem::path(sourceAbsolute));
			if (mShadersLastModified.find(sourceRelative) != mShadersLastModified.end())
			{
				if (mShadersLastModified[sourceRelative] != lastModified)
				{
					reload=true;
					break;
				}
			}
		}
		if (reload)
			reloadShaders();
	}

	void Factory::logError(const std::string &msg)
	{
		mErrorLog << msg << '\n';
	}

	std::string Factory::getErrorLog()
	{
		std::string errors = mErrorLog.str();
		mErrorLog.str("");
		return errors;
	}

	void Factory::unloadUnreferencedMaterials()
	{
		for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
		{
			if (it->second.getMaterial()->isUnreferenced())
				it->second.getMaterial()->unreferenceTextures();
		}
	}

	void Configuration::save(const std::string& name, std::ofstream &stream)
	{
		stream << "configuration " << name << '\n';
		stream << "{\n";
		PropertySetGet::save(stream, "\t");
		stream << "}\n";
	}
}
