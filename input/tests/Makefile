GCC=g++

all: sdl_driver_test ois_driver_test

sdl_driver_test: sdl_driver_test.cpp
	$(GCC) $< ../servers/sdl_driver.cpp -o $@ -I/usr/include/SDL/ -lSDL

ois_driver_test: ois_driver_test.cpp
	$(GCC) $< ../servers/ois_driver.cpp -o $@ -I/usr/local/include/OGRE/ -lOgreMain -lOIS -lboost_filesystem

clean:
	rm *_test
