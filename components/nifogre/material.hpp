#ifndef COMPONENTS_NIFOGRE_MATERIAL_HPP
#define COMPONENTS_NIFOGRE_MATERIAL_HPP

#include <iostream>
#include <string>
#include <map>
#include <cassert>

#include <OgreString.h>

namespace Nif
{
    class ShapeData;
    class NiTexturingProperty;
    class NiMaterialProperty;
    class NiAlphaProperty;
    class NiVertexColorProperty;
    class NiZBufferProperty;
    class NiSpecularProperty;
    class NiWireframeProperty;
}

namespace NifOgre
{

class NIFMaterialLoader {
    static void warn(const std::string &msg)
    {
        std::cerr << "NIFMaterialLoader: Warn: " << msg << std::endl;
    }

    static void fail(const std::string &msg)
    {
        std::cerr << "NIFMaterialLoader: Fail: "<< msg << std::endl;
        abort();
    }

    static std::map<size_t,std::string> sMaterialMap;

public:
    static std::string findTextureName(const std::string &filename);

    static Ogre::String getMaterial(const Nif::ShapeData *shapedata,
                                    const Ogre::String &name, const Ogre::String &group,
                                    const Nif::NiTexturingProperty *texprop,
                                    const Nif::NiMaterialProperty *matprop,
                                    const Nif::NiAlphaProperty *alphaprop,
                                    const Nif::NiVertexColorProperty *vertprop,
                                    const Nif::NiZBufferProperty *zprop,
                                    const Nif::NiSpecularProperty *specprop,
                                    const Nif::NiWireframeProperty *wireprop,
                                    bool &needTangents, bool particleMaterial=false);
};

}

#endif
