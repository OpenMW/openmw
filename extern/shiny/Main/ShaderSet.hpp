#ifndef SH_SHADERSET_H
#define SH_SHADERSET_H

#include <string>
#include <vector>
#include <map>

#include "ShaderInstance.hpp"

namespace sh
{
	class PropertySetGet;

	typedef std::map<size_t, ShaderInstance> ShaderInstanceMap;

	/**
	 * @brief Contains possible shader permutations of a single uber-shader (represented by one source file)
	 */
	class ShaderSet
	{
	public:
		ShaderSet (const std::string& type, const std::string& cgProfile, const std::string& hlslProfile, const std::string& sourceFile, const std::string& basePath,
				   const std::string& name, PropertySetGet* globalSettingsPtr);
		~ShaderSet();

		/// Retrieve a shader instance for the given properties. \n
		/// If a \a ShaderInstance with the same properties exists already, simply returns this instance. \n
		/// Otherwise, creates a new \a ShaderInstance (i.e. compiles a new shader). \n
		/// Might also return NULL if the shader failed to compile. \n
		/// @note Only the properties that actually affect the shader source are taken into consideration here,
		/// so it does not matter if you pass any extra properties that the shader does not care about.
		ShaderInstance* getInstance (PropertySetGet* properties);

	private:
		PropertySetGet* getCurrentGlobalSettings() const;
		std::string getBasePath() const;
		std::string getSource() const;
		std::string getCgProfile() const;
		std::string getHlslProfile() const;
		int getType() const;

		friend class ShaderInstance;

	private:
		GpuProgramType mType;
		std::string mSource;
		std::string mBasePath;
		std::string mCgProfile;
		std::string mHlslProfile;
		std::string mName;

		std::vector <size_t> mFailedToCompile;

		std::vector <std::string> mGlobalSettings; ///< names of the global settings that affect the shader source
		std::vector <std::string> mProperties; ///< names of the per-material properties that affect the shader source

		std::vector <std::string> mPropertiesToExist;
		///< same as mProperties, however in this case, it is only relevant if the property is empty or not
		/// (we don't care about the value)

		ShaderInstanceMap mInstances; ///< maps permutation ID (generated from the properties) to \a ShaderInstance

		void parse(); ///< find out which properties and global settings affect the shader source

		size_t buildHash (PropertySetGet* properties);
	};
}

#endif
