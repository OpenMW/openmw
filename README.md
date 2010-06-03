OpenMW
======

From the [official website][]:

> OpenMW is an attempt to reimplement the popular role playing game Morrowind. It aims to be a fully playable, open source implementation of the game. You must own Morrowind to use OpenMW.

About This Project
------------------

This specific repository is a branch of OpenMW intended to keep pace with development of the project in order to provide a Mac build for interested parties to contribute. This is not an official, sanctioned branch of the OpenMW project. I will only be able to answer specific questions about getting this project running on Mac OS X, **no other platform**. I will not even be able to guarantee my changes maintain backwards compatibility against builds in other operating systems. You have been warned.

Getting OpenMW Working
----------------------

1. Clone this repository.
2. Install `bjam` through MacPorts.
3. Download [boost][] 1.43 and install it with the following command:
    
        $ mkdir build && sudo bjam --build-dir=build --layout=versioned --toolset=darwin --architecture=combined --address-model=32 --link=shared,static install

4. Download [Ogre][] 1.7.1 and build and Xcode project with CMake:

         $ mdkir build && cd build && BOOST_INCLUDEDIR=/usr/local/include/boost-1_43 BOOST_LIBRARYDIR=/usr/local/lib cmake -G Xcode ..

5. Once the build completes, move `lib/Release/Ogre.framework` into `/Library/Frameworks`.

6. Generate the Makefile for OpenMW as follows:

        $ mkdir build && cd build && BOOST_INCLUDEDIR=/usr/local/include/boost-1_43 BOOST_LIBRARYDIR=/usr/local/lib CMAKE_OSX_ARCHITECTURES=i386 cmake ..
        
7. Move your Morrowind `Data Files` directory into `build`, renamed as `data`.

[boost]: http://www.boost.org
[Ogre]: http://www.ogre3d.org
[official website]: http://openmw.com
[Will Thimbleby's Ogre Framework]: http://www.thimbleby.net/ogre/