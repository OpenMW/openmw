GCC=g++ -Wall -Werror

all: sdl_test sdl_move_test sdlgl_move_test

sdl_test: sdl_test.cpp
	$(GCC) $< ../servers/sdl_driver.cpp -o $@ -I/usr/include/SDL/ -lSDL -lSDL_image

sdl_move_test: sdl_move_test.cpp ../servers/sdl_driver.cpp
	$(GCC) $^ -o $@ -I/usr/include/SDL/ -lSDL -lSDL_image

sdlgl_move_test: sdlgl_move_test.cpp ../servers/sdl_gl_driver.cpp
	$(GCC) $^ -o $@ -I/usr/include/SDL/ -lSDL -lSDL_image -lGL

clean:
	rm *_test
