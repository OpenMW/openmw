/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef COMPONENTS_TERRAIN_MATERIAL_H
#define COMPONENTS_TERRAIN_MATERIAL_H

#include <osgFX/Technique>
#include <osgFX/Effect>

#include "defs.hpp"

namespace osg
{
    class Texture2D;
}

namespace Terrain
{

    class FixedFunctionTechnique : public osgFX::Technique
    {
    public:
        FixedFunctionTechnique(
                const std::vector<osg::ref_ptr<osg::Texture2D> >& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps);

    protected:
        virtual void define_passes() {}
    };

    class Effect : public osgFX::Effect
    {
    public:
        Effect(
                const std::vector<osg::ref_ptr<osg::Texture2D> >& layers,
                const std::vector<osg::ref_ptr<osg::Texture2D> >& blendmaps);

        virtual bool define_techniques();

        virtual const char *effectName() const
        {
            return NULL;
        }
        virtual const char *effectDescription() const
        {
            return NULL;
        }
        virtual const char *effectAuthor() const
        {
            return NULL;
        }

    private:
        std::vector<osg::ref_ptr<osg::Texture2D> > mLayers;
        std::vector<osg::ref_ptr<osg::Texture2D> > mBlendmaps;
    };

}

#endif
