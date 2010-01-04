GCC=g++

all: niftool

niftool: niftool.cpp ../nif_file.h
	$(GCC) $< ../../tools/stringops.cpp -o $@

clean:
	rm niftool
