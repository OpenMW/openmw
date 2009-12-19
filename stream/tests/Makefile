GCC=g++ -I../ -I../imp_client/

all: ogre_client_test dummy_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)

ogre_client_test: ogre_client_test.cpp dummy_input.cpp ../input.h ../imp_client/iwrapper.h ../imp_client/ogre_datastream.h
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

dummy_test: dummy_test.cpp dummy_input.cpp ../input.h
	$(GCC) $< -o $@

clean:
	rm *_test
