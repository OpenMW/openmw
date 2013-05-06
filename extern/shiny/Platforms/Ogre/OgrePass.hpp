#ifndef SH_OGREPASS_H
#define SH_OGREPASS_H

#include <OgrePass.h>

#include "../../Main/Platform.hpp"

namespace sh
{
	class OgreMaterial;

	class OgrePass : public Pass
	{
	public:
		OgrePass (OgreMaterial* parent, const std::string& configuration, unsigned short lodIndex);

		virtual boost::shared_ptr<TextureUnitState> createTextureUnitState (const std::string& name);
		virtual void assignProgram (GpuProgramType type, const std::string& name);

		Ogre::Pass* getOgrePass();

		virtual void setGpuConstant (int type, const std::string& name, ValueType vt, PropertyValuePtr value, PropertySetGet* context);

		virtual void addSharedParameter (int type, const std::string& name);
		virtual void setTextureUnitIndex (int programType, const std::string& name, int index);

	private:
		Ogre::Pass* mPass;

	protected:
		virtual bool setPropertyOverride (const std::string &name, PropertyValuePtr& value, PropertySetGet* context);
	};
}

#endif
