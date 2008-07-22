# Designed for GNU Make

# Compiler settings
CXXFLAGS?= -Wall -g
DMD=gdmd -version=Posix
#DMD=dmd -version=Posix

# Some extra flags for niftool and bsatool
NIFFLAGS=-debug=warnstd -debug=check -debug=statecheck -debug=strict -debug=verbose

# Compiler settings for Ogre + OIS. Change as needed.
CF_OIS=$(shell pkg-config --cflags OGRE OIS)
OGCC=$(CXX) $(CXXFLAGS) $(CF_OIS)

# Compiler settings for ffmpeg. Change as needed.
CF_FFMPEG=$(shell pkg-config --cflags libavcodec libavformat)
AVGCC=$(CXX) $(CXXFLAGS) $(CF_FFMPEG)

# Ogre C++ files, on the form ogre/cpp_X.cpp. Only the first file is
# passed to the compiler, the rest are dependencies.
ogre_cpp=ogre framelistener interface overlay bsaarchive

# FFmpeg files, in the form sound/cpp_X.cpp.
avcodec_cpp=avcodec

## No modifications should be required below this line. ##

ogre_cpp_files=$(ogre_cpp:%=ogre/cpp_%.cpp)
avcodec_cpp_files=$(avcodec_cpp:%=sound/cpp_%.cpp)

# All object files needed by openmw and esmtool
src := $(wildcard */*.d)
src := $(src) $(wildcard monster/util/*.d)
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
cpp: cpp_ogre.o cpp_avcodec.o

cpp_ogre.o: $(ogre_cpp_files)
	$(OGCC) -c $<

cpp_avcodec.o: $(avcodec_cpp_files)
	$(AVGCC) -c $<

objs/%.o: %.d
	dirname $@ | xargs mkdir -p
	$(DMD) -c $< -of$@

nifobjs/%.o: %.d
	dirname $@ | xargs mkdir -p
	$(DMD) $(NIFFLAGS) -c $< -of$@

openmw: openmw.d cpp_ogre.o cpp_avcodec.o $(obj)
	$(DMD) $^ -of$@ -L-lopenal -L-lOgreMain -L-lOIS -L-lavcodec -L-lavformat

esmtool: esmtool.d cpp_ogre.o cpp_avcodec.o $(obj)
	$(DMD) $^ -of$@ -L-lopenal -L-lOgreMain -L-lOIS -L-lavcodec -L-lavformat

niftool: niftool.d $(obj_nif)
	$(DMD) $^ -of$@

bsatool: bsatool.d $(obj_nif) objs/bsa/bsafile.o
	$(DMD) $^ -of$@

bored: bored.d
	$(DMD) $^

clean:
	-rm -f cpp_ogre.o cpp_avcodec.o bored.o bsafile.o bsatool.o esmtool.o niftool.o openmw.o
	-rm -f openmw esmtool niftool bsatool bored
	-rm -rf objs/ nifobjs/ dsss_objs/
