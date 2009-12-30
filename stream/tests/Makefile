GCC=g++ -I../

all: ogre_client_test dummy_test audiere_client_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_AUDIERE=-laudiere

ogre_client_test: ogre_client_test.cpp ../stream.h ../clients/iwrapper.h ../clients/ogre_datastream.h
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

audiere_client_test: audiere_client_test.cpp ../stream.h ../clients/iwrapper.h ../clients/audiere_file.h ../clients/audiere_file.cpp
	$(GCC) $< -o $@ ../clients/audiere_file.cpp $(L_AUDIERE)

memory_test: memory_test.cpp ../stream.h ../servers/memory_stream.h
	$(GCC) $< -o $@

buffer_test: buffer_test.cpp ../stream.h ../servers/memory_stream.h ../filters/buffer_stream.h
	$(GCC) $< -o $@

clean:
	rm *_test
