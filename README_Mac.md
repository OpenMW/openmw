#Getting OpenMW Working on OS X

## Initial setup
First of all, clone OpenMW repo.

        $ git clone github.com/zinnschlag/openmw

Or use your github url if you forked.

About dependencies: I prefer not to install them globally (i. e. in /usr/local/), so I'm installing them in directory in my home directory. If OpenMW sources is in $HOME/path/openmw, I'm using $HOME/path/libs/root as prefix for boost and other libs.

It's useful to create env var for lib install prefix:
        
        $ export OMW_LIB_PREFIX=$HOME/path/libs/root`

Most of libs can be installed from [Homebrew][homebrew]. Only mpg123 needs to be installed from source (due to lack of universal compilation support). I think that some of libs can be installed from MacPorts or Fink too.

As OpenMW currently only supports i386 architecture on OS X, denendencies also should support it. Set some env vars in current terminal:

        $ export CFLAGS="-arch i386"
        $ export CXXFLAGS="-arch i386"
        $ export LDFLAGS="-arch i386"

If you close your terminal, you should set env vars again before pcoceeding to next steps!

## Boost
Download [boost][boost] and install it with the following command:

        $ cd /path/to/boost/source
        $ ./bootstrap.sh --prefix=$OMW_LIB_PREFIX
        $ ./bjam --build-dir=build --layout=versioned \
        --toolset=darwin architecture=x86 address-model=32 \
        --link-shared,static --prefix=$OMW_LIB_PREFIX install
    
        
Alternatively you can install boost with homebrew:

        $ brew install boost --universal

I think MacPorts also should support universal build for boost.

## Ogre
Download [Ogre][] SDK (tested with 1.7.3), unpack it somewhere and move
`lib/Release/Ogre.framework` into `/Library/Frameworks`.

## OIS
Download patched [OIS][] and use the XCode project provided. Be sure to set your build architecture to
   `i386`. Once it built, locate built OIS.framework with Xcode and move it to `/Library/Frameworks`.

## mpg123
Download [MPG 123][mpg123] and build it:

        $ cd /path/to/mpg123/source
        $ ./configure --prefix=$OMW_LIB_PREFIX --disable-debug \
        --disable-dependency-tracking \
        --with-optimization=4 \
        --with-audio=dummy \
        --with-default-audio=dummy \
        --with-cpu=sse_alone \
        $ make install

## libsndfile
Download [libsndfile][] and build it:

        $ cd /path/to/libsndfile/source
        $ ./configure --prefix=$OMW_LIB_PREFIX \
        --disable-dependency-tracking
        $ make install

or install with homebrew:
    
        $ brew install libsndfile --universal

## Bullet
Download [Bullet][] and build it:

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

or install with homebrew:
    
        $ brew install bullet --HEAD --universal
    
I prefer head because 2.79 has some issue which causes OpenMW to lag. Also you can edit formula and install 2.77, which is stable and haven't mentioned issue.

## Qt
Install [Qt][qt]. Qt SDK distributed by Nokia is not an option because it's 64 bit only, and OpenMW currently doesn't build for 64 bit on OS X. I'm installing it from Homebrew:

        $ brew install qt --universal

## Run CMake
Generate the Makefile for OpenMW as follows and build OpenMW:

        $ mkdir /path/to/openmw/build/dir
        $ cd /path/to/open/build/dir
        $ cmake \
        -D CMAKE_OSX_ARCHITECTURES=i386 \
        -D OGRE_SDK=/path/to/ogre/sdk \
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
    
You can use `-G"Xcode"` if you prefer Xcode, or -G"Eclipse CDT4 - Unix Makefiles"
if you prefer Eclipse. You also can specify `-D CMAKE_BUILD_TYPE=Debug` for debug
build. As for CMake 2.8.7 and Xcode 4.3, Xcode generator is broken. Sadly Eclipse CDT also cannot import generated project at least on my machine.

If all libs installed via homebrew (excluding mpg123), then command would be even simplier:

        $ cmake \
        -D CMAKE_OSX_ARCHITECTURES="i386" \
        -D OGRE_SDK=/path/to/ogre/sdk \
        -D MPG123_LIBRARY=$OMW_LIB_PREFIX/lib/libmpg123.a \
        -D MPG123_INCLUDE_DIR=$OMW_LIB_PREFIX/include \
        -G "Unix Makefiles" /path/to/openmw/source/dir
        $ make
    
Note for users with recent Xcode versions: you must explicitly specify what set of compilers do you use! If not, gcc will be used for C and Clang for C++. Just add this two -D's to command: `-D CMAKE_C_COMPILER=/usr/bin/clang` and `-D CMAKE_CXX_COMPILER=/usr/bin/clang`
    
Note for Xcode 4.3 users: you should specify full path to used SDK, because current CMake (2.8.7) couldn't find SDKs inside Xcode app bundle:
    
        -D CMAKE_OSX_SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk"

# Run
From your build directory run:

        $ OpenMW.app/Contents/MacOS/openmw
or:

        $ open OpenMW.app    
Enjoy!

[homebrew]: https://github.com/mxcl/homebrew
[boost]: http://www.boost.org
[Ogre]: http://www.ogre3d.org
[Bullet]: http://bulletphysics.org
[OIS]: https://github.com/corristo/ois-fork
[mpg123]: http://www.mpg123.de
[libsndfile]: http://www.mega-nerd.com/libsndfile
[official website]: http://openmw.com
[Will Thimbleby's Ogre Framework]: http://www.thimbleby.net/ogre/
[qt]: http://qt.nokia.com/