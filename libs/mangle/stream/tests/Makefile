GCC=g++ -I../ -Wall -Werror

all: ogre_client_test audiere_client_test memory_server_test buffer_filter_test file_server_test slice_filter_test file_write_test iostream_test

I_OGRE=$(shell pkg-config --cflags OGRE)
L_OGRE=$(shell pkg-config --libs OGRE)
L_AUDIERE=-laudiere

ogre_client_test: ogre_client_test.cpp ../stream.hpp ../clients/ogre_datastream.hpp
	$(GCC) $< -o $@ $(I_OGRE) $(L_OGRE)

audiere_client_test: audiere_client_test.cpp ../stream.hpp ../clients/audiere_file.hpp ../clients/audiere_file.cpp
	$(GCC) $< -o $@ ../clients/audiere_file.cpp $(L_AUDIERE)

iostream_test: iostream_test.cpp ../clients/io_stream.cpp
	$(GCC) $^ -o $@

file_server_test: file_server_test.cpp ../stream.hpp ../servers/file_stream.hpp ../servers/std_stream.hpp
	$(GCC) $< -o $@

file_write_test: file_write_test.cpp ../stream.hpp ../servers/outfile_stream.hpp ../servers/std_ostream.hpp
	$(GCC) $< -o $@

memory_server_test: memory_server_test.cpp ../stream.hpp ../servers/memory_stream.hpp
	$(GCC) $< -o $@

buffer_filter_test: buffer_filter_test.cpp ../stream.hpp ../servers/memory_stream.hpp ../filters/buffer_stream.hpp
	$(GCC) $< -o $@

slice_filter_test: slice_filter_test.cpp ../stream.hpp ../servers/memory_stream.hpp ../filters/slice_stream.hpp
	$(GCC) $< -o $@

clean:
	rm *_test
