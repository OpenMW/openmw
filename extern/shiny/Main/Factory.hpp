#ifndef SH_FACTORY_H
#define SH_FACTORY_H

#include <map>
#include <string>

#include "MaterialInstance.hpp"
#include "ShaderSet.hpp"
#include "Language.hpp"

namespace sh
{
	class Platform;

	typedef std::map<std::string, MaterialInstance> MaterialMap;
	typedef std::map<std::string, ShaderSet> ShaderSetMap;
	typedef std::map<std::string, PropertySetGet> ConfigurationMap;
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

		/// Register a configuration, which can then be used by switching the active material scheme
		void registerConfiguration (const std::string& name, PropertySetGet configuration);

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

		/// Saves all the materials that were initially loaded from the file with this name
		void saveMaterials (const std::string& filename);

		static Factory& getInstance();
		///< Return instance of this class.

		static Factory* getInstancePtr();

		/// Make sure a material technique is loaded.\n
		/// You will probably never have to use this.
		void _ensureMaterial(const std::string& name, const std::string& configuration);

	private:

		MaterialInstance* requestMaterial (const std::string& name, const std::string& configuration, unsigned short lodIndex);
		ShaderSet* getShaderSet (const std::string& name);
		PropertySetGet* getConfiguration (const std::string& name);
		Platform* getPlatform ();

		PropertySetGet* getCurrentGlobalSettings();

		void addTextureAliasInstance (const std::string& name, TextureUnitState* t);
		void removeTextureAliasInstances (TextureUnitState* t);

		std::string getCacheFolder () { return mPlatform->getCacheFolder (); }
		bool getReadSourceCache() { return mReadSourceCache; }
		bool getWriteSourceCache() { return mReadSourceCache; }
	public:
		bool getWriteMicrocodeCache() { return mWriteMicrocodeCache; } // Fixme

	private:
		void setActiveConfiguration (const std::string& configuration);
		void setActiveLodLevel (int level);

		bool getShaderDebugOutputEnabled() { return mShaderDebugOutputEnabled; }

		std::map<TextureUnitState*, std::string> mTextureAliasInstances;

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

		MaterialMap mMaterials;
		ShaderSetMap mShaderSets;
		ConfigurationMap mConfigurations;
		LodConfigurationMap mLodConfigurations;
		LastModifiedMap mShadersLastModified;

		PropertySetGet mGlobalSettings;

		PropertySetGet* mCurrentConfiguration;
		PropertySetGet* mCurrentLodConfiguration;

		TextureAliasMap mTextureAliases;

		Language mCurrentLanguage;

		MaterialListener* mListener;

		Platform* mPlatform;

		MaterialInstance* findInstance (const std::string& name);
		MaterialInstance* searchInstance (const std::string& name);
	};
}

#endif
