#ifndef SH_MATERIALINSTANCEPASS_H
#define SH_MATERIALINSTANCEPASS_H

#include <vector>

#include "PropertyBase.hpp"
#include "MaterialInstanceTextureUnit.hpp"

namespace sh
{
	/**
	 * @brief
	 * Holds properties of a single texture unit in a \a MaterialInstancePass. \n
	 * No inheritance here for now.
	 */
	class MaterialInstancePass : public PropertySetGet
	{
	public:
		MaterialInstanceTextureUnit* createTextureUnit (const std::string& name);

		void save (std::ofstream& stream);

		PropertySetGet mShaderProperties;

		std::vector <MaterialInstanceTextureUnit> mTexUnits;
	};
}

#endif
