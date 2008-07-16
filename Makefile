# Designed for GNU Make

# Compiler settings
CXX?= g++
CXXFLAGS?= -Wall -g
DMD=gdmd -version=Posix

# Some extra flags for niftool and bsatool
NIFFLAGS=

# Compiler settings for Ogre + OIS. Change as needed.
OGCC=$(CXX) $(CXXFLAGS) `pkg-config --cflags OGRE OIS`

# Compiler settings for ffmpeg. Change as needed.
AVGCC=$(CXX) $(CXXFLAGS) `pkg-config --cflags libavcodec libavformat`

# Ogre C++ files, on the form ogre/cpp_X.cpp. Only the first file is
# passed to the compiler, the rest are dependencies.
ogre_cpp=ogre framelistener interface overlay bsaarchive

# FFmpeg files, in the form sound/cpp_X.cpp. Only the first file is
# passed to the compiler, the rest are dependencies.
avcodec_cpp=avcodec

## The rest of this file is automatic ##

ogre_cpp_files=$(ogre_cpp:%=ogre/cpp_%.cpp)
avcodec_cpp_files=$(avcodec_cpp:%=sound/cpp_%.cpp)

d_files=$(wildcard */*.d) $(wildcard monster/util/*.d)
d_files_nif=$(wildcard nif/*.d) $(wildcard util/*.d) $(wildcard core/memory.d) $(wildcard monster/util/*.d)

# The NIF object files for niftool and bsatool are put in a separate
# directory, since they are built with different flags.
d_objs=$(d_files:%.d=objs/%.o)
d_objs_nif=$(d_files_nif:%.d=nifobjs/%.o)

.PHONY: cpp all clean makedirs

# By default, make will only build the Ogre C++ sources.
cpp: cpp_ogre.o cpp_avcodec.o

all: makedirs openmw esmtool niftool bsatool bored

cpp_ogre.o: $(ogre_cpp_files)
	$(OGCC) -c $<

cpp_avcodec.o: $(avcodec_cpp_files)
	$(AVGCC) -c $<

objs/%.o: %.d makedirs
	$(DMD) -c $< -of$@

nifobjs/%.o: %.d makedirs
	$(DMD) -debug=warnstd -debug=check -debug=statecheck -debug=strict -debug=verbose -c $< -of$@

# This is a hack for gdmd (dmd-like frontend to gdc), since it does
# not automatically create directories as it should.
makedirs:
	mkdir -p objs/bsa
	mkdir -p objs/core
	mkdir -p objs/esm
	mkdir -p objs/input
	mkdir -p objs/monster/util
	mkdir -p objs/nif
	mkdir -p objs/ogre
	mkdir -p objs/scene
	mkdir -p objs/sound
	mkdir -p objs/util
	mkdir -p nifobjs/nif
	mkdir -p nifobjs/util
	mkdir -p nifobjs/core
	mkdir -p nifobjs/monster/util
	mkdir -p nifobjs/bsa

openmw: openmw.d cpp_ogre.o cpp_avcodec.o $(d_objs)
	$(DMD) $^ -of$@ -L-lopenal -L-lOgreMain -L-lOIS -L-lavcodec -L-lavformat

esmtool: esmtool.d cpp_ogre.o cpp_avcodec.o $(d_objs)
	$(DMD) $^ -of$@ -L-lopenal -L-lOgreMain -L-lOIS -L-lavcodec -L-lavformat

niftool: niftool.d $(d_objs_nif)
	$(DMD) $^ -of$@

bsatool: bsatool.d $(d_objs_nif) bsa/bsafile.d
	$(DMD) $^ -of$@

bored: bored.d
	$(DMD) $^

clean:
	-rm -f cpp_ogre.o cpp_avcodec.o bored.o bsafile.o bsatool.o esmtool.o niftool.o openmw.o
	-rm -f openmw esmtool niftool bsatool bored
	-rm -rf objs/ nifobjs/ dsss_objs/
