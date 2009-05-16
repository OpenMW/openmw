# Designed for GNU Make

# Compiler settings
CXXFLAGS?= -g -Iutil/
DMD=gdmd -version=Posix

# Some extra flags for niftool and bsatool
NIFFLAGS=-debug=warnstd -debug=check -debug=statecheck -debug=strict -debug=verbose

# Linker flags
LFLAGS= -L-lopenal -L-lOgreMain -L-lOIS -L-lmygui -L-luuid -L-lavcodec -L-lavformat bullet/libbulletdynamics.a bullet/libbulletcollision.a bullet/libbulletmath.a -L-lboost_serialization

# Compiler settings for Ogre, OIS and MyGUI
# TODO: the -I when we're done
CF_OIS=$(shell pkg-config --cflags OIS OGRE MyGUI) -Iterrain/
OGCC=$(CXX) $(CXXFLAGS) $(CF_OIS)

# Compiler settings for ffmpeg.
CF_FFMPEG=$(shell pkg-config --cflags libavcodec libavformat)
AVGCC=$(CXX) $(CXXFLAGS) $(CF_FFMPEG)

# Settings for Bullet
CF_BULLET=-Iinclude/bullet
BGCC=$(CXX) $(CXXFLAGS) $(CF_BULLET)

# Ogre C++ files, on the form ogre/cpp_X.cpp. Only the first file is
# passed to the compiler, the rest are dependencies.
ogre_cpp=ogre framelistener interface bsaarchive

# MyGUI C++ files, gui/cpp_X.cpp. These are currently included
# cpp_ogre.o with cpp_ogre.cpp.
mygui_cpp=mygui console

# Ditto for the landscape engine, in terrain/cpp_X.cpp
terrain_cpp=baseland esm framelistener generator index landdata\
materialgen mwheightmap palette point2\
quad quaddata terraincls terrain terrainmesh

# FFmpeg files, in the form sound/cpp_X.cpp.
avcodec_cpp=avcodec

# Bullet cpp files
bullet_cpp=bullet player scale

#### No modifications should be required below this line. ####

ogre_cpp_files=\
	$(ogre_cpp:%=ogre/cpp_%.cpp) \
	$(mygui_cpp:%=gui/cpp_%.cpp) \
	$(terrain_cpp:%=terrain/cpp_%.cpp)
avcodec_cpp_files=$(avcodec_cpp:%=sound/cpp_%.cpp)
bullet_cpp_files=$(bullet_cpp:%=bullet/cpp_%.cpp)

# All object files needed by openmw and esmtool
src := $(wildcard bsa/*.d) $(wildcard bullet/*.d) $(wildcard core/*.d) \
$(wildcard esm/*.d) $(wildcard input/*.d) $(wildcard nif/*.d) $(wildcard ogre/*.d) \
$(wildcard scene/*.d) $(wildcard sound/*.d) $(wildcard util/*.d) $(wildcard gui/*.d)
src := $(src) $(wildcard mscripts/*.d) $(wildcard terrain/*.d)
src := $(src) monster/monster.d \
$(wildcard monster/vm/*.d) \
$(wildcard monster/compiler/*.d) \
$(wildcard monster/util/*.d) \
$(wildcard monster/modules/*.d)
obj := $(src:%.d=objs/%.o)

# The NIF object files for niftool and bsatool are put in a separate
# directory, since they are built with different flags.
src_nif := $(wildcard nif/*.d)
src_nif := $(src_nif) $(wildcard util/*.d)
src_nif := $(src_nif) core/memory.d
src_nif := $(src_nif) $(wildcard monster/util/*.d)
obj_nif := $(src_nif:%.d=nifobjs/%.o)

.PHONY: cpp all clean

# Build everything. Default when running 'make' directly.
all: openmw esmtool niftool bsatool bored

# Only build C++ sources. Used when building from DSSS.
cpp: cpp_ogre.o cpp_avcodec.o cpp_bullet.o

cpp_ogre.o: $(ogre_cpp_files)
	$(OGCC) -o $@ -c $<

cpp_avcodec.o: $(avcodec_cpp_files)
	$(AVGCC) -o $@ -c $<

cpp_bullet.o: $(bullet_cpp_files)
	$(BGCC) -o $@ -c $<

objs/%.o: %.d
	dirname $@ | xargs mkdir -p
	$(DMD) -c $< -of$@

nifobjs/%.o: %.d
	dirname $@ | xargs mkdir -p
	$(DMD) $(NIFFLAGS) -c $< -of$@

openmw: openmw.d cpp_ogre.o cpp_avcodec.o cpp_bullet.o $(obj)
	$(DMD) $^ -of$@ $(LFLAGS)

esmtool: esmtool.d cpp_ogre.o cpp_avcodec.o cpp_bullet.o $(obj)
	$(DMD) $^ -of$@ $(LFLAGS)

niftool: niftool.d $(obj_nif)
	$(DMD) $^ -of$@

bsatool: bsatool.d $(obj_nif) objs/bsa/bsafile.o
	$(DMD) $^ -of$@

bored: bored.d
	$(DMD) $^

clean:
	-rm -f cpp_bullet.o cpp_ogre.o cpp_avcodec.o bored.o bsafile.o bsatool.o esmtool.o niftool.o openmw.o
	-rm -f openmw esmtool niftool bsatool bored
	-rm -rf objs/ nifobjs/ dsss_objs/
