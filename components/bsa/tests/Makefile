GCC=g++

all: bsa_file_test ogre_archive_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)

bsa_file_test: bsa_file_test.cpp ../bsa_file.cpp
	$(GCC) $^ -o $@

ogre_archive_test: ogre_archive_test.cpp ../bsa_file.cpp ../bsa_archive.cpp
	$(GCC) $^ -o $@ $(I_OGRE) $(L_OGRE)

clean:
	rm *_test
