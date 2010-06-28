// Stand-alone MW-script compiler

#include <exception>
#include <iostream>
#include <fstream>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/exception.hpp>

#include "context.hpp"

int main (int argc, char **argv)
{
    try
    {
        SACompiler::Context context;
        Compiler::StreamErrorHandler errorHandler (std::cout);
        Compiler::FileParser parser (errorHandler, context);
        
        try
        {
            std::string filename = argc>1 ? argv[1] : "test.mwscript";
        
            std::ifstream file (filename.c_str());
            
            if (!file.is_open())
            {
                std::cout << "can't open script file: " << filename << std::endl;
                return 1;
            }
            
            Compiler::Scanner scanner (errorHandler, file);
        
            scanner.scan (parser);
        }
        catch (const Compiler::SourceException&)
        {
            // ignore exception (problem has already been reported to the user)
        }
        
        if (errorHandler.countErrors() || errorHandler.countWarnings())
        {
            std::cout
                << errorHandler.countErrors() << " error(s), "
                << errorHandler.countWarnings() << " warning(s)" << std::endl
                << std::endl;
        }
        
        if (errorHandler.isGood())
        {
            std::cout << "parsed script: " << parser.getName() << std::endl;
            return 0;
        }
        
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cout << "\nERROR: " << e.what() << std::endl;
        return 1;
    }
}

