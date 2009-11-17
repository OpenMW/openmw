GCC=g++ -I../ -I../imp/

all: audiere_test ffmpeg_openal_test

L_FFMPEG=$(shell pkg-config --libs libavcodec libavformat)
L_OPENAL=$(shell pkg-config --libs openal)

L_AUDIERE=-laudiere

ffmpeg_openal_test: ffmpeg_openal_test.cpp ../imp/input_ffmpeg.cpp ../imp/output_openal.cpp
	$(GCC) $^ -o $@ $(L_FFMPEG) $(L_OPENAL)

audiere_test: audiere_test.cpp ../imp/audiere_imp.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE)

clean:
	rm *_test
