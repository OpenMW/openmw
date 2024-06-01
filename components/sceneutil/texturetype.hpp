#ifndef OPENMW_COMPONENTS_SCENEUTIL_TEXTURETYPE_H
#define OPENMW_COMPONENTS_SCENEUTIL_TEXTURETYPE_H

#include <osg/StateAttribute>

namespace SceneUtil
{
    // The type bound to the given texture used by the ShaderVisitor to distinguish between them
    class TextureType : public osg::StateAttribute
    {
    public:
        TextureType() = default;

        TextureType(const std::string& name) { setName(name); }

        TextureType(const TextureType& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : StateAttribute(copy, copyop)
        {
        }

        static const osg::StateAttribute::Type AttributeType = static_cast<osg::StateAttribute::Type>(69420);
        META_StateAttribute(SceneUtil, TextureType, AttributeType)

        bool isTextureAttribute() const override { return true; }

        int compare(const osg::StateAttribute& sa) const override
        {
            COMPARE_StateAttribute_Types(TextureType, sa);
            COMPARE_StateAttribute_Parameter(_name);
            return 0;
        }
    };
}
#endif
