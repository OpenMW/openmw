all: esmtool

esmtool: esmtool.cpp ../esm_reader.hpp ../records.hpp esmtool_cmd.c
	g++ esmtool.cpp ../../tools/stringops.cpp esmtool_cmd.c -o $@

esmtool_cmd.c: esmtool.ggo
	gengetopt < esmtool.ggo

clean:
	rm *_test esmtool
