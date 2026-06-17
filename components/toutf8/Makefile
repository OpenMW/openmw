tablesgen.hpp: geniconv
	./geniconv > tablesgen.hpp

geniconv: geniconv.cpp
	g++ -Wall $^ -o $@

clean:
	rm -f ./geniconv
