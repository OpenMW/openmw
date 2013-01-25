#include "MaterialInstancePass.hpp"

namespace sh
{

	MaterialInstanceTextureUnit* MaterialInstancePass::createTextureUnit (const std::string& name)
	{
		mTexUnits.push_back(MaterialInstanceTextureUnit(name));
		return &mTexUnits.back();
	}

	std::vector <MaterialInstanceTextureUnit> MaterialInstancePass::getTexUnits ()
	{
		return mTexUnits;
	}
}
