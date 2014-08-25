#ifndef SH_PLATFORM_H
#define SH_PLATFORM_H

#include <string>

#include <boost/shared_ptr.hpp>

#include "Language.hpp"
#include "PropertyBase.hpp"

namespace sh
{
	class Factory;
	class MaterialInstance;

	enum GpuProgramType
	{
		GPT_Vertex,
		GPT_Fragment
		// GPT_Geometry
	};

	// These classes are supposed to be filled by the platform implementation
	class GpuProgram
	{
	public:
        virtual ~GpuProgram() {}
		virtual bool getSupported () = 0; ///< @return true if the compilation was successful

		/// @param name name of the uniform in the shader
		/// @param autoConstantName name of the auto constant (for example world_viewproj_matrix)
		/// @param extraInfo if any extra info is needed (e.g. light index), put it here
		virtual void setAutoConstant (const std::string& name, const std::string& autoConstantName, const std::string& extraInfo = "") = 0;
	};

	class TextureUnitState : public PropertySet
	{
	public:
        virtual ~TextureUnitState();
		virtual void setTextureName (const std::string& textureName) = 0;

	protected:
		virtual bool setPropertyOverride (const std::string& name, PropertyValuePtr& value, PropertySetGet *context);
	};

	class Pass : public PropertySet
	{
	public:
		virtual boost::shared_ptr<TextureUnitState> createTextureUnitState (const std::string& name) = 0;
		virtual void assignProgram (GpuProgramType type, const std::string& name) = 0;

		/// @param type gpu program type
		/// @param name name of the uniform in the shader
		/// @param vt type of value, e.g. vector4
		/// @param value value to set
		/// @param context used for retrieving linked values
		virtual void setGpuConstant (int type, const std::string& name, ValueType vt, PropertyValuePtr value, PropertySetGet* context) = 0;

		virtual void setTextureUnitIndex (int programType, const std::string& name, int index) = 0;

		virtual void addSharedParameter (int type, const std::string& name) = 0;
	};

	class Material : public PropertySet
	{
	public:
		virtual boost::shared_ptr<Pass> createPass (const std::string& configuration, unsigned short lodIndex) = 0;
		virtual bool createConfiguration (const std::string& name, unsigned short lodIndex) = 0; ///< @return false if already exists
		virtual void removeAll () = 0; ///< remove all configurations

		virtual bool isUnreferenced() = 0;
		virtual void unreferenceTextures() = 0;
		virtual void ensureLoaded() = 0;

		virtual void setLodLevels (const std::string& lodLevels) = 0;

		virtual void setShadowCasterMaterial (const std::string& name) = 0;
	};

	class Platform
	{
	public:
		Platform (const std::string& basePath);
		virtual ~Platform ();

		/// set the folder to use for shader caching
		void setCacheFolder (const std::string& folder);

	private:
		virtual boost::shared_ptr<Material> createMaterial (const std::string& name) = 0;

		virtual boost::shared_ptr<GpuProgram> createGpuProgram (
			GpuProgramType type,
			const std::string& compileArguments,
			const std::string& name, const std::string& profile,
			const std::string& source, Language lang) = 0;

		virtual void destroyGpuProgram (const std::string& name) = 0;

		virtual void setSharedParameter (const std::string& name, PropertyValuePtr value) = 0;

		virtual bool isProfileSupported (const std::string& profile) = 0;

		virtual void serializeShaders (const std::string& file);
		virtual void deserializeShaders (const std::string& file);

		std::string getCacheFolder () const;

		friend class Factory;
		friend class MaterialInstance;
		friend class ShaderInstance;
		friend class ShaderSet;

	protected:
		/**
		 * this will be \a true if the platform supports serialization (writing shader microcode
		 * to disk) and deserialization (create gpu program from saved microcode)
		 */
		virtual bool supportsShaderSerialization ();

		/**
		 * this will be \a true if the platform supports a listener that notifies the system
		 * whenever a material is requested for rendering. if this is supported, shaders can be
		 * compiled on-demand when needed (and not earlier)
		 * @todo the Factory is not designed yet to handle the case where this method returns false
		 */
		virtual bool supportsMaterialQueuedListener ();

		/**
		 * fire event: material requested for rendering
		 * @param name material name
		 * @param configuration requested configuration
		 */
		MaterialInstance* fireMaterialRequested (const std::string& name, const std::string& configuration, unsigned short lodIndex);

		std::string mCacheFolder;
		Factory* mFactory;

	private:
		void setFactory (Factory* factory);

		std::string mBasePath;
		std::string getBasePath();
	};
}

#endif
