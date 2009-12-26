GCC=g++ -I../

all: ogrevfs_audiere_openal_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

ogrevfs_audiere_openal_test: ogrevfs_audiere_openal_test.cpp ../vfs/imp_server/ogre_vfs.cpp ../sound/imp/input_audiere.cpp ../sound/imp/output_openal.cpp ../stream/imp_client/audiere_file.cpp
	$(GCC) $^ -o $@ $(I_OGRE) $(L_OGRE) $(L_OPENAL) $(L_AUDIERE)

clean:
	rm *_test
