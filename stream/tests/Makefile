GCC=g++ -I../

all: ogre_client_test audiere_client_test memory_server_test buffer_filter_test file_server_test slice_filter_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_AUDIERE=-laudiere

ogre_client_test: ogre_client_test.cpp ../stream.h ../clients/ogre_datastream.h
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

audiere_client_test: audiere_client_test.cpp ../stream.h ../clients/audiere_file.h ../clients/audiere_file.cpp
	$(GCC) $< -o $@ ../clients/audiere_file.cpp $(L_AUDIERE)

file_server_test: file_server_test.cpp ../stream.h ../servers/file_stream.h ../servers/std_stream.h
	$(GCC) $< -o $@

memory_server_test: memory_server_test.cpp ../stream.h ../servers/memory_stream.h
	$(GCC) $< -o $@

buffer_filter_test: buffer_filter_test.cpp ../stream.h ../servers/memory_stream.h ../filters/buffer_stream.h
	$(GCC) $< -o $@

slice_filter_test: slice_filter_test.cpp ../stream.h ../servers/memory_stream.h ../filters/slice_stream.h
	$(GCC) $< -o $@

clean:
	rm *_test
