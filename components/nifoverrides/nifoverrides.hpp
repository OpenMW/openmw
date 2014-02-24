#ifndef OPENMW_COMPONENTS_NIFOVERRIDES_NIFOVERRIDES_HPP
#define OPENMW_COMPONENTS_NIFOVERRIDES_NIFOVERRIDES_HPP

#include <OgreConfigFile.h>

namespace sh
{
    class MaterialInstance;
}

namespace NifOverrides
{

    typedef std::pair<bool, int> TransparencyResult;

    /// Allows to provide overrides for some material properties in NIF files.
    /// NIFs are a bit limited in that they don't allow specifying a material externally, which is
    /// painful for texture modding.
    /// We also use this to patch up transparency settings in certain NIFs that bethesda has chosen poorly.
    class Overrides
    {
    public:
        typedef std::map<std::string, int> TransparencyOverrideMap;
        static TransparencyOverrideMap mTransparencyOverrides;

        typedef std::map<std::string, std::map<std::string, std::string> > MaterialOverrideMap;
        static MaterialOverrideMap mMaterialOverrides;

        void loadTransparencyOverrides (const std::string& file);
        void loadMaterialOverrides (const std::string& file);

        static TransparencyResult getTransparencyOverride(const std::string& texture);
        static void getMaterialOverrides (const std::string& texture, sh::MaterialInstance* instance);
    };

}

#endif
