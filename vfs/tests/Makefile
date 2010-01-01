GCC=g++ -I../

all: dummy_test ogre_client_test ogre_resource_test ogre_server_test physfs_server_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_PHYSFS=-lphysfs

ogre_client_test: ogre_client_test.cpp dummy_vfs.cpp ../vfs.h ../clients/ogre_archive.h ../clients/ogre_archive.cpp
	$(GCC) $< ../clients/ogre_archive.cpp -o $@ $(I_OGRE) $(L_OGRE)

ogre_resource_test: ogre_resource_test.cpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

ogre_server_test: ogre_server_test.cpp ../vfs.h ../servers/ogre_vfs.h ../servers/ogre_vfs.cpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE) ../servers/ogre_vfs.cpp

physfs_server_test: physfs_server_test.cpp ../vfs.h ../servers/physfs_vfs.h
	$(GCC) $< -o $@ $(L_PHYSFS)

dummy_test: dummy_test.cpp dummy_vfs.cpp ../vfs.h
	$(GCC) $< -o $@

clean:
	rm *_test
