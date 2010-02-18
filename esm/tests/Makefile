all: esmtool

esmtool: esmtool.cpp ../esm_reader.hpp ../records.hpp
	g++ esmtool.cpp ../../tools/stringops.cpp -o $@

clean:
	rm *_test esmtool
