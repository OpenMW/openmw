// Stand-alone MW-script code interpreter

#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/context.hpp>
#include <components/interpreter/types.hpp>

int main (int argc, char **argv)
{
    try
    {
        Interpreter::Context context;
        Interpreter::Interpreter interpreter (context);
        
        std::string filename = argc>1 ? argv[1] : "test.mwscript";

        std::string codefilename = filename + ".code";

        std::ifstream codefile (codefilename.c_str());

        if (!codefile.is_open())
        {
            std::cout << "can't open code file: " << codefilename << std::endl;
            return 1;
        }        

        std::vector<Interpreter::Type_Code> code (4);
        
        codefile.read (reinterpret_cast<char *> (&code[0]), 4 * sizeof (Interpreter::Type_Code));
        
        unsigned int size = code[0] + code[1] + code[2] + code[3];

        code.resize (4+size);

        codefile.read (reinterpret_cast<char *> (&code[4]), size * sizeof (Interpreter::Type_Code));
    
        interpreter.run (&code[0], size+4);
    }
    catch (const std::exception &e)
    {
        std::cout << "\nERROR: " << e.what() << std::endl;
        return 1;
    }    
}

