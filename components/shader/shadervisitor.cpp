#include "shadervisitor.hpp"

#include <iostream>

#include <osg/Texture>
#include <osg/Material>
#include <osg/Geometry>

#include <osgUtil/TangentSpaceGenerator>

#include <boost/lexical_cast.hpp>

#include "shadermanager.hpp"

namespace Shader
{

    ShaderVisitor::ShaderRequirements::ShaderRequirements()
        : mColorMaterial(false)
        , mVertexColorMode(GL_AMBIENT_AND_DIFFUSE)
        , mTexStageRequiringTangents(-1)
    {
    }

    ShaderVisitor::ShaderVisitor(ShaderManager& shaderManager, const std::string &defaultVsTemplate, const std::string &defaultFsTemplate)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mShaderManager(shaderManager)
        , mDefaultVsTemplate(defaultVsTemplate)
        , mDefaultFsTemplate(defaultFsTemplate)
    {
        mRequirements.push_back(ShaderRequirements());
    }

    void ShaderVisitor::apply(osg::Node& node)
    {
        if (node.getStateSet())
        {
            pushRequirements();
            applyStateSet(node.getStateSet());
            traverse(node);
            popRequirements();
        }
        else
            traverse(node);
    }

    void ShaderVisitor::applyStateSet(osg::StateSet* stateset)
    {
        const osg::StateSet::TextureAttributeList& texAttributes = stateset->getTextureAttributeList();
        for(unsigned int unit=0;unit<texAttributes.size();++unit)
        {
            const osg::StateAttribute *attr = stateset->getTextureAttribute(unit, osg::StateAttribute::TEXTURE);
            if (attr)
            {
                const osg::Texture* texture = attr->asTexture();
                if (texture)
                {
                    if (!texture->getName().empty())
                    {
                        mRequirements.back().mTextures[unit] = texture->getName();
                        if (texture->getName() == "normalMap")
                        {
                            mRequirements.back().mTexStageRequiringTangents = unit;
                            // normal maps are by default off since the FFP can't render them, now that we'll use shaders switch to On
                            stateset->setTextureMode(unit, GL_TEXTURE_2D, osg::StateAttribute::ON);
                        }
                    }
                    else
                        std::cerr << "ShaderVisitor encountered unknown texture " << texture << std::endl;
                }
            }
            // remove state that has no effect when rendering with shaders
            stateset->removeTextureAttribute(unit, osg::StateAttribute::TEXENV);
        }

        const osg::StateSet::AttributeList& attributes = stateset->getAttributeList();
        for (osg::StateSet::AttributeList::const_iterator it = attributes.begin(); it != attributes.end(); ++it)
        {
            if (it->first.first == osg::StateAttribute::MATERIAL)
            {
                const osg::Material* mat = static_cast<const osg::Material*>(it->second.first.get());
                mRequirements.back().mVertexColorMode = mat->getColorMode();
            }
        }
    }

    void ShaderVisitor::pushRequirements()
    {
        mRequirements.push_back(mRequirements.back());
    }

    void ShaderVisitor::popRequirements()
    {
        mRequirements.pop_back();
    }

    void ShaderVisitor::createProgram(const ShaderRequirements &reqs, osg::StateSet *stateset)
    {
        ShaderManager::DefineMap defineMap;
        const char* defaultTextures[] = { "diffuseMap", "normalMap", "emissiveMap", "darkMap", "detailMap" };
        for (unsigned int i=0; i<sizeof(defaultTextures)/sizeof(defaultTextures[0]); ++i)
        {
            defineMap[defaultTextures[i]] = "0";
            defineMap[std::string(defaultTextures[i]) + std::string("UV")] = "0";
        }
        for (std::map<int, std::string>::const_iterator texIt = reqs.mTextures.begin(); texIt != reqs.mTextures.end(); ++texIt)
        {
            defineMap[texIt->second] = "1";
            defineMap[texIt->second + std::string("UV")] = boost::lexical_cast<std::string>(texIt->first);
        }

        if (!reqs.mColorMaterial)
            defineMap["colorMode"] = "0";
        else
        {
            switch (reqs.mVertexColorMode)
            {
            default:
            case GL_AMBIENT_AND_DIFFUSE:
                defineMap["colorMode"] = "2";
                break;
            case GL_AMBIENT:
                defineMap["colorMode"] = "1";
                break;
            }
        }

        osg::ref_ptr<osg::Shader> vertexShader (mShaderManager.getShader(mDefaultVsTemplate, defineMap, osg::Shader::VERTEX));
        osg::ref_ptr<osg::Shader> fragmentShader (mShaderManager.getShader(mDefaultFsTemplate, defineMap, osg::Shader::FRAGMENT));

        if (vertexShader && fragmentShader)
        {
            osg::ref_ptr<osg::Program> program (new osg::Program);
            program->addShader(vertexShader);
            program->addShader(fragmentShader);

            stateset->setAttributeAndModes(program, osg::StateAttribute::ON);

            for (std::map<int, std::string>::const_iterator texIt = reqs.mTextures.begin(); texIt != reqs.mTextures.end(); ++texIt)
            {
                stateset->addUniform(new osg::Uniform(texIt->second.c_str(), texIt->first), osg::StateAttribute::ON);
            }
        }
    }

    void ShaderVisitor::apply(osg::Geometry& geometry)
    {
        bool needPop = (geometry.getStateSet() != NULL);
        if (geometry.getStateSet())
        {
            pushRequirements();
            applyStateSet(geometry.getStateSet());
        }

        if (!mRequirements.empty())
        {
            const ShaderRequirements& reqs = mRequirements.back();
            if (reqs.mTexStageRequiringTangents != -1)
            {
                osg::ref_ptr<osgUtil::TangentSpaceGenerator> generator (new osgUtil::TangentSpaceGenerator);
                generator->generate(&geometry, reqs.mTexStageRequiringTangents);

                geometry.setTexCoordArray(7, generator->getTangentArray(), osg::Array::BIND_PER_VERTEX);
            }

            // TODO: find a better place for the stateset
            createProgram(reqs, geometry.getOrCreateStateSet());
        }

        if (needPop)
            popRequirements();
    }

    void ShaderVisitor::apply(osg::Drawable& drawable)
    {
        // non-Geometry drawable (e.g. particle system)
        bool needPop = (drawable.getStateSet() != NULL);

        if (drawable.getStateSet())
        {
            pushRequirements();
            applyStateSet(drawable.getStateSet());
        }

        if (!mRequirements.empty())
        {
            // TODO: find a better place for the stateset
            createProgram(mRequirements.back(), drawable.getOrCreateStateSet());
        }

        if (needPop)
            popRequirements();
    }

}
