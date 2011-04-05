NOTE: This README is for ardekantur's Mac branch of OpenMW. A README
for the main branch has yet to be written. If you want to submit one,
please send me a message!

OpenMW
======

From the [official website][]:

> OpenMW is an attempt to reimplement the popular role playing game
  Morrowind. It aims to be a fully playable, open source
  implementation of the game. You must own Morrowind to use OpenMW.


About This Project
------------------

This specific repository is a branch of OpenMW intended to keep pace
with development of the project in order to provide a Mac build for
interested parties to contribute. This is not an official, sanctioned
branch of the OpenMW project. I will only be able to answer specific
questions about getting this project running on Mac OS X, **no other
platform**. I will not even be able to guarantee my changes maintain
backwards compatibility against builds in other operating systems. You
have been warned.


Getting OpenMW Working
----------------------

1. Clone this repository.
2. Note about libs: I prefer not to install them globally (i. e. in /usr/local/), so I installing them in directory in my home directory. If OpenMW sources is in $HOME/path/openmw, I'm using $HOME/path/libs/root as prefix for boost and other libs.
  It's useful to create env var for lib install prefix:
        $ export OMW_LIB_PREFIX=$HOME/path/libs/root

3. First of all, set for current terminal some env vars:
        $ export CFLAGS="-arch i386"
        $ export CXXFLAGS="-arch i386"
        $ export LDFLAGS="-arch i386"
  All libs will build with correct architecture.
  If you close your terminal, you should set env vars again before pcoceeding to next steps!

4. Download [boost][] (tested with 1.45) and install it with the following command:

        $ cd /path/to/boost/source
        $ ./bootstrap.sh --prefix=$OMW_LIB_PREFIX
        $ ./bjam --build-dir=build --layout=versioned \
        --toolset=darwin architecture=x86 address-model=32 \
        --link-shared,static --prefix=$OMW_LIB_PREFIX install


5. Download [Ogre][] SDK (tested with 1.7.2) and move `lib/Release/Ogre.framework` into
  `Library/Frameworks`.

6. Download [OIS][] and use the XCode project provided in
   `ois/Mac/XCode-2.2`. Be sure to set your build architecture to
   `i386` and your SDK platform to either 10.5 or 10.6. Once it
   builds, move `ois/Mac/XCode-2.2/build/Debug/OIS.framework` to
   `/Library/Frameworks`.

7. Download [mpg123][] and build it:
        $ cd /path/to/mpg123/source
        $ ./configure --prefix=$OMW_LIB_PREFIX --disable-debug \
        --disable-dependency-tracking \
        --with-optimization=4 \
        --with-audio=coreaudio \
        --with-default-audio=coreaudio \
        --with-cpu=sse_alone \
        $ make install

8. Download [libsndfile][] and build it:
        $ cd /path/to/libsndfile/source
        $ ./configure --prefix=$OMW_LIB_PREFIX \
        --disable-dependency-tracking
        $ make install

9. Download [Bullet][] and build it:
        $ cd /path/to/bullet/source
        $ mkdir build
        $ cd build
        $ cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$OMW_LIB_PREFIX \
        -DBUILD_EXTRAS=OFF \
        -DBUILD_DEMOS=OFF \
        -DCMAKE_OSX_ARCHITECTURES=i386 \
        -DCMAKE_INSTALL_NAME_DIR=$OMW_LIB_RPEFIX/lib \
        -G"Unix Makefiles" ../
        $ make install

10. Generate the Makefile for OpenMW as follows and build OpenMW:
        $ mkdir /path/to/openmw/build/dir
        $ cd /path/to/open/build/dir
        $ cmake \
        -D CMAKE_OSX_ARCHITECTURES=i386 \
        -D BOOST_INCLUDEDIR=$OMW_LIB_PREFIX/include/boost-1_45 \
        -D BOOST_LIBRARYDIR=$OMW_LIB_PREFIX/lib \
        -D SNDFILE_INCLUDE_DIR=$OMW_LIB_PREFIX/include \
        -D SNDFILE_LIBRARY=$OMW_LIB_PREFIX/lib/libsndfile.a \
        -D MPG123_LIBRARY=$OMW_LIB_PREFIX/lib/libmpg123.a \
        -D MPG123_INCLUDE_DIR=$OMW_LIB_PREFIX/include \
        -D BULLET_DYNAMICS_LIBRARY=$OMW_LIB_PREFIX/lib/libBulletDynamics.a \
        -D BULLET_COLLISION_LIBRARY=$OMW_LIB_PREFIX/lib/libBulletCollision.a \
        -D BULLET_MATH_LIBRARY=$OMW_LIB_PREFIX/lib/libLinearMath.a \
        -D BULLET_SOFTBODY_LIBRARY=$OMW_LIB_PREFIX/lib/libBulletSoftBody.a \
        -D BULLET_INCLUDE_DIR=$OMW_LIB_PREFIX/include/bullet/ \
        -G "Unix Makefiles" /path/to/openmw/source/dir
        $ make
    You can use -G"Xcode" if you prefer Xcode, or -G"Eclipse CDT4 - Unix Makefiles"
    if you prefer Eclipse. You also can specify -D CMAKE_BUILD_TYPE=Debug for debug
    build.
        
11. In build directory create directory for game resources:
        $ cd /path/to/openmw/build/dir
        $ mkdir Contents
        $ mkdir Contents/Resources
        $ mkdir Contents/Plugins
    Copy Ogre plugins from Ogre SDK to Plugins subdir:
        $ cp /path/to/ogre/sdk/lib/*.dylib Contents/Plugins
    Create symlink to resources subdirectory:
        $ ln -s resources Contents/Resources/resources
    Create symlinks for *.cfg files:
        $ ln -s plugins.cfg Contents/MacOS/plugins.cfg
        $ ln -s openmw.cfg Contents/MacOS/openmw.cfg

12. Move your Morrowind `Data Files` directory into the `Contents/Resources`
   with the name `data` or create symlink:
        $ ln -s /path/to/morrowind/data/files Contents/Resources/data

13. From your build directory run:
        $ ./openmw
  Enjoy!

14. Optionally you can create .app bundle:
        $ make package
  But for now you shold manually copy Contents directory from build directory to bundle
  (because there is no plugins and resources in generated .app).
   

[boost]: http://www.boost.org
[Ogre]: http://www.ogre3d.org
[Bullet]: http://bulletphysics.org
[OIS]: http://wgois.sf.net
[mpg123]: http://www.mpg123.de
[libsndfile]: http://www.mega-nerd.com/libsndfile
[official website]: http://openmw.com
[Will Thimbleby's Ogre Framework]: http://www.thimbleby.net/ogre/
