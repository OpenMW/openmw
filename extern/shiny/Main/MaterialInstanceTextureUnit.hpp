#ifndef SH_MATERIALINSTANCETEXTUREUNIT_H
#define SH_MATERIALINSTANCETEXTUREUNIT_H

#include "PropertyBase.hpp"

namespace sh
{
	/**
	 * @brief
	 * A single texture unit state that belongs to a \a MaterialInstancePass \n
	 * this is not the real "backend" \a TextureUnitState (provided by \a Platform),
	 * it is merely a placeholder for properties. \n
	 * @note The backend \a TextureUnitState will only be created if this texture unit is
	 * actually used (i.e. referenced in the shader, or marked with property create_in_ffp = true).
	 */
	class MaterialInstanceTextureUnit : public PropertySetGet
	{
	public:
		MaterialInstanceTextureUnit (const std::string& name);
		std::string getName() const;
		void setName (const std::string& name) { mName = name; }
	private:
		std::string mName;
	};
}

#endif
