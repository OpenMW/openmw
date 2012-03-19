GCC=g++ -I../ -Wall

all: dummy_test ogre_client_test ogre_resource_test ogre_server_test physfs_server_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_PHYSFS=-lphysfs

ogre_client_test: ogre_client_test.cpp dummy_vfs.cpp ../vfs.hpp ../clients/ogre_archive.hpp ../clients/ogre_archive.cpp
	$(GCC) $< ../clients/ogre_archive.cpp -o $@ $(I_OGRE) $(L_OGRE)

ogre_resource_test: ogre_resource_test.cpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

ogre_server_test: ogre_server_test.cpp ../vfs.hpp ../servers/ogre_vfs.hpp ../servers/ogre_vfs.cpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE) ../servers/ogre_vfs.cpp

physfs_server_test: physfs_server_test.cpp ../vfs.hpp ../servers/physfs_vfs.hpp
	$(GCC) $< -o $@ $(L_PHYSFS)

dummy_test: dummy_test.cpp dummy_vfs.cpp ../vfs.hpp
	$(GCC) $< -o $@

clean:
	rm *_test
