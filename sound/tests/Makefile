GCC=g++ -I../ -Wall

all: audiere_source_test ffmpeg_source_test openal_output_test openal_audiere_test openal_ffmpeg_test openal_mpg123_test openal_sndfile_test

L_FFMPEG=$(shell pkg-config --libs libavcodec libavformat)
I_FFMPEG=-I/usr/include/libavcodec -I/usr/include/libavformat
L_OPENAL=$(shell pkg-config --libs openal)
L_AUDIERE=-laudiere

openal_audiere_test: openal_audiere_test.cpp ../sources/audiere_source.cpp ../sources/sample_reader.cpp ../outputs/openal_out.cpp ../../stream/clients/audiere_file.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE) $(L_OPENAL)

openal_ffmpeg_test: openal_ffmpeg_test.cpp ../sources/ffmpeg_source.cpp ../outputs/openal_out.cpp
	$(GCC) $^ -o $@ $(L_FFMPEG) $(L_OPENAL) $(I_FFMPEG)

openal_mpg123_test: openal_mpg123_test.cpp ../sources/mpg123_source.cpp ../outputs/openal_out.cpp
	$(GCC) $^ -o $@ -lmpg123 ${L_OPENAL}

openal_sndfile_test: openal_sndfile_test.cpp ../sources/libsndfile.cpp ../sources/sample_reader.cpp ../outputs/openal_out.cpp
	$(GCC) $^ -o $@ -lsndfile ${L_OPENAL}

openal_output_test: openal_output_test.cpp ../outputs/openal_out.cpp
	$(GCC) $^ -o $@ $(L_OPENAL)

audiere_source_test: audiere_source_test.cpp ../sources/audiere_source.cpp ../../stream/clients/audiere_file.cpp ../sources/sample_reader.cpp
	$(GCC) $^ -o $@ $(L_AUDIERE)

ffmpeg_source_test: ffmpeg_source_test.cpp ../sources/ffmpeg_source.cpp
	$(GCC) $^ -o $@ $(L_FFMPEG) $(I_FFMPEG)

clean:
	rm *_test
