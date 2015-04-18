#include "node.hpp"

namespace Nif
{

void Node::getProperties(const Nif::NiTexturingProperty *&texprop,
                         const Nif::NiMaterialProperty *&matprop,
                         const Nif::NiAlphaProperty *&alphaprop,
                         const Nif::NiVertexColorProperty *&vertprop,
                         const Nif::NiZBufferProperty *&zprop,
                         const Nif::NiSpecularProperty *&specprop,
                         const Nif::NiWireframeProperty *&wireprop,
                         const Nif::NiStencilProperty *&stencilprop) const
{
    if(parent)
        parent->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);

    for(size_t i = 0;i < props.length();i++)
    {
        // Entries may be empty
        if(props[i].empty())
            continue;

        const Nif::Property *pr = props[i].getPtr();
        if(pr->recType == Nif::RC_NiTexturingProperty)
            texprop = static_cast<const Nif::NiTexturingProperty*>(pr);
        else if(pr->recType == Nif::RC_NiMaterialProperty)
            matprop = static_cast<const Nif::NiMaterialProperty*>(pr);
        else if(pr->recType == Nif::RC_NiAlphaProperty)
            alphaprop = static_cast<const Nif::NiAlphaProperty*>(pr);
        else if(pr->recType == Nif::RC_NiVertexColorProperty)
            vertprop = static_cast<const Nif::NiVertexColorProperty*>(pr);
        else if(pr->recType == Nif::RC_NiZBufferProperty)
            zprop = static_cast<const Nif::NiZBufferProperty*>(pr);
        else if(pr->recType == Nif::RC_NiSpecularProperty)
            specprop = static_cast<const Nif::NiSpecularProperty*>(pr);
        else if(pr->recType == Nif::RC_NiWireframeProperty)
            wireprop = static_cast<const Nif::NiWireframeProperty*>(pr);
        else if (pr->recType == Nif::RC_NiStencilProperty)
            stencilprop = static_cast<const Nif::NiStencilProperty*>(pr);
        // the following are unused by the MW engine
        else if (pr->recType != Nif::RC_NiFogProperty
                 && pr->recType != Nif::RC_NiDitherProperty
                 && pr->recType != Nif::RC_NiShadeProperty)
            std::cerr<< "Unhandled property type: "<<pr->recName <<std::endl;
    }
}

Ogre::Matrix4 Node::getLocalTransform() const
{
    Ogre::Matrix4 mat4 = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    mat4.makeTransform(trafo.pos, Ogre::Vector3(trafo.scale), Ogre::Quaternion(trafo.rotation));
    return mat4;
}

Ogre::Matrix4 Node::getWorldTransform() const
{
    if(parent != NULL)
        return parent->getWorldTransform() * getLocalTransform();
    return getLocalTransform();
}

}
