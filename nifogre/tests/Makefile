GCC=g++

all: ogre_manualresource_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)

ogre_manualresource_test: ogre_manualresource_test.cpp
	$(GCC) $^ -o $@ $(I_OGRE) $(L_OGRE)

ogre_nif_test: ogre_nif_test.cpp ../../nif/nif_file.cpp ../../bsa/bsa_file.cpp ../../tools/stringops.cpp
	$(GCC) $^ -o $@ $(I_OGRE) $(L_OGRE)

clean:
	rm *_test

