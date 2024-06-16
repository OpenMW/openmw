#ifndef OPENMW_COMPONENTS_NIF_RECORDPTR_HPP
#define OPENMW_COMPONENTS_NIF_RECORDPTR_HPP

#include <vector>

#include "niffile.hpp"
#include "nifstream.hpp"

namespace Nif
{

    /** A reference to another record. It is read as an index from the
        NIF, and later looked up in the index table to get an actual
        pointer.
    */
    template <class X>
    class RecordPtrT
    {
        union
        {
            intptr_t index;
            X* ptr;
        };

    public:
        RecordPtrT()
            : index(-2)
        {
        }

        RecordPtrT(X* ptr)
            : ptr(ptr)
        {
        }

        /// Read the index from the nif
        void read(NIFStream* nif)
        {
            // Can only read the index once
            assert(index == -2);

            // Store the index for later
            index = nif->get<int32_t>();
            assert(index >= -1);
        }

        /// Resolve index to pointer
        void post(Reader& nif)
        {
            if (index < 0)
                ptr = nullptr;
            else
            {
                Record* r = nif.getRecord(index);
                // And cast it
                ptr = dynamic_cast<X*>(r);
                assert(ptr != nullptr);
            }
        }

        /// Look up the actual object from the index
        const X* getPtr() const
        {
            assert(ptr != nullptr);
            return ptr;
        }
        X* getPtr()
        {
            assert(ptr != nullptr);
            return ptr;
        }

        const X& get() const { return *getPtr(); }
        X& get() { return *getPtr(); }

        /// Syntactic sugar
        const X* operator->() const { return getPtr(); }
        X* operator->() { return getPtr(); }

        /// Pointers are allowed to be empty
        bool empty() const { return ptr == nullptr; }
    };

    /** A list of references to other records. These are read as a list,
        and later converted to pointers as needed. Not an optimized
        implementation.
     */
    template <class X>
    using RecordListT = std::vector<RecordPtrT<X>>;

    template <class T>
    void readRecordList(NIFStream* nif, RecordListT<T>& list)
    {
        const std::uint32_t length = nif->get<std::uint32_t>();

        // No reasonable list can hit this generous limit
        if (length >= (1 << 24))
            throw std::runtime_error("Record list too long: " + std::to_string(length));

        list.resize(length);

        for (auto& value : list)
            value.read(nif);
    }

    template <class T>
    void postRecordList(Reader& nif, RecordListT<T>& list)
    {
        for (auto& value : list)
            value.post(nif);
    }

    struct NiAVObject;
    struct Extra;
    struct NiProperty;
    struct NiUVData;
    struct NiPosData;
    struct NiVisData;
    struct NiTimeController;
    struct NiObjectNET;
    struct NiSkinData;
    struct NiFloatData;
    struct NiMorphData;
    struct NiPixelData;
    struct NiColorData;
    struct NiKeyframeData;
    struct NiTriStripsData;
    struct NiSkinInstance;
    struct NiSourceTexture;
    struct NiPalette;
    struct NiParticleModifier;
    struct BSMasterParticleSystem;
    struct NiParticleSystem;
    struct NiPSysCollider;
    struct NiPSysColliderManager;
    struct NiPSysEmitterCtlrData;
    struct NiPSysModifier;
    struct NiPSysSpawnModifier;
    struct NiBoolData;
    struct NiBSplineData;
    struct NiBSplineBasisData;
    struct NiSkinPartition;
    struct BSShaderTextureSet;
    struct NiTriBasedGeom;
    struct NiGeometryData;
    struct BSShaderProperty;
    struct NiAlphaProperty;
    struct NiCollisionObject;
    struct bhkSystem;
    struct bhkWorldObject;
    struct bhkShape;
    struct bhkSerializable;
    struct bhkEntity;
    struct bhkConvexShape;
    struct bhkRigidBody;
    struct hkPackedNiTriStripsData;
    struct NiAccumulator;
    struct NiInterpolator;
    struct NiStringPalette;
    struct NiControllerManager;
    struct NiBlendInterpolator;
    struct NiDefaultAVObjectPalette;
    struct NiControllerSequence;
    struct bhkCompressedMeshShapeData;
    struct BSMultiBound;
    struct BSMultiBoundData;
    struct BSSkinBoneData;
    struct BSAnimNote;
    struct BSAnimNotes;

    using NiAVObjectPtr = RecordPtrT<NiAVObject>;
    using ExtraPtr = RecordPtrT<Extra>;
    using NiUVDataPtr = RecordPtrT<NiUVData>;
    using NiPosDataPtr = RecordPtrT<NiPosData>;
    using NiVisDataPtr = RecordPtrT<NiVisData>;
    using NiTimeControllerPtr = RecordPtrT<NiTimeController>;
    using NiObjectNETPtr = RecordPtrT<NiObjectNET>;
    using NiSkinDataPtr = RecordPtrT<NiSkinData>;
    using NiMorphDataPtr = RecordPtrT<NiMorphData>;
    using NiPixelDataPtr = RecordPtrT<NiPixelData>;
    using NiFloatDataPtr = RecordPtrT<NiFloatData>;
    using NiColorDataPtr = RecordPtrT<NiColorData>;
    using NiKeyframeDataPtr = RecordPtrT<NiKeyframeData>;
    using NiSkinInstancePtr = RecordPtrT<NiSkinInstance>;
    using NiSourceTexturePtr = RecordPtrT<NiSourceTexture>;
    using NiPalettePtr = RecordPtrT<NiPalette>;
    using NiParticleModifierPtr = RecordPtrT<NiParticleModifier>;
    using BSMasterParticleSystemPtr = RecordPtrT<BSMasterParticleSystem>;
    using NiParticleSystemPtr = RecordPtrT<NiParticleSystem>;
    using NiPSysColliderPtr = RecordPtrT<NiPSysCollider>;
    using NiPSysColliderManagerPtr = RecordPtrT<NiPSysColliderManager>;
    using NiPSysEmitterCtlrDataPtr = RecordPtrT<NiPSysEmitterCtlrData>;
    using NiPSysModifierPtr = RecordPtrT<NiPSysModifier>;
    using NiPSysSpawnModifierPtr = RecordPtrT<NiPSysSpawnModifier>;
    using NiBoolDataPtr = RecordPtrT<NiBoolData>;
    using NiBSplineDataPtr = RecordPtrT<NiBSplineData>;
    using NiBSplineBasisDataPtr = RecordPtrT<NiBSplineBasisData>;
    using NiSkinPartitionPtr = RecordPtrT<NiSkinPartition>;
    using BSShaderTextureSetPtr = RecordPtrT<BSShaderTextureSet>;
    using NiTriBasedGeomPtr = RecordPtrT<NiTriBasedGeom>;
    using NiGeometryDataPtr = RecordPtrT<NiGeometryData>;
    using BSShaderPropertyPtr = RecordPtrT<BSShaderProperty>;
    using NiAlphaPropertyPtr = RecordPtrT<NiAlphaProperty>;
    using NiCollisionObjectPtr = RecordPtrT<NiCollisionObject>;
    using bhkSystemPtr = RecordPtrT<bhkSystem>;
    using bhkWorldObjectPtr = RecordPtrT<bhkWorldObject>;
    using bhkShapePtr = RecordPtrT<bhkShape>;
    using bhkEntityPtr = RecordPtrT<bhkEntity>;
    using bhkConvexShapePtr = RecordPtrT<bhkConvexShape>;
    using bhkRigidBodyPtr = RecordPtrT<bhkRigidBody>;
    using hkPackedNiTriStripsDataPtr = RecordPtrT<hkPackedNiTriStripsData>;
    using NiAccumulatorPtr = RecordPtrT<NiAccumulator>;
    using NiInterpolatorPtr = RecordPtrT<NiInterpolator>;
    using NiStringPalettePtr = RecordPtrT<NiStringPalette>;
    using NiControllerManagerPtr = RecordPtrT<NiControllerManager>;
    using NiBlendInterpolatorPtr = RecordPtrT<NiBlendInterpolator>;
    using NiDefaultAVObjectPalettePtr = RecordPtrT<NiDefaultAVObjectPalette>;
    using bhkCompressedMeshShapeDataPtr = RecordPtrT<bhkCompressedMeshShapeData>;
    using BSMultiBoundPtr = RecordPtrT<BSMultiBound>;
    using BSMultiBoundDataPtr = RecordPtrT<BSMultiBoundData>;
    using BSSkinBoneDataPtr = RecordPtrT<BSSkinBoneData>;

    using NiAVObjectList = RecordListT<NiAVObject>;
    using NiPropertyList = RecordListT<NiProperty>;
    using ExtraList = RecordListT<Extra>;
    using NiSourceTextureList = RecordListT<NiSourceTexture>;
    using NiInterpolatorList = RecordListT<NiInterpolator>;
    using NiTriStripsDataList = RecordListT<NiTriStripsData>;
    using bhkShapeList = RecordListT<bhkShape>;
    using bhkSerializableList = RecordListT<bhkSerializable>;
    using bhkEntityList = RecordListT<bhkEntity>;
    using bhkRigidBodyList = RecordListT<bhkEntity>;
    using NiControllerSequenceList = RecordListT<NiControllerSequence>;
    using NiPSysModifierList = RecordListT<NiPSysModifier>;
    using NiTriBasedGeomList = RecordListT<NiTriBasedGeom>;
    using BSAnimNoteList = RecordListT<BSAnimNote>;
    using BSAnimNotesList = RecordListT<BSAnimNotes>;

} // Namespace
#endif
