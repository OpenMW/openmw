#ifndef OPENMW_COMPONENTS_SERIALIZER_KEYMAP_HPP
#define OPENMW_COMPONENTS_SERIALIZER_KEYMAP_HPP

#include <boost/foreach.hpp>

template<typename KeyMapT>
static void writeKeyMap(osgDB::OutputStream& os, const KeyMapT& keyMap) {
    os << os.PROPERTY("Keys") << keyMap.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const typename KeyMapT::value_type& pair, keyMap) {
        os << pair.first;
        os << pair.second.mValue;
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
}

template<typename KeyMapT, typename ValueT>
static void readKeyMap(osgDB::InputStream& is, KeyMapT& keyMap) {
    // Improve performance by hinting, since keys should be in order.
    typename KeyMapT::iterator it;
    it = keyMap.begin();

    is >> is.PROPERTY("Keys");
    size_t size;
    is >> size >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        typename KeyMapT::key_type time;
        is >> time;
        ValueT value;
        is >> value;
        Nif::KeyT<ValueT> kval;
        kval.mValue = value;
        typename KeyMapT::value_type pair = typename KeyMapT::value_type(time, kval);
        it = keyMap.insert(it, pair);
    }
    is >> is.END_BRACKET;
}

#define SERIALIZER_KEYMAPT(MEMBER, MTYPE, VTYPE, NTYPE)   \
static bool check##MEMBER(const NTYPE & node) { \
    CHECKMSG(#MEMBER); \
    if (node.m##MEMBER.getMapTPtr() && node.m##MEMBER.getMapTPtr()->mKeys.size() > 0) return true; \
    return false; } \
static bool write##MEMBER(osgDB::OutputStream& os, const NTYPE & node) { \
    WRITEMSG(#MEMBER); \
    os << os.BEGIN_BRACKET << std::endl; \
    os << os.PROPERTY("Interpolation") << node.m##MEMBER.getMapTPtr()->mInterpolationType << std::endl; \
    writeKeyMap<MTYPE>(os, node.m##MEMBER.getMapTPtr()->mKeys); \
    os << os.END_BRACKET << std::endl; return true; } \
static bool read##MEMBER(osgDB::InputStream& is, NTYPE & node) { \
    READMSG(#MEMBER); \
    is >> is.BEGIN_BRACKET; node.m##MEMBER.initMapTPtr(); \
    is >> is.PROPERTY("Interpolation") >> node.m##MEMBER.getMapTPtr()->mInterpolationType; \
    readKeyMap<MTYPE, VTYPE>(is, node.m##MEMBER.getMapTPtr()->mKeys); \
    is >> is.END_BRACKET; return true; }
#endif
