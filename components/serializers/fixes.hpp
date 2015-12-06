#undef REGISTER_OBJECT_WRAPPER
#define REGISTER_OBJECT_WRAPPER(NAME, CREATEINSTANCE, CLASS, ASSOCIATES) \
    extern "C" void wrapper_serializer_##NAME(void) {} \
    extern void wrapper_propfunc_##NAME(osgDB::ObjectWrapper*); \
    static osg::Object* wrapper_createinstancefunc##NAME() { return CREATEINSTANCE; } \
    osgDB::RegisterWrapperProxy NAME ( \
        wrapper_createinstancefunc##NAME, CLASS, ASSOCIATES, &wrapper_propfunc_##NAME); \
    void wrapper_propfunc_##NAME(osgDB::ObjectWrapper* wrapper)
