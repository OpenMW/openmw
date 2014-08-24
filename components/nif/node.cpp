/**
 * @file
 * @brief Handle the details for node.hpp
 * @details 
 * @section LICENSE
 * OpenMW is distributed as free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 3, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * version 3 along with this program. If not, see
 * http://www.gnu.org/licenses/
 */

#include "node.hpp"

namespace Nif
{

void Node::getProperties(const Nif::NiTexturingProperty *&texprop,
                         const Nif::NiMaterialProperty *&matprop,
                         const Nif::NiAlphaProperty *&alphaprop,
                         const Nif::NiVertexColorProperty *&vertprop,
                         const Nif::NiZBufferProperty *&zprop,
                         const Nif::NiSpecularProperty *&specprop,
                         const Nif::NiWireframeProperty *&wireprop) const
{
    if(parent)
        parent->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop);

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
        else
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
