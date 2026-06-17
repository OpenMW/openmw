#ifndef OPENMW_COMPONENTS_SCENEUTIL_MEMORYBARRIER_H
#define OPENMW_COMPONENTS_SCENEUTIL_MEMORYBARRIER_H

#include <osg/Drawable>

namespace SceneUtil
{
    struct MemoryBarrier : public osg::Drawable::DrawCallback
    {
        MemoryBarrier(GLbitfield barriers)
            : _barriers(barriers)
        {
        }

        virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
        {
            drawable->drawImplementation(renderInfo);
            renderInfo.getState()->get<osg::GLExtensions>()->glMemoryBarrier(_barriers);
        }
        GLbitfield _barriers;
    };
}

#endif
