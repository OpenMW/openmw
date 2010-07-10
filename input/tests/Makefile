GCC=g++

all: sdl_driver_test ois_driver_test evtlist_test

sdl_driver_test: sdl_driver_test.cpp
	$(GCC) $< ../servers/sdl_driver.cpp -o $@ -I/usr/include/SDL/ -lSDL

ois_driver_test: ois_driver_test.cpp
	$(GCC) $< ../servers/ois_driver.cpp -o $@ -I/usr/local/include/OGRE/ -lOgreMain -lOIS -lboost_filesystem

evtlist_test: evtlist_test.cpp ../filters/eventlist.hpp ../event.hpp
	$(GCC) $< -o $@

clean:
	rm *_test
