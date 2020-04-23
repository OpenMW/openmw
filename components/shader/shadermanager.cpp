#include "shadermanager.hpp"

#include <fstream>
#include <algorithm>
#include <sstream>

#include <osg/Program>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

namespace Shader
{

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    bool addLineDirectivesAfterConditionalBlocks(std::string& source)
    {
        for (size_t position = 0; position < source.length(); )
        {
            size_t foundPos = source.find("#endif", position);
            foundPos = std::min(foundPos, source.find("#elif", position));
            foundPos = std::min(foundPos, source.find("#else", position));

            if (foundPos == std::string::npos)
                break;

            foundPos = source.find_first_of("\n\r", foundPos);
            foundPos = source.find_first_not_of("\n\r", foundPos);

            size_t lineDirectivePosition = source.rfind("#line", foundPos);
            int lineNumber;
            if (lineDirectivePosition != std::string::npos)
            {
                size_t lineNumberStart = lineDirectivePosition + std::string("#line ").length();
                size_t lineNumberEnd = source.find_first_not_of("0123456789", lineNumberStart);
                std::string lineNumberString = source.substr(lineNumberStart, lineNumberEnd - lineNumberStart);
                lineNumber = std::stoi(lineNumberString) - 1;
            }
            else
            {
                lineDirectivePosition = 0;
                lineNumber = 1;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + foundPos, '\n');

            source.replace(foundPos, 0, "#line " + std::to_string(lineNumber) + "\n");

            position = foundPos;
        }

        return true;
    }

    bool parseIncludes(boost::filesystem::path shaderPath, std::string& source, const std::string& shaderTemplate)
    {
        Misc::StringUtils::replaceAll(source, "\r\n", "\n");

        std::set<boost::filesystem::path> includedFiles;
        size_t foundPos = 0;
        int fileNumber = 1;
        while ((foundPos = source.find("#include")) != std::string::npos)
        {
            size_t start = source.find('"', foundPos);
            if (start == std::string::npos || start == source.size()-1)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Invalid #include";
                return false;
            }
            size_t end = source.find('"', start+1);
            if (end == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Invalid #include";
                return false;
            }
            std::string includeFilename = source.substr(start+1, end-(start+1));
            boost::filesystem::path includePath = shaderPath / includeFilename;
            boost::filesystem::ifstream includeFstream;
            includeFstream.open(includePath);
            if (includeFstream.fail())
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Failed to open include " << includePath.string();
                return false;
            }

            std::stringstream buffer;
            buffer << includeFstream.rdbuf();
            std::string stringRepresentation = buffer.str();
            addLineDirectivesAfterConditionalBlocks(stringRepresentation);

            // insert #line directives so we get correct line numbers in compiler errors
            int includedFileNumber = fileNumber++;

            size_t lineDirectivePosition = source.rfind("#line", foundPos);
            int lineNumber;
            if (lineDirectivePosition != std::string::npos)
            {
                size_t lineNumberStart = lineDirectivePosition + std::string("#line ").length();
                size_t lineNumberEnd = source.find_first_not_of("0123456789", lineNumberStart);
                std::string lineNumberString = source.substr(lineNumberStart, lineNumberEnd - lineNumberStart);
                lineNumber = std::stoi(lineNumberString) - 1;
            }
            else
            {
                lineDirectivePosition = 0;
                lineNumber = 1;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + foundPos, '\n');

            std::stringstream toInsert;
            toInsert << "#line 0 " << includedFileNumber << "\n" << stringRepresentation << "\n#line " << lineNumber << " 0\n";

            source.replace(foundPos, (end-foundPos+1), toInsert.str());

            if (includedFiles.insert(includePath).second == false)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Detected cyclic #includes";
                return false;
            }
        }
        return true;
    }

    bool parseFors(std::string& source, const std::string& shaderTemplate)
    {
        const char escapeCharacter = '$';
        size_t foundPos = 0;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unexpected EOF";
                return false;
            }
            std::string command = source.substr(foundPos + 1, endPos - (foundPos + 1));
            if (command != "foreach")
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unknown shader directive: $" << command;
                return false;
            }

            size_t iterNameStart = endPos + 1;
            size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
            if (iterNameEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unexpected EOF";
                return false;
            }
            std::string iteratorName = "$" + source.substr(iterNameStart, iterNameEnd - iterNameStart);

            size_t listStart = iterNameEnd + 1;
            size_t listEnd = source.find_first_of("\n\r", listStart);
            if (listEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unexpected EOF";
                return false;
            }
            std::string list = source.substr(listStart, listEnd - listStart);
            std::vector<std::string> listElements;
            if (list != "")
                Misc::StringUtils::split (list, listElements, ",");

            size_t contentStart = source.find_first_not_of("\n\r", listEnd);
            size_t contentEnd = source.find("$endforeach", contentStart);
            if (contentEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unexpected EOF";
                return false;
            }
            std::string content = source.substr(contentStart, contentEnd - contentStart);

            size_t overallEnd = contentEnd + std::string("$endforeach").length();

            size_t lineDirectivePosition = source.rfind("#line", overallEnd);
            int lineNumber;
            if (lineDirectivePosition != std::string::npos)
            {
                size_t lineNumberStart = lineDirectivePosition + std::string("#line ").length();
                size_t lineNumberEnd = source.find_first_not_of("0123456789", lineNumberStart);
                std::string lineNumberString = source.substr(lineNumberStart, lineNumberEnd - lineNumberStart);
                lineNumber = std::stoi(lineNumberString);
            }
            else
            {
                lineDirectivePosition = 0;
                lineNumber = 2;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + overallEnd, '\n');

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

    bool parseDefines(std::string& source, const ShaderManager::DefineMap& defines,
        const ShaderManager::DefineMap& globalDefines, const std::string& shaderTemplate)
    {
        const char escapeCharacter = '@';
        size_t foundPos = 0;
        std::vector<std::string> forIterators;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unexpected EOF";
                return false;
            }
            std::string define = source.substr(foundPos+1, endPos - (foundPos+1));
            ShaderManager::DefineMap::const_iterator defineFound = defines.find(define);
            ShaderManager::DefineMap::const_iterator globalDefineFound = globalDefines.find(define);
            if (define == "foreach")
            {
                source.replace(foundPos, 1, "$");
                size_t iterNameStart = endPos + 1;
                size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
                if (iterNameEnd == std::string::npos)
                {
                    Log(Debug::Error) << "Shader " << shaderTemplate << " error: Unexpected EOF";
                    return false;
                }
                forIterators.push_back(source.substr(iterNameStart, iterNameEnd - iterNameStart));
            }
            else if (define == "endforeach")
            {
                source.replace(foundPos, 1, "$");
                if (forIterators.empty())
                {
                    Log(Debug::Error) << "Shader " << shaderTemplate << " error: endforeach without foreach";
                    return false;
                }
                else
                    forIterators.pop_back();
            }
            else if (std::find(forIterators.begin(), forIterators.end(), define) != forIterators.end())
            {
                source.replace(foundPos, 1, "$");
            }
            else if (defineFound != defines.end())
            {
                source.replace(foundPos, endPos - foundPos, defineFound->second);
            }
            else if (globalDefineFound != globalDefines.end())
            {
                source.replace(foundPos, endPos - foundPos, globalDefineFound->second);
            }
            else
            {
                Log(Debug::Error) << "Shader " << shaderTemplate << " error: Undefined " << define;
                return false;
            }
        }
        return true;
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
                Log(Debug::Error) << "Failed to open " << p.string();
                return nullptr;
            }
            std::stringstream buffer;
            buffer << stream.rdbuf();

            // parse includes
            std::string source = buffer.str();
            if (!addLineDirectivesAfterConditionalBlocks(source)
                    || !parseIncludes(boost::filesystem::path(mPath), source, shaderTemplate))
                return nullptr;

            templateIt = mShaderTemplates.insert(std::make_pair(shaderTemplate, source)).first;
        }

        ShaderMap::iterator shaderIt = mShaders.find(std::make_pair(shaderTemplate, defines));
        if (shaderIt == mShaders.end())
        {
            std::string shaderSource = templateIt->second;
            if (!parseDefines(shaderSource, defines, mGlobalDefines, shaderTemplate) || !parseFors(shaderSource, shaderTemplate))
            {
                // Add to the cache anyway to avoid logging the same error over and over.
                mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, defines), nullptr));
                return nullptr;
            }

            osg::ref_ptr<osg::Shader> shader (new osg::Shader(shaderType));
            shader->setShaderSource(shaderSource);
            // Assign a unique name to allow the SharedStateManager to compare shaders efficiently
            static unsigned int counter = 0;
            shader->setName(std::to_string(counter++));

            shaderIt = mShaders.insert(std::make_pair(std::make_pair(shaderTemplate, defines), shader)).first;
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

    ShaderManager::DefineMap ShaderManager::getGlobalDefines()
    {
        return DefineMap(mGlobalDefines);
    }

    void ShaderManager::setGlobalDefines(DefineMap & globalDefines)
    {
        mGlobalDefines = globalDefines;
        for (auto shaderMapElement: mShaders)
        {
            std::string templateId = shaderMapElement.first.first;
            ShaderManager::DefineMap defines = shaderMapElement.first.second;
            osg::ref_ptr<osg::Shader> shader = shaderMapElement.second;
            if (shader == nullptr)
                // I'm not sure how to handle a shader that was already broken as there's no way to get a potential replacement to the nodes that need it.
                continue;
            std::string shaderSource = mShaderTemplates[templateId];
            if (!parseDefines(shaderSource, defines, mGlobalDefines, templateId) || !parseFors(shaderSource, templateId))
                // We just broke the shader and there's no way to force existing objects back to fixed-function mode as we would when creating the shader.
                // If we put a nullptr in the shader map, we just lose the ability to put a working one in later.
                continue;
            shader->setShaderSource(shaderSource);
        }
    }

    void ShaderManager::releaseGLObjects(osg::State *state)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mMutex);
        for (auto shader : mShaders)
        {
            if (shader.second != nullptr)
                shader.second->releaseGLObjects(state);
        }
        for (auto program : mPrograms)
            program.second->releaseGLObjects(state);
    }

}
