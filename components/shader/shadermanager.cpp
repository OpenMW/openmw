#include "shadermanager.hpp"

#include <fstream>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <OpenThreads/ScopedLock>

namespace Shader
{

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    osg::ref_ptr<osg::Shader> ShaderManager::getShader(const std::string &shaderTemplate, const ShaderManager::DefineMap &defines, osg::Shader::Type shaderType)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

        // read the template if we haven't already
        TemplateMap::iterator templateIt = mShaderTemplates.find(shaderTemplate);
        if (templateIt == mShaderTemplates.end())
        {
            boost::filesystem::path p = (boost::filesystem::path(mPath) / shaderTemplate);
            boost::filesystem::ifstream stream;
            stream.open(p);
            if (stream.fail())
            {
                std::cerr << "Failed to open " << p.string() << std::endl;
                return NULL;
            }
            std::stringstream buffer;
            buffer << stream.rdbuf();

            templateIt = mShaderTemplates.insert(std::make_pair(shaderTemplate, buffer.str())).first;
        }

        ShaderMap::iterator shaderIt = mShaders.find(std::make_pair(shaderTemplate, defines));
        if (shaderIt == mShaders.end())
        {
            std::string shaderSource = templateIt->second;

            const char escapeCharacter = '@';
            size_t foundPos = 0;
            while ((foundPos = shaderSource.find(escapeCharacter)) != std::string::npos)
            {
                size_t endPos = shaderSource.find_first_of(" \n\r()[].;", foundPos);
                if (endPos == std::string::npos)
                {
                    std::cerr << "Unexpected EOF" << std::endl;
                    return NULL;
                }
                std::string define = shaderSource.substr(foundPos+1, endPos - (foundPos+1));
                DefineMap::const_iterator defineFound = defines.find(define);
                if (defineFound == defines.end())
                {
                    std::cerr << "Undefined " << define << " in shader " << shaderTemplate << std::endl;
                    return NULL;
                }
                else
                {
                    shaderSource.replace(foundPos, endPos-foundPos, defineFound->second);
                }
            }

            osg::ref_ptr<osg::Shader> shader (new osg::Shader(shaderType));
            shader->setShaderSource(shaderSource);
            // Assign a unique name to allow the SharedStateManager to compare shaders efficiently
            static unsigned int counter = 0;
            shader->setName(boost::lexical_cast<std::string>(counter++));

            shaderIt = mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, defines), shader)).first;
        }
        return shaderIt->second;
    }

}
