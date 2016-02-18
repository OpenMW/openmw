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
        : mHasNormalMap(false)
        , mColorMaterial(false)
        , mVertexColorMode(GL_AMBIENT_AND_DIFFUSE)
        , mMaterialOverridden(false)
        , mTexStageRequiringTangents(-1)
    {
    }

    ShaderVisitor::ShaderVisitor(ShaderManager& shaderManager, const std::string &defaultVsTemplate, const std::string &defaultFsTemplate)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mForceShaders(false)
        , mClampLighting(false)
        , mForcePerPixelLighting(false)
        , mAllowedToModifyStateSets(true)
        , mShaderManager(shaderManager)
        , mDefaultVsTemplate(defaultVsTemplate)
        , mDefaultFsTemplate(defaultFsTemplate)
    {
        mRequirements.push_back(ShaderRequirements());
    }

    void ShaderVisitor::setForceShaders(bool force)
    {
        mForceShaders = force;
    }

    void ShaderVisitor::setClampLighting(bool clamp)
    {
        mClampLighting = clamp;
    }

    void ShaderVisitor::setForcePerPixelLighting(bool force)
    {
        mForcePerPixelLighting = force;
    }

    void ShaderVisitor::apply(osg::Node& node)
    {
        if (node.getStateSet())
        {
            pushRequirements();
            applyStateSet(node.getStateSet(), node);
            traverse(node);
            popRequirements();
        }
        else
            traverse(node);
    }

    osg::StateSet* getWritableStateSet(osg::Node& node)
    {
        if (!node.getStateSet())
            return node.getOrCreateStateSet();

        osg::ref_ptr<osg::StateSet> newStateSet = osg::clone(node.getStateSet(), osg::CopyOp::SHALLOW_COPY);
        node.setStateSet(newStateSet);
        return newStateSet.get();
    }

    void ShaderVisitor::applyStateSet(osg::ref_ptr<osg::StateSet> stateset, osg::Node& node)
    {
        osg::StateSet* writableStateSet = NULL;
        if (mAllowedToModifyStateSets)
            writableStateSet = node.getStateSet();
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
                            mRequirements.back().mHasNormalMap = true;
                            if (!writableStateSet)
                                writableStateSet = getWritableStateSet(node);
                            // normal maps are by default off since the FFP can't render them, now that we'll use shaders switch to On
                            writableStateSet->setTextureMode(unit, GL_TEXTURE_2D, osg::StateAttribute::ON);
                        }
                    }
                    else
                        std::cerr << "ShaderVisitor encountered unknown texture " << texture << std::endl;
                }
            }
            // remove state that has no effect when rendering with shaders
            if (stateset->getTextureAttribute(unit, osg::StateAttribute::TEXENV))
            {
                if (!writableStateSet)
                    writableStateSet = getWritableStateSet(node);
                writableStateSet->removeTextureAttribute(unit, osg::StateAttribute::TEXENV);
            }
        }

        const osg::StateSet::AttributeList& attributes = stateset->getAttributeList();
        for (osg::StateSet::AttributeList::const_iterator it = attributes.begin(); it != attributes.end(); ++it)
        {
            if (it->first.first == osg::StateAttribute::MATERIAL)
            {
                if (!mRequirements.back().mMaterialOverridden || it->second.second & osg::StateAttribute::PROTECTED)
                {
                    if (it->second.second & osg::StateAttribute::OVERRIDE)
                        mRequirements.back().mMaterialOverridden = true;

                    const osg::Material* mat = static_cast<const osg::Material*>(it->second.first.get());
                    mRequirements.back().mColorMaterial = (mat->getColorMode() != osg::Material::OFF);
                    mRequirements.back().mVertexColorMode = mat->getColorMode();
                }
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

    void ShaderVisitor::createProgram(const ShaderRequirements &reqs, osg::Node& node)
    {
        osg::StateSet* writableStateSet = NULL;
        if (mAllowedToModifyStateSets)
            writableStateSet = node.getOrCreateStateSet();
        else
            writableStateSet = getWritableStateSet(node);

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
            case GL_EMISSION:
                defineMap["colorMode"] = "1";
                break;
            }
        }

        defineMap["forcePPL"] = mForcePerPixelLighting ? "1" : "0";
        defineMap["clamp"] = mClampLighting ? "1" : "0";

        osg::ref_ptr<osg::Shader> vertexShader (mShaderManager.getShader(mDefaultVsTemplate, defineMap, osg::Shader::VERTEX));
        osg::ref_ptr<osg::Shader> fragmentShader (mShaderManager.getShader(mDefaultFsTemplate, defineMap, osg::Shader::FRAGMENT));

        if (vertexShader && fragmentShader)
        {
            writableStateSet->setAttributeAndModes(mShaderManager.getProgram(vertexShader, fragmentShader), osg::StateAttribute::ON);

            for (std::map<int, std::string>::const_iterator texIt = reqs.mTextures.begin(); texIt != reqs.mTextures.end(); ++texIt)
            {
                writableStateSet->addUniform(new osg::Uniform(texIt->second.c_str(), texIt->first), osg::StateAttribute::ON);
            }
        }
    }

    void ShaderVisitor::apply(osg::Geometry& geometry)
    {
        bool needPop = (geometry.getStateSet() != NULL);
        if (geometry.getStateSet())
        {
            pushRequirements();
            applyStateSet(geometry.getStateSet(), geometry);
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
            if (reqs.mHasNormalMap || mForceShaders)
                createProgram(reqs, geometry);
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
            applyStateSet(drawable.getStateSet(), drawable);
        }

        if (!mRequirements.empty())
        {
            const ShaderRequirements& reqs = mRequirements.back();
            // TODO: find a better place for the stateset
            if (reqs.mHasNormalMap || mForceShaders)
                createProgram(reqs, drawable);
        }

        if (needPop)
            popRequirements();
    }

    void ShaderVisitor::setAllowedToModifyStateSets(bool allowed)
    {
        mAllowedToModifyStateSets = allowed;
    }

}
