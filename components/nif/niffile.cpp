/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (nif_file.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#include "niffile.hpp"
#include "record.hpp"
#include "components/misc/stringops.hpp"

#include "extra.hpp"
#include "controlled.hpp"
#include "node.hpp"
#include "property.hpp"
#include "data.hpp"
#include "effect.hpp"
#include "controller.hpp"

#include <iostream>

namespace Nif
{

/// Open a NIF stream. The name is used for error messages.
NIFFile::NIFFile(const std::string &name)
    : ver(0)
    , filename(name)
{
    parse();
}

NIFFile::~NIFFile()
{
    for(std::size_t i=0; i<records.size(); i++)
        delete records[i];
}

template <typename NodeType> static Record* construct() { return new NodeType; }

struct RecordFactoryEntry {

    typedef Record* (*create_t) ();

    char const *    mName;
    create_t        mCreate;
    RecordType      mType;

};

/* These are all the record types we know how to read.

    This can be heavily optimized later if needed. For example, a
    hash table or a FSM-based parser could be used to look up
    node names.
*/

static const RecordFactoryEntry recordFactories [] = {

    { "NiNode",                     &construct <NiNode                      >, RC_NiNode                        },
    { "AvoidNode",                  &construct <NiNode                      >, RC_AvoidNode                     },
    { "NiBSParticleNode",           &construct <NiNode                      >, RC_NiBSParticleNode              },
    { "NiBSAnimationNode",          &construct <NiNode                      >, RC_NiBSAnimationNode             },
    { "NiBillboardNode",            &construct <NiNode                      >, RC_NiBillboardNode               },
    { "NiTriShape",                 &construct <NiTriShape                  >, RC_NiTriShape                    },
    { "NiRotatingParticles",        &construct <NiRotatingParticles         >, RC_NiRotatingParticles           },
    { "NiAutoNormalParticles",      &construct <NiAutoNormalParticles       >, RC_NiAutoNormalParticles         },
    { "NiCamera",                   &construct <NiCamera                    >, RC_NiCamera                      },
    { "RootCollisionNode",          &construct <NiNode                      >, RC_RootCollisionNode             },
    { "NiTexturingProperty",        &construct <NiTexturingProperty         >, RC_NiTexturingProperty           },
    { "NiMaterialProperty",         &construct <NiMaterialProperty          >, RC_NiMaterialProperty            },
    { "NiZBufferProperty",          &construct <NiZBufferProperty           >, RC_NiZBufferProperty             },
    { "NiAlphaProperty",            &construct <NiAlphaProperty             >, RC_NiAlphaProperty               },
    { "NiVertexColorProperty",      &construct <NiVertexColorProperty       >, RC_NiVertexColorProperty         },
    { "NiShadeProperty",            &construct <NiShadeProperty             >, RC_NiShadeProperty               },
    { "NiDitherProperty",           &construct <NiDitherProperty            >, RC_NiDitherProperty              },
    { "NiWireframeProperty",        &construct <NiWireframeProperty         >, RC_NiWireframeProperty           },
    { "NiSpecularProperty",         &construct <NiSpecularProperty          >, RC_NiSpecularProperty            },
    { "NiStencilProperty",          &construct <NiStencilProperty           >, RC_NiStencilProperty             },
    { "NiVisController",            &construct <NiVisController             >, RC_NiVisController               },
    { "NiGeomMorpherController",    &construct <NiGeomMorpherController     >, RC_NiGeomMorpherController       },
    { "NiKeyframeController",       &construct <NiKeyframeController        >, RC_NiKeyframeController          },
    { "NiAlphaController",          &construct <NiAlphaController           >, RC_NiAlphaController             },
    { "NiUVController",             &construct <NiUVController              >, RC_NiUVController                },
    { "NiPathController",           &construct <NiPathController            >, RC_NiPathController              },
    { "NiMaterialColorController",  &construct <NiMaterialColorController   >, RC_NiMaterialColorController     },
    { "NiBSPArrayController",       &construct <NiBSPArrayController        >, RC_NiBSPArrayController          },
    { "NiParticleSystemController", &construct <NiParticleSystemController  >, RC_NiParticleSystemController    },
    { "NiFlipController",           &construct <NiFlipController            >, RC_NiFlipController              },
    { "NiAmbientLight",             &construct <NiLight                     >, RC_NiLight                       },
    { "NiDirectionalLight",         &construct <NiLight                     >, RC_NiLight                       },
    { "NiTextureEffect",            &construct <NiTextureEffect             >, RC_NiTextureEffect               },
    { "NiVertWeightsExtraData",     &construct <NiVertWeightsExtraData      >, RC_NiVertWeightsExtraData        },
    { "NiTextKeyExtraData",         &construct <NiTextKeyExtraData          >, RC_NiTextKeyExtraData            },
    { "NiStringExtraData",          &construct <NiStringExtraData           >, RC_NiStringExtraData             },
    { "NiGravity",                  &construct <NiGravity                   >, RC_NiGravity                     },
    { "NiPlanarCollider",           &construct <NiPlanarCollider            >, RC_NiPlanarCollider              },
    { "NiParticleGrowFade",         &construct <NiParticleGrowFade          >, RC_NiParticleGrowFade            },
    { "NiParticleColorModifier",    &construct <NiParticleColorModifier     >, RC_NiParticleColorModifier       },
    { "NiParticleRotation",         &construct <NiParticleRotation          >, RC_NiParticleRotation            },
    { "NiFloatData",                &construct <NiFloatData                 >, RC_NiFloatData                   },
    { "NiTriShapeData",             &construct <NiTriShapeData              >, RC_NiTriShapeData                },
    { "NiVisData",                  &construct <NiVisData                   >, RC_NiVisData                     },
    { "NiColorData",                &construct <NiColorData                 >, RC_NiColorData                   },
    { "NiPixelData",                &construct <NiPixelData                 >, RC_NiPixelData                   },
    { "NiMorphData",                &construct <NiMorphData                 >, RC_NiMorphData                   },
    { "NiKeyframeData",             &construct <NiKeyframeData              >, RC_NiKeyframeData                },
    { "NiSkinData",                 &construct <NiSkinData                  >, RC_NiSkinData                    },
    { "NiUVData",                   &construct <NiUVData                    >, RC_NiUVData                      },
    { "NiPosData",                  &construct <NiPosData                   >, RC_NiPosData                     },
    { "NiRotatingParticlesData",    &construct <NiRotatingParticlesData     >, RC_NiRotatingParticlesData       },
    { "NiAutoNormalParticlesData",  &construct <NiAutoNormalParticlesData   >, RC_NiAutoNormalParticlesData     },
    { "NiSequenceStreamHelper",     &construct <NiSequenceStreamHelper      >, RC_NiSequenceStreamHelper        },
    { "NiSourceTexture",            &construct <NiSourceTexture             >, RC_NiSourceTexture               },
    { "NiSkinInstance",             &construct <NiSkinInstance              >, RC_NiSkinInstance                },
};

static RecordFactoryEntry const * recordFactories_begin = &recordFactories [0];
static RecordFactoryEntry const * recordFactories_end   = &recordFactories [sizeof (recordFactories) / sizeof (recordFactories[0])];

RecordFactoryEntry const * lookupRecordFactory (char const * name)
{
    RecordFactoryEntry const * i;

    for (i = recordFactories_begin; i != recordFactories_end; ++i)
        if (strcmp (name, i->mName) == 0)
            break;

    if (i == recordFactories_end)
        return NULL;

    return i;
}

/* This file implements functions from the NIFFile class. It is also
   where we stash all the functions we couldn't add as inline
   definitions in the record types.
 */

void NIFFile::parse()
{
    NIFStream nif (this, Ogre::ResourceGroupManager::getSingleton().openResource(filename));

  // Check the header string
  std::string head = nif.getString(40);
  if(head.compare(0, 22, "NetImmerse File Format") != 0)
    fail("Invalid NIF header");

  // Get BCD version
  ver = nif.getInt();
  if(ver != VER_MW)
    fail("Unsupported NIF version");

  // Number of records
  size_t recNum = nif.getInt();
  records.resize(recNum);

  /* The format for 10.0.1.0 seems to be a bit different. After the
     header, it contains the number of records, r (int), just like
     4.0.0.2, but following that it contains a short x, followed by x
     strings. Then again by r shorts, one for each record, giving
     which of the above strings to use to identify the record. After
     this follows two ints (zero?) and then the record data. However
     we do not support or plan to support other versions yet.
  */

  for(size_t i = 0;i < recNum;i++)
    {
      Record *r = NULL;

      std::string rec = nif.getString();
      if(rec.empty())
        fail("Record number " + Ogre::StringConverter::toString(i) + " out of " + Ogre::StringConverter::toString(recNum) + " is blank.");

      RecordFactoryEntry const * entry = lookupRecordFactory (rec.c_str ());

      if (entry != NULL)
      {
          r = entry->mCreate ();
          r->recType = entry->mType;
      }
      else
          fail("Unknown record type " + rec);

      assert(r != NULL);
      assert(r->recType != RC_MISSING);
      r->recName = rec;
      r->recIndex = i;
      records[i] = r;
      r->read(&nif);

      // Discard tranformations for the root node, otherwise some meshes
      // occasionally get wrong orientation. Only for NiNode-s for now, but
      // can be expanded if needed.
      // This should be rewritten when the method is cleaned up.
      if (0 == i && rec == "NiNode")
      {
          static_cast<Nif::Node*>(r)->trafo = Nif::Transformation::getIdentity();
      }
    }

    size_t rootNum = nif.getUInt();
    roots.resize(rootNum);

    for(size_t i = 0;i < rootNum;i++)
    {
        intptr_t idx = nif.getInt();
        roots[i] = ((idx >= 0) ? records.at(idx) : NULL);
    }

    // Once parsing is done, do post-processing.
    for(size_t i=0; i<recNum; i++)
        records[i]->post(this);
}

/// \todo move to the write cpp file

void NiSkinInstance::post(NIFFile *nif)
{
    data.post(nif);
    root.post(nif);
    bones.post(nif);

    if(data.empty() || root.empty())
        nif->fail("NiSkinInstance missing root or data");

    size_t bnum = bones.length();
    if(bnum != data->bones.size())
        nif->fail("Mismatch in NiSkinData bone count");

    root->makeRootBone(&data->trafo);

    for(size_t i=0; i<bnum; i++)
    {
        if(bones[i].empty())
            nif->fail("Oops: Missing bone! Don't know how to handle this.");
        bones[i]->makeBone(i, data->bones[i]);
    }
}


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
