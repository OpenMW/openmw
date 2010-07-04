GCC=g++

all: funcbind_test dispatch_map_test

funcbind_test: funcbind_test.cpp ../func_binder.hpp
	$(GCC) $< -o $@

dispatch_map_test: dispatch_map_test.cpp ../dispatch_map.hpp
	$(GCC) $< -o $@

clean:
	rm *_test
