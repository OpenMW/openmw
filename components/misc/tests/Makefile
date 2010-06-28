GCC=g++

all: strops_test slice_test

slice_test: slice_test.cpp ../slice_array.hpp
	$(GCC) $< -o $@

strops_test: strops_test.cpp ../stringops.hpp ../stringops.cpp
	$(GCC) $< -o $@ ../stringops.cpp

clean:
	rm *_test
