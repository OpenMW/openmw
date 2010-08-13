GCC=g++ -Wall

all: sdl_test

sdl_test: sdl_test.cpp
	$(GCC) $< ../servers/sdl_driver.cpp -o $@ -I/usr/include/SDL/ -lSDL -lSDL_image

clean:
	rm *_test
