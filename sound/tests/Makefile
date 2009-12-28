GCC=g++ -I../

all: audiere_test ffmpeg_openal_test openal_audiere_test

L_FFMPEG=$(shell pkg-config --libs libavcodec libavformat)
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

ffmpeg_openal_test: ffmpeg_openal_test.cpp ../servers/input_ffmpeg.cpp ../servers/output_openal.cpp
	$(GCC) $^ -o $@ $(L_FFMPEG) $(L_OPENAL)

openal_audiere_test: openal_audiere_test.cpp ../servers/input_audiere.cpp ../servers/output_openal.cpp ../../stream/clients/audiere_file.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE) $(L_OPENAL)

audiere_test: audiere_test.cpp ../servers/audiere_imp.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE)

clean:
	rm *_test
