#include "reader.hpp"

#include <array>
#include <stdexcept>
#include <string>

#include "file.hpp"
#include "stream.hpp"

namespace Bgsm
{
    void Reader::parse(Files::IStreamPtr&& inputStream)
    {
        BGSMStream stream(std::move(inputStream));

        std::array<char, 4> signature;
        stream.readArray(signature);
        std::string shaderType(signature.data(), 4);
        if (shaderType == "BGEM")
        {
            mFile = std::make_unique<BGEMFile>();
            mFile->mShaderType = Bgsm::ShaderType::Effect;
        }
        else if (shaderType == "BGSM")
        {
            mFile = std::make_unique<BGSMFile>();
            mFile->mShaderType = Bgsm::ShaderType::Lighting;
        }
        else
            throw std::runtime_error("Invalid material file");

        mFile->read(stream);
    }

}
