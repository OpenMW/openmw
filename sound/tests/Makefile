GCC=g++ -I../

all: audiere_source_test ffmpeg_source_test openal_output_test openal_audiere_test

L_FFMPEG=$(shell pkg-config --libs libavcodec libavformat)
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

openal_audiere_test: openal_audiere_test.cpp ../sources/audiere_source.cpp ../outputs/openal_out.cpp ../../stream/clients/audiere_file.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE) $(L_OPENAL)

openal_output_test: openal_output_test.cpp ../outputs/openal_out.cpp
	$(GCC) $^ -o $@ $(L_OPENAL)

audiere_source_test: audiere_source_test.cpp ../sources/audiere_source.cpp ../../stream/clients/audiere_file.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE)

ffmpeg_source_test: ffmpeg_source_test.cpp ../sources/ffmpeg_source.cpp
	$(GCC) $^ -o $@ $(L_FFMPEG)

clean:
	rm *_test
