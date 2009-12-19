GCC=g++ -I../ -I../imp_client/

all: dummy_test ogre_client_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)

ogre_client_test: ogre_client_test.cpp dummy_vfs.cpp ../vfs.h ../imp_client/wrapper.h ../imp_client/ogre_archive.h ../imp_client/ogre_archive.cpp
	$(GCC) $< ../imp_client/ogre_archive.cpp -o $@ $(I_OGRE) $(L_OGRE)

dummy_test: dummy_test.cpp dummy_vfs.cpp ../vfs.h
	$(GCC) $< -o $@

clean:
	rm *_test
