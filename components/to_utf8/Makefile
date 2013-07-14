tables_gen.hpp: gen_iconv
	./gen_iconv > tables_gen.hpp

gen_iconv: gen_iconv.cpp
	g++ -Wall $^ -o $@

clean:
	rm -f ./gen_iconv