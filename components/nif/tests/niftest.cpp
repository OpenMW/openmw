/**
 * @file
 * @brief Program to test .nif files both on the FileSystem and in BSA archives.
 * @details 
 * @section LICENSE
 * OpenMW is distributed as free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 3, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * version 3 along with this program. If not, see
 * http://www.gnu.org/licenses/
 */

#include "../niffile.hpp"
#include "../../bsa/bsa_file.hpp"
#include "../../bsa/bsa_archive.hpp"
#include <OgreRoot.h>
#include <OgreResourceGroupManager.h>
#include <iostream>
#include <algorithm>
#include <exception>

using namespace Nif;

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
    Bsa::BSAFile bsa;
    bsa.open(filename.c_str());

    const Bsa::BSAFile::FileList &files = bsa.getList();
    Bsa::addBSA(filename,"Bsa Files");

    for(unsigned int i=0; i<files.size(); i++)
    {
      std::string name = files[i].name;
      if(isNIF(name))
      {
          //std::cout << "Decoding " << name << std::endl;
          Nif::NIFFile temp_nif(name);
      }
    }
}

int main(int argc, char **argv)
{

    //Need this for Ogre's getSingleton
    Ogre::Root *mRoot = new Ogre::Root("", "", "niftest.log");
    Ogre::ResourceGroupManager::getSingleton ().createResourceGroup ("Bsa Files");
    //Needed to read files from file system
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation("/", "FileSystem");
    // Initialize the resource groups:
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    std::cout << "Reading Files" << std::endl;
     for(int i = 1; i<argc;i++)
     {
         std::string name = argv[i];

        try{
            if(isNIF(name))
            {
                //std::cout << "Decoding " << name << std::endl;
                Nif::NIFFile temp_nif(name);
             }
             else if(isBSA(name))
             {
                std::cout << "Reading " << name << std::endl;
                readBSA(name);
             }
             else
             {
                 std::cerr << "ERROR:  \"" << name << "\" is not a nif or bsa file!" << std::endl;
             }
        }
        catch (std::exception& e)
        {
            std::cerr << "ERROR, an exception has occured" << e.what() << std::endl;
        }
     }
     return 0;
}
