GCC=g++ -I../

all: openal_audiere_test

#L_FFMPEG=$(shell pkg-config --libs libavcodec libavformat)
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

openal_audiere_test: openal_audiere_test.cpp ../sources/audiere_source.cpp ../outputs/openal_out.cpp ../../stream/clients/audiere_file.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE) $(L_OPENAL)

clean:
	rm *_test
