#include "niffile.hpp"

#include <components/debug/debuglog.hpp>
#include <components/files/hash.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include "controlled.hpp"
#include "controller.hpp"
#include "data.hpp"
#include "effect.hpp"
#include "extra.hpp"
#include "physics.hpp"
#include "property.hpp"

namespace Nif
{

/// Open a NIF stream. The name is used for error messages.
NIFFile::NIFFile(Files::IStreamPtr&& stream, const std::string &name)
    : filename(name)
{
    parse(std::move(stream));
}

template <typename NodeType, RecordType recordType>
static std::unique_ptr<Record> construct()
{
    auto result = std::make_unique<NodeType>();
    result->recType = recordType;
    return result;
}

using CreateRecord = std::unique_ptr<Record> (*)();

///These are all the record types we know how to read.
static std::map<std::string, CreateRecord> makeFactory()
{
    return 
    {
        {"NiNode"                           , &construct <NiNode                            , RC_NiNode                             >},
        {"NiSwitchNode"                     , &construct <NiSwitchNode                      , RC_NiSwitchNode                       >},
        {"NiLODNode"                        , &construct <NiLODNode                         , RC_NiLODNode                          >},
        {"NiFltAnimationNode"               , &construct <NiFltAnimationNode                , RC_NiFltAnimationNode                 >},
        {"AvoidNode"                        , &construct <NiNode                            , RC_AvoidNode                          >},
        {"NiCollisionSwitch"                , &construct <NiNode                            , RC_NiCollisionSwitch                  >},
        {"NiBSParticleNode"                 , &construct <NiNode                            , RC_NiBSParticleNode                   >},
        {"NiBSAnimationNode"                , &construct <NiNode                            , RC_NiBSAnimationNode                  >},
        {"NiBillboardNode"                  , &construct <NiBillboardNode                   , RC_NiBillboardNode                    >},
        {"NiTriShape"                       , &construct <NiTriShape                        , RC_NiTriShape                         >},
        {"NiTriStrips"                      , &construct <NiTriStrips                       , RC_NiTriStrips                        >},
        {"NiLines"                          , &construct <NiLines                           , RC_NiLines                            >},
        {"NiParticles"                      , &construct <NiParticles                       , RC_NiParticles                        >},
        {"NiRotatingParticles"              , &construct <NiParticles                       , RC_NiParticles                        >},
        {"NiAutoNormalParticles"            , &construct <NiParticles                       , RC_NiParticles                        >},
        {"NiCamera"                         , &construct <NiCamera                          , RC_NiCamera                           >},
        {"RootCollisionNode"                , &construct <NiNode                            , RC_RootCollisionNode                  >},
        {"NiTexturingProperty"              , &construct <NiTexturingProperty               , RC_NiTexturingProperty                >},
        {"NiFogProperty"                    , &construct <NiFogProperty                     , RC_NiFogProperty                      >},
        {"NiMaterialProperty"               , &construct <NiMaterialProperty                , RC_NiMaterialProperty                 >},
        {"NiZBufferProperty"                , &construct <NiZBufferProperty                 , RC_NiZBufferProperty                  >},
        {"NiAlphaProperty"                  , &construct <NiAlphaProperty                   , RC_NiAlphaProperty                    >},
        {"NiVertexColorProperty"            , &construct <NiVertexColorProperty             , RC_NiVertexColorProperty              >},
        {"NiShadeProperty"                  , &construct <NiShadeProperty                   , RC_NiShadeProperty                    >},
        {"NiDitherProperty"                 , &construct <NiDitherProperty                  , RC_NiDitherProperty                   >},
        {"NiWireframeProperty"              , &construct <NiWireframeProperty               , RC_NiWireframeProperty                >},
        {"NiSpecularProperty"               , &construct <NiSpecularProperty                , RC_NiSpecularProperty                 >},
        {"NiStencilProperty"                , &construct <NiStencilProperty                 , RC_NiStencilProperty                  >},
        {"NiVisController"                  , &construct <NiVisController                   , RC_NiVisController                    >},
        {"NiGeomMorpherController"          , &construct <NiGeomMorpherController           , RC_NiGeomMorpherController            >},
        {"NiKeyframeController"             , &construct <NiKeyframeController              , RC_NiKeyframeController               >},
        {"NiAlphaController"                , &construct <NiAlphaController                 , RC_NiAlphaController                  >},
        {"NiRollController"                 , &construct <NiRollController                  , RC_NiRollController                   >},
        {"NiUVController"                   , &construct <NiUVController                    , RC_NiUVController                     >},
        {"NiPathController"                 , &construct <NiPathController                  , RC_NiPathController                   >},
        {"NiMaterialColorController"        , &construct <NiMaterialColorController         , RC_NiMaterialColorController          >},
        {"NiBSPArrayController"             , &construct <NiBSPArrayController              , RC_NiBSPArrayController               >},
        {"NiParticleSystemController"       , &construct <NiParticleSystemController        , RC_NiParticleSystemController         >},
        {"NiFlipController"                 , &construct <NiFlipController                  , RC_NiFlipController                   >},
        {"NiAmbientLight"                   , &construct <NiLight                           , RC_NiLight                            >},
        {"NiDirectionalLight"               , &construct <NiLight                           , RC_NiLight                            >},
        {"NiPointLight"                     , &construct <NiPointLight                      , RC_NiLight                            >},
        {"NiSpotLight"                      , &construct <NiSpotLight                       , RC_NiLight                            >},
        {"NiTextureEffect"                  , &construct <NiTextureEffect                   , RC_NiTextureEffect                    >},
        {"NiExtraData"                      , &construct <NiExtraData                       , RC_NiExtraData                        >},
        {"NiVertWeightsExtraData"           , &construct <NiVertWeightsExtraData            , RC_NiVertWeightsExtraData             >},
        {"NiTextKeyExtraData"               , &construct <NiTextKeyExtraData                , RC_NiTextKeyExtraData                 >},
        {"NiStringExtraData"                , &construct <NiStringExtraData                 , RC_NiStringExtraData                  >},
        {"NiGravity"                        , &construct <NiGravity                         , RC_NiGravity                          >},
        {"NiPlanarCollider"                 , &construct <NiPlanarCollider                  , RC_NiPlanarCollider                   >},
        {"NiSphericalCollider"              , &construct <NiSphericalCollider               , RC_NiSphericalCollider                >},
        {"NiParticleGrowFade"               , &construct <NiParticleGrowFade                , RC_NiParticleGrowFade                 >},
        {"NiParticleColorModifier"          , &construct <NiParticleColorModifier           , RC_NiParticleColorModifier            >},
        {"NiParticleRotation"               , &construct <NiParticleRotation                , RC_NiParticleRotation                 >},
        {"NiFloatData"                      , &construct <NiFloatData                       , RC_NiFloatData                        >},
        {"NiTriShapeData"                   , &construct <NiTriShapeData                    , RC_NiTriShapeData                     >},
        {"NiTriStripsData"                  , &construct <NiTriStripsData                   , RC_NiTriStripsData                    >},
        {"NiLinesData"                      , &construct <NiLinesData                       , RC_NiLinesData                        >},
        {"NiVisData"                        , &construct <NiVisData                         , RC_NiVisData                          >},
        {"NiColorData"                      , &construct <NiColorData                       , RC_NiColorData                        >},
        {"NiPixelData"                      , &construct <NiPixelData                       , RC_NiPixelData                        >},
        {"NiMorphData"                      , &construct <NiMorphData                       , RC_NiMorphData                        >},
        {"NiKeyframeData"                   , &construct <NiKeyframeData                    , RC_NiKeyframeData                     >},
        {"NiSkinData"                       , &construct <NiSkinData                        , RC_NiSkinData                         >},
        {"NiUVData"                         , &construct <NiUVData                          , RC_NiUVData                           >},
        {"NiPosData"                        , &construct <NiPosData                         , RC_NiPosData                          >},
        {"NiParticlesData"                  , &construct <NiParticlesData                   , RC_NiParticlesData                    >},
        {"NiRotatingParticlesData"          , &construct <NiRotatingParticlesData           , RC_NiParticlesData                    >},
        {"NiAutoNormalParticlesData"        , &construct <NiParticlesData                   , RC_NiParticlesData                    >},
        {"NiSequenceStreamHelper"           , &construct <NiSequenceStreamHelper            , RC_NiSequenceStreamHelper             >},
        {"NiSourceTexture"                  , &construct <NiSourceTexture                   , RC_NiSourceTexture                    >},
        {"NiSkinInstance"                   , &construct <NiSkinInstance                    , RC_NiSkinInstance                     >},
        {"NiLookAtController"               , &construct <NiLookAtController                , RC_NiLookAtController                 >},
        {"NiPalette"                        , &construct <NiPalette                         , RC_NiPalette                          >},
        {"NiIntegerExtraData"               , &construct <NiIntegerExtraData                , RC_NiIntegerExtraData                 >},
        {"NiIntegersExtraData"              , &construct <NiIntegersExtraData               , RC_NiIntegersExtraData                >},
        {"NiBinaryExtraData"                , &construct <NiBinaryExtraData                 , RC_NiBinaryExtraData                  >},
        {"NiBooleanExtraData"               , &construct <NiBooleanExtraData                , RC_NiBooleanExtraData                 >},
        {"NiVectorExtraData"                , &construct <NiVectorExtraData                 , RC_NiVectorExtraData                  >},
        {"NiColorExtraData"                 , &construct <NiVectorExtraData                 , RC_NiColorExtraData                   >},
        {"NiFloatExtraData"                 , &construct <NiFloatExtraData                  , RC_NiFloatExtraData                   >},
        {"NiFloatsExtraData"                , &construct <NiFloatsExtraData                 , RC_NiFloatsExtraData                  >},
        {"NiStringPalette"                  , &construct <NiStringPalette                   , RC_NiStringPalette                    >},
        {"NiBoolData"                       , &construct <NiBoolData                        , RC_NiBoolData                         >},
        {"NiSkinPartition"                  , &construct <NiSkinPartition                   , RC_NiSkinPartition                    >},
        {"BSXFlags"                         , &construct <NiIntegerExtraData                , RC_BSXFlags                           >},
        {"BSBound"                          , &construct <BSBound                           , RC_BSBound                            >},
        {"NiTransformData"                  , &construct <NiKeyframeData                    , RC_NiKeyframeData                     >},
        {"BSFadeNode"                       , &construct <NiNode                            , RC_NiNode                             >},
        {"bhkBlendController"               , &construct <bhkBlendController                , RC_bhkBlendController                 >},
        {"NiFloatInterpolator"              , &construct <NiFloatInterpolator               , RC_NiFloatInterpolator                >},
        {"NiBoolInterpolator"               , &construct <NiBoolInterpolator                , RC_NiBoolInterpolator                 >},
        {"NiPoint3Interpolator"             , &construct <NiPoint3Interpolator              , RC_NiPoint3Interpolator               >},
        {"NiTransformController"            , &construct <NiKeyframeController              , RC_NiKeyframeController               >},
        {"NiMultiTargetTransformController" , &construct <NiMultiTargetTransformController  , RC_NiMultiTargetTransformController   >},
        {"NiTransformInterpolator"          , &construct <NiTransformInterpolator           , RC_NiTransformInterpolator            >},
        {"NiColorInterpolator"              , &construct <NiColorInterpolator               , RC_NiColorInterpolator                >},
        {"BSShaderTextureSet"               , &construct <BSShaderTextureSet                , RC_BSShaderTextureSet                 >},
        {"BSLODTriShape"                    , &construct <BSLODTriShape                     , RC_BSLODTriShape                      >},
        {"BSShaderProperty"                 , &construct <BSShaderProperty                  , RC_BSShaderProperty                   >},
        {"BSShaderPPLightingProperty"       , &construct <BSShaderPPLightingProperty        , RC_BSShaderPPLightingProperty         >},
        {"BSShaderNoLightingProperty"       , &construct <BSShaderNoLightingProperty        , RC_BSShaderNoLightingProperty         >},
        {"BSFurnitureMarker"                , &construct <BSFurnitureMarker                 , RC_BSFurnitureMarker                  >},
        {"NiCollisionObject"                , &construct <NiCollisionObject                 , RC_NiCollisionObject                  >},
        {"bhkCollisionObject"               , &construct <bhkCollisionObject                , RC_bhkCollisionObject                 >},
        {"BSDismemberSkinInstance"          , &construct <BSDismemberSkinInstance           , RC_BSDismemberSkinInstance            >},
        {"NiControllerManager"              , &construct <NiControllerManager               , RC_NiControllerManager                >},
        {"bhkMoppBvTreeShape"               , &construct <bhkMoppBvTreeShape                , RC_bhkMoppBvTreeShape                 >},
        {"bhkNiTriStripsShape"              , &construct <bhkNiTriStripsShape               , RC_bhkNiTriStripsShape                >},
        {"bhkPackedNiTriStripsShape"        , &construct <bhkPackedNiTriStripsShape         , RC_bhkPackedNiTriStripsShape          >},
        {"hkPackedNiTriStripsData"          , &construct <hkPackedNiTriStripsData           , RC_hkPackedNiTriStripsData            >},
        {"bhkConvexVerticesShape"           , &construct <bhkConvexVerticesShape            , RC_bhkConvexVerticesShape             >},
        {"bhkBoxShape"                      , &construct <bhkBoxShape                       , RC_bhkBoxShape                        >},
        {"bhkListShape"                     , &construct <bhkListShape                      , RC_bhkListShape                       >},
        {"bhkRigidBody"                     , &construct <bhkRigidBody                      , RC_bhkRigidBody                       >},
        {"bhkRigidBodyT"                    , &construct <bhkRigidBody                      , RC_bhkRigidBodyT                      >},
        {"BSLightingShaderProperty"         , &construct <BSLightingShaderProperty          , RC_BSLightingShaderProperty           >},
        {"NiSortAdjustNode"                 , &construct <NiSortAdjustNode                  , RC_NiSortAdjustNode                   >},
        {"NiClusterAccumulator"             , &construct <NiClusterAccumulator              , RC_NiClusterAccumulator               >},
        {"NiAlphaAccumulator"               , &construct <NiAlphaAccumulator                , RC_NiAlphaAccumulator                 >},
    };
}

///Make the factory map used for parsing the file
static const std::map<std::string, CreateRecord> factories = makeFactory();

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

void NIFFile::parse(Files::IStreamPtr&& stream)
{
    const std::array<std::uint64_t, 2> fileHash = Files::getHash(filename, *stream);
    hash.append(reinterpret_cast<const char*>(fileHash.data()), fileHash.size() * sizeof(std::uint64_t));

    NIFStream nif (this, std::move(stream));

    // Check the header string
    std::string head = nif.getVersionString();
    static const std::array<std::string, 2> verStrings =
    {
        "NetImmerse File Format",
        "Gamebryo File Format"
    };
    const bool supportedHeader = std::any_of(verStrings.begin(), verStrings.end(),
        [&] (const std::string& verString) { return head.compare(0, verString.size(), verString) == 0; });
    if (!supportedHeader)
        fail("Invalid NIF header: " + head);

    // Get BCD version
    ver = nif.getUInt();
    // 4.0.0.0 is an older, practically identical version of the format.
    // It's not used by Morrowind assets but Morrowind supports it.
    static const std::array<uint32_t, 2> supportedVers =
    {
        NIFStream::generateVersion(4,0,0,0),
        VER_MW
    };
    const bool supportedVersion = std::find(supportedVers.begin(), supportedVers.end(), ver) != supportedVers.end();
    if (!supportedVersion)
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
        std::unique_ptr<Record> r;

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

        const auto entry = factories.find(rec);

        if (entry == factories.end())
            fail("Unknown record type " + rec);

        r = entry->second();

        if (!supportedVersion)
            Log(Debug::Verbose) << "NIF Debug: Reading record of type " << rec << ", index " << i << " (" << filename << ")";

        assert(r != nullptr);
        assert(r->recType != RC_MISSING);
        r->recName = rec;
        r->recIndex = i;
        r->read(&nif);
        records[i] = std::move(r);
    }

    const std::size_t rootNum = nif.getUInt();
    roots.resize(rootNum);

    //Determine which records are roots
    for (std::size_t i = 0; i < rootNum; i++)
    {
        int idx = nif.getInt();
        if (idx >= 0 && static_cast<std::size_t>(idx) < records.size())
        {
            roots[i] = records[idx].get();
        }
        else
        {
            roots[i] = nullptr;
            warn("Root " + std::to_string(i + 1) + " does not point to a record: index " + std::to_string(idx));
        }
    }

    // Once parsing is done, do post-processing.
    for (const auto& record : records)
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

std::atomic_bool NIFFile::sLoadUnsupportedFiles = false;

void NIFFile::setLoadUnsupportedFiles(bool load)
{
    sLoadUnsupportedFiles = load;
}

void NIFFile::warn(const std::string &msg) const
{
    Log(Debug::Warning) << " NIFFile Warning: " << msg << "\nFile: " << filename;
}

[[noreturn]] void NIFFile::fail(const std::string &msg) const
{
    throw std::runtime_error(" NIFFile Error: " + msg + "\nFile: " + filename);
}

std::string NIFFile::getString(uint32_t index) const
{
    if (index == std::numeric_limits<uint32_t>::max())
        return std::string();
    return strings.at(index);
}

}
