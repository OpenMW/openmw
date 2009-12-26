GCC=g++ -I../

all: dummy_test ogre_client_test ogre_resource_test ogre_server_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)

ogre_client_test: ogre_client_test.cpp dummy_vfs.cpp ../vfs.h ../imp_client/wrapper.h ../imp_client/ogre_archive.h ../imp_client/ogre_archive.cpp
	$(GCC) $< ../imp_client/ogre_archive.cpp -o $@ $(I_OGRE) $(L_OGRE)

ogre_resource_test: ogre_resource_test.cpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

ogre_server_test: ogre_server_test.cpp ../vfs.h ../imp_server/ogre_vfs.h ../imp_server/ogre_vfs.cpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE) ../imp_server/ogre_vfs.cpp

dummy_test: dummy_test.cpp dummy_vfs.cpp ../vfs.h
	$(GCC) $< -o $@

clean:
	rm *_test
