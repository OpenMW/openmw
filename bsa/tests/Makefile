GCC=g++

all: bsa_file_test bsatool

bsa_file_test: bsa_file_test.cpp ../bsa_file.cpp
	$(GCC) $^ -o $@

bsatool: bsatool.cpp ../bsa_file.cpp bsatool_cmd.c
	$(GCC) $^ -o $@

bsatool_cmd.c: bsatool.ggo
	gengetopt < bsatool.ggo

clean:
	rm *_test
	rm bsatool
