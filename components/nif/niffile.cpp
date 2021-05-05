#include "niffile.hpp"
#include "effect.hpp"

#include <array>
#include <map>
#include <sstream>

namespace Nif
{

/// Open a NIF stream. The name is used for error messages.
NIFFile::NIFFile(Files::IStreamPtr stream, const std::string &name)
    : filename(name)
{
    parse(stream);
}

NIFFile::~NIFFile()
{
    for (Record* record : records)
        delete record;
}

template <typename NodeType> static Record* construct() { return new NodeType; }

struct RecordFactoryEntry {

    using create_t = Record* (*)();

    create_t        mCreate;
    RecordType      mType;

};

///These are all the record types we know how to read.
static std::map<std::string,RecordFactoryEntry> makeFactory()
{
    std::map<std::string,RecordFactoryEntry> factory;
    factory["NiNode"]                       = {&construct <NiNode>                      , RC_NiNode                     };
    factory["NiSwitchNode"]                 = {&construct <NiSwitchNode>                , RC_NiSwitchNode               };
    factory["NiLODNode"]                    = {&construct <NiLODNode>                   , RC_NiLODNode                  };
    factory["AvoidNode"]                    = {&construct <NiNode>                      , RC_AvoidNode                  };
    factory["NiCollisionSwitch"]            = {&construct <NiNode>                      , RC_NiCollisionSwitch          };
    factory["NiBSParticleNode"]             = {&construct <NiNode>                      , RC_NiBSParticleNode           };
    factory["NiBSAnimationNode"]            = {&construct <NiNode>                      , RC_NiBSAnimationNode          };
    factory["NiBillboardNode"]              = {&construct <NiNode>                      , RC_NiBillboardNode            };
    factory["NiTriShape"]                   = {&construct <NiTriShape>                  , RC_NiTriShape                 };
    factory["NiTriStrips"]                  = {&construct <NiTriStrips>                 , RC_NiTriStrips                };
    factory["NiLines"]                      = {&construct <NiLines>                     , RC_NiLines                    };
    factory["NiParticles"]                  = {&construct <NiParticles>                 , RC_NiParticles                };
    factory["NiRotatingParticles"]          = {&construct <NiParticles>                 , RC_NiParticles                };
    factory["NiAutoNormalParticles"]        = {&construct <NiParticles>                 , RC_NiParticles                };
    factory["NiCamera"]                     = {&construct <NiCamera>                    , RC_NiCamera                   };
    factory["RootCollisionNode"]            = {&construct <NiNode>                      , RC_RootCollisionNode          };
    factory["NiTexturingProperty"]          = {&construct <NiTexturingProperty>         , RC_NiTexturingProperty        };
    factory["NiFogProperty"]                = {&construct <NiFogProperty>               , RC_NiFogProperty              };
    factory["NiMaterialProperty"]           = {&construct <NiMaterialProperty>          , RC_NiMaterialProperty         };
    factory["NiZBufferProperty"]            = {&construct <NiZBufferProperty>           , RC_NiZBufferProperty          };
    factory["NiAlphaProperty"]              = {&construct <NiAlphaProperty>             , RC_NiAlphaProperty            };
    factory["NiVertexColorProperty"]        = {&construct <NiVertexColorProperty>       , RC_NiVertexColorProperty      };
    factory["NiShadeProperty"]              = {&construct <NiShadeProperty>             , RC_NiShadeProperty            };
    factory["NiDitherProperty"]             = {&construct <NiDitherProperty>            , RC_NiDitherProperty           };
    factory["NiWireframeProperty"]          = {&construct <NiWireframeProperty>         , RC_NiWireframeProperty        };
    factory["NiSpecularProperty"]           = {&construct <NiSpecularProperty>          , RC_NiSpecularProperty         };
    factory["NiStencilProperty"]            = {&construct <NiStencilProperty>           , RC_NiStencilProperty          };
    factory["NiVisController"]              = {&construct <NiVisController>             , RC_NiVisController            };
    factory["NiGeomMorpherController"]      = {&construct <NiGeomMorpherController>     , RC_NiGeomMorpherController    };
    factory["NiKeyframeController"]         = {&construct <NiKeyframeController>        , RC_NiKeyframeController       };
    factory["NiAlphaController"]            = {&construct <NiAlphaController>           , RC_NiAlphaController          };
    factory["NiRollController"]             = {&construct <NiRollController>            , RC_NiRollController           };
    factory["NiUVController"]               = {&construct <NiUVController>              , RC_NiUVController             };
    factory["NiPathController"]             = {&construct <NiPathController>            , RC_NiPathController           };
    factory["NiMaterialColorController"]    = {&construct <NiMaterialColorController>   , RC_NiMaterialColorController  };
    factory["NiBSPArrayController"]         = {&construct <NiBSPArrayController>        , RC_NiBSPArrayController       };
    factory["NiParticleSystemController"]   = {&construct <NiParticleSystemController>  , RC_NiParticleSystemController };
    factory["NiFlipController"]             = {&construct <NiFlipController>            , RC_NiFlipController           };
    factory["NiAmbientLight"]               = {&construct <NiLight>                     , RC_NiLight                    };
    factory["NiDirectionalLight"]           = {&construct <NiLight>                     , RC_NiLight                    };
    factory["NiPointLight"]                 = {&construct <NiPointLight>                , RC_NiLight                    };
    factory["NiSpotLight"]                  = {&construct <NiSpotLight>                 , RC_NiLight                    };
    factory["NiTextureEffect"]              = {&construct <NiTextureEffect>             , RC_NiTextureEffect            };
    factory["NiVertWeightsExtraData"]       = {&construct <NiVertWeightsExtraData>      , RC_NiVertWeightsExtraData     };
    factory["NiTextKeyExtraData"]           = {&construct <NiTextKeyExtraData>          , RC_NiTextKeyExtraData         };
    factory["NiStringExtraData"]            = {&construct <NiStringExtraData>           , RC_NiStringExtraData          };
    factory["NiGravity"]                    = {&construct <NiGravity>                   , RC_NiGravity                  };
    factory["NiPlanarCollider"]             = {&construct <NiPlanarCollider>            , RC_NiPlanarCollider           };
    factory["NiSphericalCollider"]          = {&construct <NiSphericalCollider>         , RC_NiSphericalCollider        };
    factory["NiParticleGrowFade"]           = {&construct <NiParticleGrowFade>          , RC_NiParticleGrowFade         };
    factory["NiParticleColorModifier"]      = {&construct <NiParticleColorModifier>     , RC_NiParticleColorModifier    };
    factory["NiParticleRotation"]           = {&construct <NiParticleRotation>          , RC_NiParticleRotation         };
    factory["NiFloatData"]                  = {&construct <NiFloatData>                 , RC_NiFloatData                };
    factory["NiTriShapeData"]               = {&construct <NiTriShapeData>              , RC_NiTriShapeData             };
    factory["NiTriStripsData"]              = {&construct <NiTriStripsData>             , RC_NiTriStripsData            };
    factory["NiLinesData"]                  = {&construct <NiLinesData>                 , RC_NiLinesData                };
    factory["NiVisData"]                    = {&construct <NiVisData>                   , RC_NiVisData                  };
    factory["NiColorData"]                  = {&construct <NiColorData>                 , RC_NiColorData                };
    factory["NiPixelData"]                  = {&construct <NiPixelData>                 , RC_NiPixelData                };
    factory["NiMorphData"]                  = {&construct <NiMorphData>                 , RC_NiMorphData                };
    factory["NiKeyframeData"]               = {&construct <NiKeyframeData>              , RC_NiKeyframeData             };
    factory["NiSkinData"]                   = {&construct <NiSkinData>                  , RC_NiSkinData                 };
    factory["NiUVData"]                     = {&construct <NiUVData>                    , RC_NiUVData                   };
    factory["NiPosData"]                    = {&construct <NiPosData>                   , RC_NiPosData                  };
    factory["NiParticlesData"]              = {&construct <NiParticlesData>             , RC_NiParticlesData            };
    factory["NiRotatingParticlesData"]      = {&construct <NiRotatingParticlesData>     , RC_NiParticlesData            };
    factory["NiAutoNormalParticlesData"]    = {&construct <NiParticlesData>             , RC_NiParticlesData            };
    factory["NiSequenceStreamHelper"]       = {&construct <NiSequenceStreamHelper>      , RC_NiSequenceStreamHelper     };
    factory["NiSourceTexture"]              = {&construct <NiSourceTexture>             , RC_NiSourceTexture            };
    factory["NiSkinInstance"]               = {&construct <NiSkinInstance>              , RC_NiSkinInstance             };
    factory["NiLookAtController"]           = {&construct <NiLookAtController>          , RC_NiLookAtController         };
    factory["NiPalette"]                    = {&construct <NiPalette>                   , RC_NiPalette                  };
    factory["NiIntegerExtraData"]           = {&construct <NiIntegerExtraData>          , RC_NiIntegerExtraData         };
    factory["NiIntegersExtraData"]          = {&construct <NiIntegersExtraData>         , RC_NiIntegersExtraData        };
    factory["NiBinaryExtraData"]            = {&construct <NiBinaryExtraData>           , RC_NiBinaryExtraData          };
    factory["NiBooleanExtraData"]           = {&construct <NiBooleanExtraData>          , RC_NiBooleanExtraData         };
    factory["NiVectorExtraData"]            = {&construct <NiVectorExtraData>           , RC_NiVectorExtraData          };
    factory["NiColorExtraData"]             = {&construct <NiVectorExtraData>           , RC_NiColorExtraData           };
    factory["NiFloatExtraData"]             = {&construct <NiFloatExtraData>            , RC_NiFloatExtraData           };
    factory["NiFloatsExtraData"]            = {&construct <NiFloatsExtraData>           , RC_NiFloatsExtraData          };
    factory["NiStringPalette"]              = {&construct <NiStringPalette>             , RC_NiStringPalette            };
    factory["NiBoolData"]                   = {&construct <NiBoolData>                  , RC_NiBoolData                 };
    factory["NiSkinPartition"]              = {&construct <NiSkinPartition>             , RC_NiSkinPartition            };
    factory["BSXFlags"]                     = {&construct <NiIntegerExtraData>          , RC_BSXFlags                   };
    factory["BSBound"]                      = {&construct <BSBound>                     , RC_BSBound                    };
    factory["NiTransformData"]              = {&construct <NiKeyframeData>              , RC_NiKeyframeData             };
    factory["BSFadeNode"]                   = {&construct <NiNode>                      , RC_NiNode                     };
    factory["bhkBlendController"]           = {&construct <bhkBlendController>          , RC_bhkBlendController         };
    factory["NiFloatInterpolator"]          = {&construct <NiFloatInterpolator>         , RC_NiFloatInterpolator        };
    factory["NiBoolInterpolator"]           = {&construct <NiBoolInterpolator>          , RC_NiBoolInterpolator         };
    factory["NiPoint3Interpolator"]         = {&construct <NiPoint3Interpolator>        , RC_NiPoint3Interpolator       };
    factory["NiTransformController"]        = {&construct <NiKeyframeController>        , RC_NiKeyframeController       };
    factory["NiTransformInterpolator"]      = {&construct <NiTransformInterpolator>     , RC_NiTransformInterpolator    };
    factory["NiColorInterpolator"]          = {&construct <NiColorInterpolator>         , RC_NiColorInterpolator        };
    factory["BSShaderTextureSet"]           = {&construct <BSShaderTextureSet>          , RC_BSShaderTextureSet         };
    factory["BSLODTriShape"]                = {&construct <BSLODTriShape>               , RC_BSLODTriShape              };
    factory["BSShaderProperty"]             = {&construct <BSShaderProperty>            , RC_BSShaderProperty           };
    factory["BSShaderPPLightingProperty"]   = {&construct <BSShaderPPLightingProperty>  , RC_BSShaderPPLightingProperty };
    factory["BSShaderNoLightingProperty"]   = {&construct <BSShaderNoLightingProperty>  , RC_BSShaderNoLightingProperty };
    return factory;
}

///Make the factory map used for parsing the file
static const std::map<std::string,RecordFactoryEntry> factories = makeFactory();

std::string NIFFile::printVersion(unsigned int version)
{
    int major = (version >> 24) & 0xFF;
    int minor = (version >> 16) & 0xFF;
    int patch = (version >> 8) & 0xFF;
    int rev = version & 0xFF;

    std::stringstream stream;
    stream << major << "." << minor << "." << patch << "." << rev;
    return stream.str();
}

void NIFFile::parse(Files::IStreamPtr stream)
{
    NIFStream nif (this, stream);

    // Check the header string
    std::string head = nif.getVersionString();
    static const std::array<std::string, 2> verStrings =
    {
        "NetImmerse File Format",
        "Gamebryo File Format"
    };
    bool supported = false;
    for (const std::string& verString : verStrings)
    {
        supported = (head.compare(0, verString.size(), verString) == 0);
        if (supported)
            break;
    }
    if (!supported)
        fail("Invalid NIF header: " + head);

    supported = false;

    // Get BCD version
    ver = nif.getUInt();
    // 4.0.0.0 is an older, practically identical version of the format.
    // It's not used by Morrowind assets but Morrowind supports it.
    static const std::array<uint32_t, 2> supportedVers =
    {
        NIFStream::generateVersion(4,0,0,0),
        VER_MW
    };
    for (uint32_t supportedVer : supportedVers)
    {
        supported = (ver == supportedVer);
        if (supported)
            break;
    }
    if (!supported)
    {
        if (sLoadUnsupportedFiles)
            warn("Unsupported NIF version: " + printVersion(ver) + ". Proceed with caution!");
        else
            fail("Unsupported NIF version: " + printVersion(ver));
    }

    // NIF data endianness
    if (ver >= NIFStream::generateVersion(20,0,0,4))
    {
        unsigned char endianness = nif.getChar();
        if (endianness == 0)
            fail("Big endian NIF files are unsupported");
    }

    // User version
    if (ver > NIFStream::generateVersion(10,0,1,8))
        userVer = nif.getUInt();

    // Number of records
    const std::size_t recNum = nif.getUInt();
    records.resize(recNum);

    // Bethesda stream header
    // It contains Bethesda format version and (useless) export information
    if (ver == VER_OB_OLD ||
       (userVer >= 3 && ((ver == VER_OB || ver == VER_BGS)
    || (ver >= NIFStream::generateVersion(10,1,0,0) && ver <= NIFStream::generateVersion(20,0,0,4) && userVer <= 11))))
    {
        bethVer = nif.getUInt();
        nif.getExportString(); // Author
        if (bethVer > BETHVER_FO4)
            nif.getUInt(); // Unknown
        nif.getExportString(); // Process script
        nif.getExportString(); // Export script
        if (bethVer == BETHVER_FO4)
            nif.getExportString(); // Max file path
    }

    std::vector<std::string> recTypes;
    std::vector<unsigned short> recTypeIndices;

    const bool hasRecTypeListings = ver >= NIFStream::generateVersion(5,0,0,1);
    if (hasRecTypeListings)
    {
        unsigned short recTypeNum = nif.getUShort();
        if (recTypeNum) // Record type list
            nif.getSizedStrings(recTypes, recTypeNum);
        if (recNum) // Record type mapping for each record
            nif.getUShorts(recTypeIndices, recNum);
        if (ver >= NIFStream::generateVersion(5,0,0,6)) // Groups
        {
            if (ver >= NIFStream::generateVersion(20,1,0,1)) // String table
            {
                if (ver >= NIFStream::generateVersion(20,2,0,5) && recNum) // Record sizes
                {
                    std::vector<unsigned int> recSizes; // Currently unused
                    nif.getUInts(recSizes, recNum);
                }
                const std::size_t stringNum = nif.getUInt();
                nif.getUInt(); // Max string length
                if (stringNum)
                    nif.getSizedStrings(strings, stringNum);
            }
            std::vector<unsigned int> groups; // Currently unused
            unsigned int groupNum = nif.getUInt();
            if (groupNum)
                nif.getUInts(groups, groupNum);
        }
    }

    const bool hasRecordSeparators = ver >= NIFStream::generateVersion(10,0,0,0) && ver < NIFStream::generateVersion(10,2,0,0);
    for (std::size_t i = 0; i < recNum; i++)
    {
        Record *r = nullptr;

        std::string rec = hasRecTypeListings ? recTypes[recTypeIndices[i]] : nif.getString();
        if(rec.empty())
        {
            std::stringstream error;
            error << "Record number " << i << " out of " << recNum << " is blank.";
            fail(error.str());
        }

        // Record separator. Some Havok records in Oblivion do not have it.
        if (hasRecordSeparators && rec.compare(0, 3, "bhk"))
        {
            if (nif.getInt())
            {
                std::stringstream warning;
                warning << "Record number " << i << " out of " << recNum << " is preceded by a non-zero separator.";
                warn(warning.str());
            }
        }

        std::map<std::string,RecordFactoryEntry>::const_iterator entry = factories.find(rec);

        if (entry != factories.end())
        {
            r = entry->second.mCreate ();
            r->recType = entry->second.mType;
        }
        else
            fail("Unknown record type " + rec);

        if (!supported)
            Log(Debug::Verbose) << "NIF Debug: Reading record of type " << rec << ", index " << i << " (" << filename << ")";

        assert(r != nullptr);
        assert(r->recType != RC_MISSING);
        r->recName = rec;
        r->recIndex = i;
        records[i] = r;
        r->read(&nif);
    }

    const std::size_t rootNum = nif.getUInt();
    roots.resize(rootNum);

    //Determine which records are roots
    for (std::size_t i = 0; i < rootNum; i++)
    {
        int idx = nif.getInt();
        if (idx >= 0 && static_cast<std::size_t>(idx) < records.size())
        {
            roots[i] = records[idx];
        }
        else
        {
            roots[i] = nullptr;
            warn("Root " + std::to_string(i + 1) + " does not point to a record: index " + std::to_string(idx));
        }
    }

    // Once parsing is done, do post-processing.
    for (Record* record : records)
        record->post(this);
}

void NIFFile::setUseSkinning(bool skinning)
{
    mUseSkinning = skinning;
}

bool NIFFile::getUseSkinning() const
{
    return mUseSkinning;
}

bool NIFFile::sLoadUnsupportedFiles = false;

void NIFFile::setLoadUnsupportedFiles(bool load)
{
    sLoadUnsupportedFiles = load;
}

}
