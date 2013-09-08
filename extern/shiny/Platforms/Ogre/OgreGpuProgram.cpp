#include <stdexcept>

#include "OgreGpuProgram.hpp"

#include <boost/lexical_cast.hpp>

#include <OgreHighLevelGpuProgramManager.h>
#include <OgreGpuProgramManager.h>
#include <OgreVector4.h>

namespace sh
{
	OgreGpuProgram::OgreGpuProgram(
		GpuProgramType type,
		const std::string& compileArguments,
		const std::string& name, const std::string& profile,
		const std::string& source, const std::string& lang,
		const std::string& resourceGroup)
		: GpuProgram()
	{
		Ogre::HighLevelGpuProgramManager& mgr = Ogre::HighLevelGpuProgramManager::getSingleton();
		assert (mgr.getByName(name).isNull() && "Vertex program already exists");

		Ogre::GpuProgramType t;
		if (type == GPT_Vertex)
			t = Ogre::GPT_VERTEX_PROGRAM;
		else
			t = Ogre::GPT_FRAGMENT_PROGRAM;

		mProgram = mgr.createProgram(name, resourceGroup, lang, t);
		if (lang != "glsl" && lang != "glsles")
			mProgram->setParameter("entry_point", "main");
		if (lang == "hlsl")
			mProgram->setParameter("target", profile);
		else if (lang == "cg")
			mProgram->setParameter("profiles", profile);

		mProgram->setSource(source);
		mProgram->load();

		if (mProgram.isNull() || !mProgram->isSupported())
			std::cerr << "Failed to compile shader \"" << name << "\". Consider the OGRE log for more information." << std::endl;
	}

	bool OgreGpuProgram::getSupported()
	{
		return (!mProgram.isNull() && mProgram->isSupported());
	}

	void OgreGpuProgram::setAutoConstant (const std::string& name, const std::string& autoConstantName, const std::string& extraInfo)
	{
		assert (!mProgram.isNull() && mProgram->isSupported());
		const Ogre::GpuProgramParameters::AutoConstantDefinition* d = Ogre::GpuProgramParameters::getAutoConstantDefinition(autoConstantName);

		if (!d)
			throw std::runtime_error ("can't find auto constant with name \"" + autoConstantName + "\"");
		Ogre::GpuProgramParameters::AutoConstantType t = d->acType;

		// this simplifies debugging for CG a lot.
		mProgram->getDefaultParameters()->setIgnoreMissingParams(true);

		if (d->dataType == Ogre::GpuProgramParameters::ACDT_NONE)
			mProgram->getDefaultParameters()->setNamedAutoConstant (name, t, 0);
		else if (d->dataType == Ogre::GpuProgramParameters::ACDT_INT)
			mProgram->getDefaultParameters()->setNamedAutoConstant (name, t, extraInfo == "" ? 0 : boost::lexical_cast<int>(extraInfo));
		else if (d->dataType == Ogre::GpuProgramParameters::ACDT_REAL)
			mProgram->getDefaultParameters()->setNamedAutoConstantReal (name, t, extraInfo == "" ? 0.f : boost::lexical_cast<float>(extraInfo));
	}
}
