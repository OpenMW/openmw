GCC=g++ -I../

all: sound_manager_test sound_3d_test

L_FFMPEG=$(shell pkg-config --libs libavcodec libavformat)
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

sound_manager_test: sound_manager_test.cpp ../../mangle/sound/sources/audiere_source.cpp ../../mangle/sound/outputs/openal_out.cpp ../../mangle/stream/clients/audiere_file.cpp ../sndmanager.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE) $(L_OPENAL) -I../..

sound_3d_test: sound_3d_test.cpp ../../mangle/sound/sources/audiere_source.cpp ../../mangle/sound/outputs/openal_out.cpp ../../mangle/stream/clients/audiere_file.cpp ../sndmanager.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE) $(L_OPENAL) -I../..

clean:
	rm *_test
