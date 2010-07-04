GCC=g++

all: funcbind_test dispatch_map_test sdl_driver_test sdl_binder_test

funcbind_test: funcbind_test.cpp ../func_binder.hpp
	$(GCC) $< -o $@

dispatch_map_test: dispatch_map_test.cpp ../dispatch_map.hpp
	$(GCC) $< -o $@

sdl_driver_test: sdl_driver_test.cpp
	$(GCC) $< ../../mangle/input/servers/sdl_driver.cpp -o $@ -I/usr/include/SDL/ -lSDL -I../../

sdl_binder_test: sdl_binder_test.cpp
	$(GCC) $< ../../mangle/input/servers/sdl_driver.cpp -o $@ -I/usr/include/SDL/ -lSDL -I../../

clean:
	rm *_test
