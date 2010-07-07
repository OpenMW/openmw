
#include "context.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace SAInterpreter
{
    Context::Context (const std::string& filename)
    {
        std::ifstream file (filename.c_str());
        
        if (!file.is_open())
            throw std::runtime_error ("can't open locals file: " + filename);
    
        std::size_t shortSize, longSize, floatSize;
        
        file >> shortSize >> longSize >> floatSize;
        
        mShorts.resize (shortSize, 0);
        mLongs.resize (longSize, 0);
        mFloats.resize (floatSize, 0.0);
        
        std::size_t size = shortSize + longSize + floatSize;
        
        mNames.resize (size);
        
        for (std::size_t i=0; i<size; ++i)
            file >> mNames[i];
    }

    int Context::getLocalShort (int index) const
    {
        assert (index>=0);
        return mShorts.at (index);
    }

    int Context::getLocalLong (int index) const
    {
        assert (index>=0);
        return mLongs.at (index);
    }
    
    float Context::getLocalFloat (int index) const
    {
        assert (index>=0);
        return mFloats.at (index);
    }
    
    void Context::setLocalShort (int index, int value)
    {
        assert (index>=0);
        mShorts.at (index) = value;
    }    

    void Context::setLocalLong (int index, int value)
    {
        assert (index>=0);
        mLongs.at (index) = value;
    }    

    void Context::setLocalFloat (int index, float value)
    {
        assert (index>=0);
        mFloats.at (index) = value;
    }    
    
    void Context::messageBox (const std::string& message,
        const std::vector<std::string>& buttons)
    {
        std::cout << "message box: " << message << std::endl;
        for (std::size_t i=0; i<buttons.size(); ++i)
            std::cout << "    button " << i << ": " << buttons[i] << std::endl;
    }
    
    bool Context::menuMode()
    {
        return false;
    }
    
    int Context::getGlobalShort (const std::string& name) const
    {
        return 0;
    }

    int Context::getGlobalLong (const std::string& name) const
    {
        return 0;
    }

    float Context::getGlobalFloat (const std::string& name) const
    {
        return 0;
    }

    void Context::setGlobalShort (const std::string& name, int value) {}

    void Context::setGlobalLong (const std::string& name, int value) {}     

    void Context::setGlobalFloat (const std::string& name, float value) {}
                    
    bool Context::isScriptRunning (const std::string& name) const
    {
        return false;
    }
    
    void Context::startScript (const std::string& name) {}
    
    void Context::stopScript (const std::string& name) {}
    
    float Context::getDistance (const std::string& name) const
    {
        return 0;
    }
    
    float Context::getSecondsPassed() const
    {
        return 0;
    }
    
    void Context::report()
    {
        std::size_t i = 0;
        
        std::cout << "local shorts:" << std::endl;
        
        for (std::vector<Interpreter::Type_Short>::const_iterator iter (mShorts.begin());
            iter!=mShorts.end(); ++iter)
            std::cout << mNames[i++] << ": " << *iter << std::endl;            
    
        std::cout << "local longs:" << std::endl;
        
        for (std::vector<Interpreter::Type_Integer>::const_iterator iter (mLongs.begin());
            iter!=mLongs.end(); ++iter)
            std::cout << mNames[i++] << ": " << *iter << std::endl;            

        std::cout << "local floats:" << std::endl;        

        for (std::vector<Interpreter::Type_Float>::const_iterator iter (mFloats.begin());
            iter!=mFloats.end(); ++iter)
            std::cout << mNames[i++] << ": " << *iter << std::endl;            
    
    }
}

