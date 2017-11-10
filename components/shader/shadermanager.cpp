#include "shadermanager.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

#include <osg/Program>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include "components/sceneutil/shadow.hpp"

namespace Shader
{

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    bool parseIncludes(boost::filesystem::path shaderPath, std::string& source)
    {
        boost::replace_all(source, "\r\n", "\n");

        std::set<boost::filesystem::path> includedFiles;
        size_t foundPos = 0;
        int fileNumber = 1;
        while ((foundPos = source.find("#include")) != std::string::npos)
        {
            size_t start = source.find('"', foundPos);
            if (start == std::string::npos || start == source.size()-1)
            {
                std::cerr << "Invalid #include " << std::endl;
                return false;
            }
            size_t end = source.find('"', start+1);
            if (end == std::string::npos)
            {
                std::cerr << "Invalid #include " << std::endl;
                return false;
            }
            std::string includeFilename = source.substr(start+1, end-(start+1));
            boost::filesystem::path includePath = shaderPath / includeFilename;
            boost::filesystem::ifstream includeFstream;
            includeFstream.open(includePath);
            if (includeFstream.fail())
            {
                std::cerr << "Failed to open " << includePath.string() << std::endl;
                return false;
            }

            std::stringstream buffer;
            buffer << includeFstream.rdbuf();

            // insert #line directives so we get correct line numbers in compiler errors
            int includedFileNumber = fileNumber++;

            int lineNumber = std::count(source.begin(), source.begin() + foundPos, '\n');

            std::stringstream toInsert;
            toInsert << "#line 0 " << includedFileNumber << "\n" << buffer.str() << "\n#line " << lineNumber << " 0\n";

            source.replace(foundPos, (end-foundPos+1), toInsert.str());

            if (includedFiles.insert(includePath).second == false)
            {
                std::cerr << "Detected cyclic #includes" << std::endl;
                return false;
            }
        }
        return true;
    }

    bool parseFors(std::string& source)
    {
        const char escapeCharacter = '$';
        size_t foundPos = 0;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                std::cerr << "Unexpected EOF" << std::endl;
                return false;
            }
            std::string command = source.substr(foundPos + 1, endPos - (foundPos + 1));
            if (command != "foreach")
            {
                std::cerr << "Unknown shader directive: $" << command << std::endl;
                return false;
            }

            size_t iterNameStart = endPos + 1;
            size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
            if (iterNameEnd == std::string::npos)
            {
                std::cerr << "Unexpected EOF" << std::endl;
                return false;
            }
            std::string iteratorName = "$" + source.substr(iterNameStart, iterNameEnd - iterNameStart);

            size_t listStart = iterNameEnd + 1;
            size_t listEnd = source.find_first_of("\n\r", listStart);
            if (listEnd == std::string::npos)
            {
                std::cerr << "Unexpected EOF" << std::endl;
                return false;
            }
            std::string list = source.substr(listStart, listEnd - listStart);
            std::vector<std::string> listElements;
            boost::split(listElements, list, boost::is_any_of(","));

            size_t contentStart = source.find_first_not_of("\n\r", listEnd);
            size_t contentEnd = source.find("$endforeach", contentStart);
            if (contentEnd == std::string::npos)
            {
                std::cerr << "Unexpected EOF" << std::endl;
                return false;
            }
            std::string content = source.substr(contentStart, contentEnd - contentStart);

            size_t overallEnd = contentEnd + std::string("$endforeach").length();
            // This will be wrong if there are other #line directives, so that needs fixing
            int lineNumber = std::count(source.begin(), source.begin() + overallEnd, '\n') + 2;

            std::string replacement = "";
            for (std::vector<std::string>::const_iterator element = listElements.cbegin(); element != listElements.cend(); element++)
            {
                std::string contentInstance = content;
                size_t foundIterator;
                while ((foundIterator = contentInstance.find(iteratorName)) != std::string::npos)
                    contentInstance.replace(foundIterator, iteratorName.length(), *element);
                replacement += contentInstance;
            }
            replacement += "\n#line " + std::to_string(lineNumber);
            source.replace(foundPos, overallEnd - foundPos, replacement);
        }

        return true;
    }

    bool parseDefines(std::string& source, const ShaderManager::DefineMap& defines)
    {
        const char escapeCharacter = '@';
        size_t foundPos = 0;
        std::vector<std::string> forIterators;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                std::cerr << "Unexpected EOF" << std::endl;
                return false;
            }
            std::string define = source.substr(foundPos+1, endPos - (foundPos+1));
            ShaderManager::DefineMap::const_iterator defineFound = defines.find(define);
            if (define == "foreach")
            {
                source.replace(foundPos, 1, "$");
                size_t iterNameStart = endPos + 1;
                size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
                if (iterNameEnd == std::string::npos)
                {
                    std::cerr << "Unexpected EOF" << std::endl;
                    return false;
                }
                forIterators.push_back(source.substr(iterNameStart, iterNameEnd - iterNameStart));
            }
            else if (define == "endforeach")
            {
                source.replace(foundPos, 1, "$");
                if (forIterators.empty())
                {
                    std::cerr << "endforeach without foreach" << std::endl;
                    return false;
                }
                else
                    forIterators.pop_back();
            }
            else if (std::find(forIterators.begin(), forIterators.end(), define) != forIterators.end())
            {
                source.replace(foundPos, 1, "$");
            }
            else if (defineFound == defines.end())
            {
                std::cerr << "Undefined " << define << std::endl;
                return false;
            }
            else
            {
                source.replace(foundPos, endPos-foundPos, defineFound->second);
            }
        }
        return true;
    }

    osg::ref_ptr<osg::Shader> ShaderManager::getShader(const std::string &shaderTemplate, const ShaderManager::DefineMap &defines, osg::Shader::Type shaderType, bool disableShadows)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);

        // set up shadows in the shader
        // get these values from settings manager
        bool shadows = true & !disableShadows;
        int numShadowMaps = SceneUtil::MWShadow::numberOfShadowMapsPerLight;
        DefineMap definesWithShadows;
        if (shadows)
        {
            definesWithShadows.insert(std::make_pair(std::string("shadows_enabled"), std::string("1")));
            for (int i = 0; i < numShadowMaps; ++i)
                definesWithShadows["shadow_texture_unit_list"] += std::to_string(i) + ",";
            // remove extra comma
            definesWithShadows["shadow_texture_unit_list"] = definesWithShadows["shadow_texture_unit_list"].substr(0, definesWithShadows["shadow_texture_unit_list"].length() - 1);
        }

        definesWithShadows.insert(defines.begin(), defines.end());

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

            // parse includes
            std::string source = buffer.str();
            if (!parseIncludes(boost::filesystem::path(mPath), source))
                return NULL;

            templateIt = mShaderTemplates.insert(std::make_pair(shaderTemplate, source)).first;
        }

        ShaderMap::iterator shaderIt = mShaders.find(std::make_pair(shaderTemplate, definesWithShadows));
        if (shaderIt == mShaders.end())
        {
            std::string shaderSource = templateIt->second;
            if (!parseDefines(shaderSource, definesWithShadows) || !parseFors(shaderSource))
            {
                // Add to the cache anyway to avoid logging the same error over and over.
                mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, defines), nullptr));
                return NULL;
            }

            osg::ref_ptr<osg::Shader> shader (new osg::Shader(shaderType));
            shader->setShaderSource(shaderSource);
            // Assign a unique name to allow the SharedStateManager to compare shaders efficiently
            static unsigned int counter = 0;
            shader->setName(std::to_string(counter++));

            shaderIt = mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, definesWithShadows), shader)).first;
        }
        return shaderIt->second;
    }

    osg::ref_ptr<osg::Program> ShaderManager::getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        ProgramMap::iterator found = mPrograms.find(std::make_pair(vertexShader, fragmentShader));
        if (found == mPrograms.end())
        {
            osg::ref_ptr<osg::Program> program (new osg::Program);
            program->addShader(vertexShader);
            program->addShader(fragmentShader);
            found = mPrograms.insert(std::make_pair(std::make_pair(vertexShader, fragmentShader), program)).first;
        }
        return found->second;
    }

    void ShaderManager::releaseGLObjects(osg::State *state)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        for (auto shader : mShaders)
            shader.second->releaseGLObjects(state);
        for (auto program : mPrograms)
            program.second->releaseGLObjects(state);
    }

}
