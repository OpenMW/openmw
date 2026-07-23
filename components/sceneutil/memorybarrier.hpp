#ifndef OPENMW_COMPONENTS_SCENEUTIL_MEMORYBARRIER_H
#define OPENMW_COMPONENTS_SCENEUTIL_MEMORYBARRIER_H

#include <osg/Drawable>

namespace SceneUtil
{
    struct MemoryBarrier : public osg::Drawable::DrawCallback
    {
        MemoryBarrier(GLbitfield barriers)
            : mBarriers(barriers)
        {
        }

        virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
        {
            drawable->drawImplementation(renderInfo);
            renderInfo.getState()->get<osg::GLExtensions>()->glMemoryBarrier(mBarriers);
        }
        GLbitfield mBarriers;
    };
}

#endif
