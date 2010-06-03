GCC=g++ -I../

all: ogre_client_test audiere_client_test memory_server_test buffer_filter_test file_server_test slice_filter_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_AUDIERE=-laudiere

ogre_client_test: ogre_client_test.cpp ../stream.hpp ../clients/ogre_datastream.hpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

audiere_client_test: audiere_client_test.cpp ../stream.hpp ../clients/audiere_file.hpp ../clients/audiere_file.cpp
	$(GCC) $< -o $@ ../clients/audiere_file.cpp $(L_AUDIERE)

file_server_test: file_server_test.cpp ../stream.hpp ../servers/file_stream.hpp ../servers/std_stream.hpp
	$(GCC) $< -o $@

memory_server_test: memory_server_test.cpp ../stream.hpp ../servers/memory_stream.hpp
	$(GCC) $< -o $@

buffer_filter_test: buffer_filter_test.cpp ../stream.hpp ../servers/memory_stream.hpp ../filters/buffer_stream.hpp
	$(GCC) $< -o $@

slice_filter_test: slice_filter_test.cpp ../stream.hpp ../servers/memory_stream.hpp ../filters/slice_stream.hpp
	$(GCC) $< -o $@

clean:
	rm *_test
