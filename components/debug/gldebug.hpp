#ifndef OPENMW_COMPONENTS_DEBUG_GLDEBUG_H
#define OPENMW_COMPONENTS_DEBUG_GLDEBUG_H

#include <osgViewer/ViewerEventHandlers>

namespace Debug
{
    class EnableGLDebugOperation : public osg::GraphicsOperation
    {
    public:
        EnableGLDebugOperation();

        void operator()(osg::GraphicsContext* graphicsContext) override;

    private:
        OpenThreads::Mutex mMutex;
    };

    bool shouldDebugOpenGL();


    /*
        Debug groups allow rendering to be annotated, making debugging via APITrace/CodeXL/NSight etc. much clearer.

        Because I've not thought of a quick and clean way of doing it without incurring a small performance cost,
        there are no uses of this class checked in. For now, add annotations locally when you need them.

        To use this class, add it to a StateSet just like any other StateAttribute. Prefer the string-only constructor.
        You'll need OPENMW_DEBUG_OPENGL set to true, or shouldDebugOpenGL() redefined to just return true as otherwise
        the extension function pointers won't get set up. That can maybe be cleaned up in the future.

        Beware that consecutive identical debug groups (i.e. pointers match) won't always get applied due to OSG thinking
        it's already applied them. Either avoid nesting the same object, add dummy groups so they're not consecutive, or
        ensure the leaf group isn't identical to its parent.
    */
    class DebugGroup : public osg::StateAttribute
    {
    public:
        DebugGroup()
            : mSource(0)
            , mId(0)
            , mMessage("")
        {}

        DebugGroup(GLenum source, GLuint id, const std::string& message)
            : mSource(source)
            , mId(id)
            , mMessage(message)
        {}

        DebugGroup(const std::string& message, GLuint id = 0);

        DebugGroup(const DebugGroup& debugGroup, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_StateAttribute(Debug, DebugGroup, osg::StateAttribute::Type(101));

        void apply(osg::State& state) const override;

        int compare(const StateAttribute& sa) const override;

        void releaseGLObjects(osg::State* state = nullptr) const override;

        virtual bool isValid() const;

    protected:
        virtual ~DebugGroup() = default;

        virtual void push(osg::State& state) const;

        virtual void pop(osg::State& state) const;

        GLenum mSource;
        GLuint mId;
        std::string mMessage;

        static std::map<unsigned int, std::vector<const DebugGroup *>> sLastAppliedStack;

        friend EnableGLDebugOperation;
    };
}
#endif
