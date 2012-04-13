#ifndef COMPONENTS_NIFOVERRIDES_H
#define COMPONENTS_NIFOVERRIDES_H

#include <OgreConfigFile.h>

namespace NifOverrides
{

    typedef std::pair<bool, int> TransparencyResult;

    /// \brief provide overrides for some model / texture properties that bethesda has chosen poorly
    class Overrides
    {
    public:
        static Ogre::ConfigFile mTransparencyOverrides;
        void loadTransparencyOverrides (const std::string& file);

        static TransparencyResult getTransparencyOverride(const std::string& texture);
    };

}

#endif
