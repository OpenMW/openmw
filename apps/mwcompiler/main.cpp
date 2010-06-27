// Stand-alone MW-script compiler

#include <exception>
#include <iostream>
#include <fstream>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/compiler/scanner.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/compiler/context.hpp>

int main (int argc, char **argv)
{
    try
    {
        Compiler::Context context;
        Compiler::StreamErrorHandler errorHandler (std::cout);
        Compiler::FileParser parser (errorHandler, context);
        
        std::ifstream file (argc>1 ? argv[1] : "test.mwscript");
        Compiler::Scanner scanner (errorHandler, file);
        
        scanner.scan (parser);
    }
    catch (const std::exception &e)
    {
        std::cout << "\nERROR: " << e.what() << std::endl;
        return 1;
    }
}

