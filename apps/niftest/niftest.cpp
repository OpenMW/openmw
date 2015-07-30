///Program to test .nif files both on the FileSystem and in BSA archives.

#include <iostream>
#include <fstream>

#include <components/nif/niffile.hpp>
#include <components/files/constrainedfilestream.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/bsaarchive.hpp>


///See if the file has the named extension
bool hasExtension(std::string filename, std::string  extensionToFind)
{
    std::string extension = filename.substr(filename.find_last_of(".")+1);

    //Convert strings to lower case for comparison
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    std::transform(extensionToFind.begin(), extensionToFind.end(), extensionToFind.begin(), ::tolower);

    if(extension == extensionToFind)
        return true;
    else
        return false;
}

///See if the file has the "nif" extension.
bool isNIF(std::string filename)
{
    return hasExtension(filename,"nif");
}
///See if the file has the "bsa" extension.
bool isBSA(std::string filename)
{
    return hasExtension(filename,"bsa");
}

///Check all the nif files in the given BSA archive
void readBSA(std::string filename)
{
    VFS::Manager myManager(false);
    myManager.addArchive(new VFS::BsaArchive(filename));
    myManager.buildIndex();

    std::map<std::string, VFS::File*> files=myManager.getIndex();
    for(std::map<std::string, VFS::File*>::const_iterator it=files.begin(); it!=files.end(); ++it)
    {
      std::string name = it->first;
      if(isNIF(name))
      {
//           std::cout << "Decoding: " << name << std::endl;
          Nif::NIFFile temp_nif(myManager.get(name),name);
      }
    }
}

int main(int argc, char **argv)
{

    std::cout << "Reading Files" << std::endl;
     for(int i = 1; i<argc;i++)
     {
         std::string name = argv[i];

        try{
            if(isNIF(name))
            {
                //std::cout << "Decoding: " << name << std::endl;
                Nif::NIFFile temp_nif(Files::openConstrainedFileStream(name.c_str()),name);
             }
             else if(isBSA(name))
             {
                std::cout << "Reading BSA File: " << name << std::endl;
                readBSA(name);
             }
             else
             {
                 std::cerr << "ERROR:  \"" << name << "\" is not a nif or bsa file!" << std::endl;
             }
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR, an exception has occurred:  " << e.what() << std::endl;
        }
     }
     return 0;
}
