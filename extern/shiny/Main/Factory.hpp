#ifndef SH_FACTORY_H
#define SH_FACTORY_H

#include <map>
#include <string>
#include <sstream>

#include "MaterialInstance.hpp"
#include "ShaderSet.hpp"
#include "Language.hpp"

namespace sh
{
	class Platform;

	class Configuration : public PropertySetGet
	{
	public:
		void setSourceFile (const std::string& file) { mSourceFile = file ; }
		std::string getSourceFile () { return mSourceFile; }

		void save(const std::string& name, std::ofstream &stream);

	private:
		std::string mSourceFile;
	};

	typedef std::map<std::string, MaterialInstance> MaterialMap;
	typedef std::map<std::string, ShaderSet> ShaderSetMap;
	typedef std::map<std::string, Configuration> ConfigurationMap;
	typedef std::map<int, PropertySetGet> LodConfigurationMap;
	typedef std::map<std::string, int> LastModifiedMap;

	typedef std::map<std::string, std::string> TextureAliasMap;

	/**
	 * @brief
	 * Allows you to be notified when a certain material was just created. Useful for changing material properties that you can't
	 * do in a .mat script (for example a series of animated textures) \n
	 * When receiving the event, you can get the platform material by calling m->getMaterial()
	 * and casting that to the platform specific material (e.g. for Ogre, sh::OgreMaterial)
	 */
	class MaterialListener
	{
	public:
		virtual void materialCreated (MaterialInstance* m, const std::string& configuration, unsigned short lodIndex) = 0;
	};

	/**
	 * @brief
	 * The main interface class
	 */
	class Factory
	{
	public:
		Factory(Platform* platform);
		///< @note Ownership of \a platform is transferred to this class, so you don't have to delete it.

		~Factory();

		/**
		 * Create a MaterialInstance, optionally copying all properties from \a parentInstance
		 * @param name name of the new instance
		 * @param name of the parent (optional)
		 * @return newly created instance
		 */
		MaterialInstance* createMaterialInstance (const std::string& name, const std::string& parentInstance = "");

		/// @note It is safe to call this if the instance does not exist
		void destroyMaterialInstance (const std::string& name);

		/// Use this to enable or disable shaders on-the-fly
		void setShadersEnabled (bool enabled);

		/// write generated shaders to current directory, useful for debugging
		void setShaderDebugOutputEnabled (bool enabled);

		/// Use this to manage user settings. \n
		/// Global settings can be retrieved in shaders through a macro. \n
		/// When a global setting is changed, the shaders that depend on them are recompiled automatically.
		void setGlobalSetting (const std::string& name, const std::string& value);

		/// Adjusts the given shared parameter. \n
		/// Internally, this will change all uniform parameters of this name marked with the macro \@shSharedParameter \n
		/// @param name of the shared parameter
		/// @param value of the parameter, use sh::makeProperty to construct this value
		void setSharedParameter (const std::string& name, PropertyValuePtr value);

		Language getCurrentLanguage ();

		/// Switch between different shader languages (cg, glsl, hlsl)
		void setCurrentLanguage (Language lang);

		/// Get a MaterialInstance by name
		MaterialInstance* getMaterialInstance (const std::string& name);

		/// Create a configuration, which can then be altered by using Factory::getConfiguration
		void createConfiguration (const std::string& name);

		/// Register a lod configuration, which can then be used by setting up lod distance values for the material \n
		/// 0 refers to highest lod, so use 1 or higher as index parameter
		void registerLodConfiguration (int index, PropertySetGet configuration);

		/// Set an alias name for a texture, the real name can then be retrieved with the "texture_alias"
		/// property in a texture unit - this is useful if you don't know the name of your texture beforehand. \n
		/// Example: \n
		///  - In the material definition: texture_alias ReflectionMap \n
		///  - At runtime: factory->setTextureAlias("ReflectionMap", "rtt_654654"); \n
		/// You can call factory->setTextureAlias as many times as you want, and if the material was already created, its texture will be updated!
		void setTextureAlias (const std::string& alias, const std::string& realName);

		/// Retrieve the real texture name for a texture alias (the real name is set by the user)
		std::string retrieveTextureAlias (const std::string& name);

		/// Attach a listener for material created events
		void setMaterialListener (MaterialListener* listener);

		/// Call this after you have set up basic stuff, like the shader language.
		void loadAllFiles ();

		/// Controls writing of generated shader source code to the cache folder, so that the
		/// (rather expensive) preprocessing step can be skipped on the next run. See Factory::setReadSourceCache \n
		/// \note The default is off (no cache writing)
		void setWriteSourceCache(bool write) { mWriteSourceCache = write; }

		/// Controls reading of generated shader sources from the cache folder
		/// \note The default is off (no cache reading)
		/// \note Even if microcode caching is enabled, generating (or caching) the source is still required due to the macros.
		void setReadSourceCache(bool read) { mReadSourceCache = read; }

		/// Controls writing the microcode of the generated shaders to the cache folder. Microcode is machine independent
		/// and loads very fast compared to regular compilation. Note that the availability of this feature depends on the \a Platform.
		/// \note The default is off (no cache writing)
		void setWriteMicrocodeCache(bool write) { mWriteMicrocodeCache = write; }

		/// Controls reading of shader microcode from the cache folder. Microcode is machine independent
		/// and loads very fast compared to regular compilation. Note that the availability of this feature depends on the \a Platform.
		/// \note The default is off (no cache reading)
		void setReadMicrocodeCache(bool read) { mReadMicrocodeCache = read; }

		/// Lists all materials currently registered with the factory. Whether they are
		/// loaded or not does not matter.
		void listMaterials (std::vector<std::string>& out);

		/// Lists current name & value of all global settings.
		void listGlobalSettings (std::map<std::string, std::string>& out);

		/// Lists configuration names.
		void listConfigurationNames (std::vector<std::string>& out);

		/// Lists current name & value of settings for a given configuration.
		void listConfigurationSettings (const std::string& name, std::map<std::string, std::string>& out);

		/// Lists shader sets.
		void listShaderSets (std::vector<std::string>& out);

		/// \note This only works if microcode caching is disabled, as there is currently no way to remove the cache
		/// through the Ogre API. Luckily, this is already fixed in Ogre 1.9.
		bool reloadShaders();

		/// Calls reloadShaders() if shader files have been modified since the last reload.
		/// \note This only works if microcode caching is disabled, as there is currently no way to remove the cache
		/// through the Ogre API. Luckily, this is already fixed in Ogre 1.9.
		void doMonitorShaderFiles();

		/// Unloads all materials that are currently not referenced. This will not unload the textures themselves,
		/// but it will let go of the SharedPtr's to the textures, so that you may unload them if you so desire. \n
		/// A good time to call this would be after a new level has been loaded, but just calling it occasionally after a period
		/// of time should work just fine too.
		void unloadUnreferencedMaterials();

		void destroyConfiguration (const std::string& name);

		void notifyConfigurationChanged();

		/// Saves all materials and configurations, by default to the file they were loaded from.
		/// If you wish to save them elsewhere, use setSourceFile first.
		void saveAll ();

		/// Returns the error log as a string, then clears it.
		/// Note: Errors are also written to the standard error output, or thrown if they are fatal.
		std::string getErrorLog ();

		static Factory& getInstance();
		///< Return instance of this class.

		static Factory* getInstancePtr();

		/// Make sure a material technique is loaded.\n
		/// You will probably never have to use this.
		void _ensureMaterial(const std::string& name, const std::string& configuration);


		Configuration* getConfiguration (const std::string& name);

	private:

		MaterialInstance* requestMaterial (const std::string& name, const std::string& configuration, unsigned short lodIndex);
		ShaderSet* getShaderSet (const std::string& name);
		Platform* getPlatform ();

		PropertySetGet* getCurrentGlobalSettings();

		void addTextureAliasInstance (const std::string& name, TextureUnitState* t);
		void removeTextureAliasInstances (TextureUnitState* t);

		std::string getCacheFolder () { return mPlatform->getCacheFolder (); }
		bool getReadSourceCache() { return mReadSourceCache; }
		bool getWriteSourceCache() { return mWriteSourceCache; }
	public:
		bool getWriteMicrocodeCache() { return mWriteMicrocodeCache; } // Fixme

	private:
		void setActiveConfiguration (const std::string& configuration);
		void setActiveLodLevel (int level);

		bool getShaderDebugOutputEnabled() { return mShaderDebugOutputEnabled; }

		std::map<TextureUnitState*, std::string> mTextureAliasInstances;

		void logError (const std::string& msg);

		friend class Platform;
		friend class MaterialInstance;
		friend class ShaderInstance;
		friend class ShaderSet;
		friend class TextureUnitState;

	private:
		static Factory* sThis;

		bool mShadersEnabled;
		bool mShaderDebugOutputEnabled;

		bool mReadMicrocodeCache;
		bool mWriteMicrocodeCache;
		bool mReadSourceCache;
		bool mWriteSourceCache;
		std::stringstream mErrorLog;

		MaterialMap mMaterials;
		ShaderSetMap mShaderSets;
		ConfigurationMap mConfigurations;
		LodConfigurationMap mLodConfigurations;
		LastModifiedMap mShadersLastModified;
		LastModifiedMap mShadersLastModifiedNew;

		PropertySetGet mGlobalSettings;

		PropertySetGet* mCurrentConfiguration;
		PropertySetGet* mCurrentLodConfiguration;

		TextureAliasMap mTextureAliases;

		Language mCurrentLanguage;

		MaterialListener* mListener;

		Platform* mPlatform;

		MaterialInstance* findInstance (const std::string& name);
		MaterialInstance* searchInstance (const std::string& name);

		/// @return was anything removed?
		bool removeCache (const std::string& pattern);

		static const std::string mBinaryCacheName;
	};
}

#endif
