#include "MaterialInstanceTextureUnit.hpp"

namespace sh
{
	MaterialInstanceTextureUnit::MaterialInstanceTextureUnit (const std::string& name)
		: mName(name)
	{
	}

	std::string MaterialInstanceTextureUnit::getName() const
	{
		return mName;
	}
}
