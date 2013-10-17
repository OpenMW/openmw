#include "MaterialInstancePass.hpp"

#include <fstream>

namespace sh
{

	MaterialInstanceTextureUnit* MaterialInstancePass::createTextureUnit (const std::string& name)
	{
		mTexUnits.push_back(MaterialInstanceTextureUnit(name));
		return &mTexUnits.back();
	}

	void MaterialInstancePass::save(std::ofstream &stream)
	{
		if (mShaderProperties.listProperties().size())
		{
			stream << "\t\t" << "shader_properties" << '\n';
			stream << "\t\t{\n";
			mShaderProperties.save(stream, "\t\t\t");
			stream << "\t\t}\n";
		}

		PropertySetGet::save(stream, "\t\t");

		for (std::vector <MaterialInstanceTextureUnit>::iterator it = mTexUnits.begin();
			 it != mTexUnits.end(); ++it)
		{
			stream << "\t\ttexture_unit " << it->getName() << '\n';
			stream << "\t\t{\n";
			it->save(stream, "\t\t\t");
			stream << "\t\t}\n";
		}
	}
}
