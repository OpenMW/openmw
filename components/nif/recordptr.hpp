#ifndef OPENMW_COMPONENTS_NIF_RECORDPTR_HPP
#define OPENMW_COMPONENTS_NIF_RECORDPTR_HPP

#include "niffile.hpp"
#include "nifstream.hpp"
#include <vector>

namespace Nif
{

/** A reference to another record. It is read as an index from the
    NIF, and later looked up in the index table to get an actual
    pointer.
*/
template <class X>
class RecordPtrT
{
    union {
        intptr_t index;
        X* ptr;
    };

public:
    RecordPtrT() : index(-2) {}

    RecordPtrT(X* ptr) : ptr(ptr) {}

    /// Read the index from the nif
    void read(NIFStream *nif)
    {
        // Can only read the index once
        assert(index == -2);

        // Store the index for later
        index = nif->getInt();
        assert(index >= -1);
    }

    /// Resolve index to pointer
    void post(NIFFile *nif)
    {
        if(index < 0)
            ptr = NULL;
        else
        {
            Record *r = nif->getRecord(index);
            // And cast it
            ptr = dynamic_cast<X*>(r);
            assert(ptr != NULL);
        }
    }

    /// Look up the actual object from the index
    const X* getPtr() const
    {
        assert(ptr != NULL);
        return ptr;
    }
    X* getPtr()
    {
        assert(ptr != NULL);
        return ptr;
    }

    const X& get() const
    { return *getPtr(); }
    X& get()
    { return *getPtr(); }

    /// Syntactic sugar
    const X* operator->() const
    { return getPtr(); }
    X* operator->()
    { return getPtr(); }

    /// Pointers are allowed to be empty
    bool empty() const
    { return ptr == NULL; }
};

/** A list of references to other records. These are read as a list,
    and later converted to pointers as needed. Not an optimized
    implementation.
 */
template <class X>
class RecordListT
{
    typedef RecordPtrT<X> Ptr;
    std::vector<Ptr> list;

public:
    RecordListT() = default;

    RecordListT(std::vector<Ptr> list)
        : list(std::move(list))
    {}

    void read(NIFStream *nif)
    {
        int len = nif->getInt();
        list.resize(len);

        for(size_t i=0;i < list.size();i++)
            list[i].read(nif);
    }

    void post(NIFFile *nif)
    {
        for(size_t i=0;i < list.size();i++)
            list[i].post(nif);
    }

    const Ptr& operator[](size_t index) const
    { return list.at(index); }
    Ptr& operator[](size_t index)
    { return list.at(index); }

    size_t length() const
    { return list.size(); }
};


class Node;
class Extra;
class Property;
class NiUVData;
class NiPosData;
class NiVisData;
class Controller;
class Controlled;
class NiSkinData;
class NiFloatData;
struct NiMorphData;
class NiPixelData;
class NiColorData;
struct NiKeyframeData;
class NiTriShapeData;
class NiSkinInstance;
class NiSourceTexture;
class NiRotatingParticlesData;
class NiAutoNormalParticlesData;

typedef RecordPtrT<Node> NodePtr;
typedef RecordPtrT<Extra> ExtraPtr;
typedef RecordPtrT<NiUVData> NiUVDataPtr;
typedef RecordPtrT<NiPosData> NiPosDataPtr;
typedef RecordPtrT<NiVisData> NiVisDataPtr;
typedef RecordPtrT<Controller> ControllerPtr;
typedef RecordPtrT<Controlled> ControlledPtr;
typedef RecordPtrT<NiSkinData> NiSkinDataPtr;
typedef RecordPtrT<NiMorphData> NiMorphDataPtr;
typedef RecordPtrT<NiPixelData> NiPixelDataPtr;
typedef RecordPtrT<NiFloatData> NiFloatDataPtr;
typedef RecordPtrT<NiColorData> NiColorDataPtr;
typedef RecordPtrT<NiKeyframeData> NiKeyframeDataPtr;
typedef RecordPtrT<NiTriShapeData> NiTriShapeDataPtr;
typedef RecordPtrT<NiSkinInstance> NiSkinInstancePtr;
typedef RecordPtrT<NiSourceTexture> NiSourceTexturePtr;
typedef RecordPtrT<NiRotatingParticlesData> NiRotatingParticlesDataPtr;
typedef RecordPtrT<NiAutoNormalParticlesData> NiAutoNormalParticlesDataPtr;

typedef RecordListT<Node> NodeList;
typedef RecordListT<Property> PropertyList;
typedef RecordListT<NiSourceTexture> NiSourceTextureList;

} // Namespace
#endif
