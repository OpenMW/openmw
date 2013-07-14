#ifndef SH_OGREGPUPROGRAM_H
#define SH_OGREGPUPROGRAM_H

#include <string>

#include <OgreHighLevelGpuProgram.h>

#include "../../Main/Platform.hpp"

namespace sh
{
	class OgreGpuProgram : public GpuProgram
	{
	public:
		OgreGpuProgram (
			GpuProgramType type,
			const std::string& compileArguments,
			const std::string& name, const std::string& profile,
			const std::string& source, const std::string& lang,
			const std::string& resourceGroup);

		virtual bool getSupported();

		virtual void setAutoConstant (const std::string& name, const std::string& autoConstantName, const std::string& extraInfo = "");

	private:
		Ogre::HighLevelGpuProgramPtr mProgram;
	};
}

#endif
