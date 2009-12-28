GCC=g++ -I../

all: ogre_client_test dummy_test audiere_client_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_AUDIERE=-laudiere

ogre_client_test: ogre_client_test.cpp dummy_input.cpp ../stream.h ../clients/iwrapper.h ../clients/ogre_datastream.h
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

audiere_client_test: audiere_client_test.cpp dummy_input.cpp ../stream.h ../clients/iwrapper.h ../clients/audiere_file.h ../clients/audiere_file.cpp
	$(GCC) $< -o $@ ../clients/audiere_file.cpp $(L_AUDIERE)

dummy_test: dummy_test.cpp dummy_input.cpp ../stream.h
	$(GCC) $< -o $@

clean:
	rm *_test
