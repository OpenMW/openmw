GCC=g++

all: triangle_test esm_test

LIB_INC=-I../../../libs/

triangle_test: triangle_test.cpp
	$(GCC) $^ -o $@

esm_test: esm_test.cpp
	$(GCC) $^ -o $@ $(LIB_INC)

clean:
	rm *_test
