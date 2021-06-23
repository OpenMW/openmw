#include "shadermanager.hpp"

#include <fstream>
#include <algorithm>
#include <sstream>

#include <osg/Program>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/sceneutil/lightmanager.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>

namespace Shader
{

    ShaderManager::ShaderManager()
        : mLightingMethod(SceneUtil::LightingMethod::FFP)
    {
    }

    void ShaderManager::setShaderPath(const std::string &path)
    {
        mPath = path;
    }

    void ShaderManager::setLightingMethod(SceneUtil::LightingMethod method)
    {
        mLightingMethod = method;
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

            if (foundPos == std::string::npos)
                break;

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

    // Recursively replaces include statements with the actual source of the included files.
    // Adjusts #line statements accordingly and detects cyclic includes.
    // includingFiles is the set of files that include this file directly or indirectly, and is intentionally not a reference to allow automatic cleanup.
    static bool parseIncludes(const boost::filesystem::path& shaderPath, std::string& source, const std::string& fileName, int& fileNumber, std::set<boost::filesystem::path> includingFiles)
    {
        // An include is cyclic if it is being included by itself
        if (includingFiles.insert(shaderPath/fileName).second == false)
        {
            Log(Debug::Error) << "Shader " << fileName << " error: Detected cyclic #includes";
            return false;
        }

        Misc::StringUtils::replaceAll(source, "\r\n", "\n");

        size_t foundPos = 0;
        while ((foundPos = source.find("#include")) != std::string::npos)
        {
            size_t start = source.find('"', foundPos);
            if (start == std::string::npos || start == source.size() - 1)
            {
                Log(Debug::Error) << "Shader " << fileName << " error: Invalid #include";
                return false;
            }
            size_t end = source.find('"', start + 1);
            if (end == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << fileName << " error: Invalid #include";
                return false;
            }
            std::string includeFilename = source.substr(start + 1, end - (start + 1));
            boost::filesystem::path includePath = shaderPath / includeFilename;

            // Determine the line number that will be used for the #line directive following the included source
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
                lineNumber = 0;
            }
            lineNumber += std::count(source.begin() + lineDirectivePosition, source.begin() + foundPos, '\n');

            // Include the file recursively
            boost::filesystem::ifstream includeFstream;
            includeFstream.open(includePath);
            if (includeFstream.fail())
            {
                Log(Debug::Error) << "Shader " << fileName << " error: Failed to open include " << includePath.string();
                return false;
            }
            int includedFileNumber = fileNumber++;

            std::stringstream buffer;
            buffer << includeFstream.rdbuf();
            std::string stringRepresentation = buffer.str();
            if (!addLineDirectivesAfterConditionalBlocks(stringRepresentation)
                || !parseIncludes(shaderPath, stringRepresentation, includeFilename, fileNumber, includingFiles))
            {
                Log(Debug::Error) << "In file included from " << fileName << "." << lineNumber;
                return false;
            }

            std::stringstream toInsert;
            toInsert << "#line 0 " << includedFileNumber << "\n" << stringRepresentation << "\n#line " << lineNumber << " 0\n";

            source.replace(foundPos, (end - foundPos + 1), toInsert.str());
        }
        return true;
    }

    bool parseFors(std::string& source, const std::string& templateName)
    {
        const char escapeCharacter = '$';
        size_t foundPos = 0;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string command = source.substr(foundPos + 1, endPos - (foundPos + 1));
            if (command != "foreach")
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unknown shader directive: $" << command;
                return false;
            }

            size_t iterNameStart = endPos + 1;
            size_t iterNameEnd = source.find_first_of(" \n\r()[].;,", iterNameStart);
            if (iterNameEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                return false;
            }
            std::string iteratorName = "$" + source.substr(iterNameStart, iterNameEnd - iterNameStart);

            size_t listStart = iterNameEnd + 1;
            size_t listEnd = source.find_first_of("\n\r", listStart);
            if (listEnd == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
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
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
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
        const ShaderManager::DefineMap& globalDefines, const std::string& templateName)
    {
        const char escapeCharacter = '@';
        size_t foundPos = 0;
        std::vector<std::string> forIterators;
        while ((foundPos = source.find(escapeCharacter)) != std::string::npos)
        {
            size_t endPos = source.find_first_of(" \n\r()[].;,", foundPos);
            if (endPos == std::string::npos)
            {
                Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
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
                    Log(Debug::Error) << "Shader " << templateName << " error: Unexpected EOF";
                    return false;
                }
                forIterators.push_back(source.substr(iterNameStart, iterNameEnd - iterNameStart));
            }
            else if (define == "endforeach")
            {
                source.replace(foundPos, 1, "$");
                if (forIterators.empty())
                {
                    Log(Debug::Error) << "Shader " << templateName << " error: endforeach without foreach";
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
                Log(Debug::Error) << "Shader " << templateName << " error: Undefined " << define;
                return false;
            }
        }
        return true;
    }

    osg::ref_ptr<osg::Shader> ShaderManager::getShader(const std::string &templateName, const ShaderManager::DefineMap &defines, osg::Shader::Type shaderType)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        // read the template if we haven't already
        TemplateMap::iterator templateIt = mShaderTemplates.find(templateName);
        if (templateIt == mShaderTemplates.end())
        {
            boost::filesystem::path path = (boost::filesystem::path(mPath) / templateName);
            boost::filesystem::ifstream stream;
            stream.open(path);
            if (stream.fail())
            {
                Log(Debug::Error) << "Failed to open " << path.string();
                return nullptr;
            }
            std::stringstream buffer;
            buffer << stream.rdbuf();

            // parse includes
            int fileNumber = 1;
            std::string source = buffer.str();
            if (!addLineDirectivesAfterConditionalBlocks(source)
                || !parseIncludes(boost::filesystem::path(mPath), source, templateName, fileNumber, {}))
                return nullptr;

            templateIt = mShaderTemplates.insert(std::make_pair(templateName, source)).first;
        }

        ShaderMap::iterator shaderIt = mShaders.find(std::make_pair(templateName, defines));
        if (shaderIt == mShaders.end())
        {
            std::string shaderSource = templateIt->second;
            if (!parseDefines(shaderSource, defines, mGlobalDefines, templateName) || !parseFors(shaderSource, templateName))
            {
                // Add to the cache anyway to avoid logging the same error over and over.
                mShaders.insert(std::make_pair(std::make_pair(templateName, defines), nullptr));
                return nullptr;
            }

            osg::ref_ptr<osg::Shader> shader (new osg::Shader(shaderType));
            shader->setShaderSource(shaderSource);
            // Assign a unique prefix to allow the SharedStateManager to compare shaders efficiently.
            // Append shader source filename for debugging.
            static unsigned int counter = 0;
            shader->setName(Misc::StringUtils::format("%u %s", counter++, templateName));

            shaderIt = mShaders.insert(std::make_pair(std::make_pair(templateName, defines), shader)).first;
        }
        return shaderIt->second;
    }

    osg::ref_ptr<osg::Program> ShaderManager::getProgram(osg::ref_ptr<osg::Shader> vertexShader, osg::ref_ptr<osg::Shader> fragmentShader)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        ProgramMap::iterator found = mPrograms.find(std::make_pair(vertexShader, fragmentShader));
        if (found == mPrograms.end())
        {
            osg::ref_ptr<osg::Program> program (new osg::Program);
            program->addShader(vertexShader);
            program->addShader(fragmentShader);
            program->addBindAttribLocation("aOffset", 6);
            program->addBindAttribLocation("aRotation", 7);
            if (mLightingMethod == SceneUtil::LightingMethod::SingleUBO)
                program->addBindUniformBlock("LightBufferBinding", static_cast<int>(UBOBinding::LightBuffer));
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
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto shader : mShaders)
        {
            if (shader.second != nullptr)
                shader.second->releaseGLObjects(state);
        }
        for (auto program : mPrograms)
            program.second->releaseGLObjects(state);
    }

}
