GCC=g++ -I../

all: ogrevfs_audiere_openal_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

ogrevfs_audiere_openal_test: ogrevfs_audiere_openal_test.cpp ../vfs/servers/ogre_vfs.cpp ../sound/sources/audiere_source.cpp ../sound/outputs/openal_out.cpp ../stream/clients/audiere_file.cpp
	$(GCC) $^ -o $@ $(I_OGRE) $(L_OGRE) $(L_OPENAL) $(L_AUDIERE)

clean:
	rm *_test
