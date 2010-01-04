GCC=g++

all: niftool

niftool: niftool.cpp ../nif_file.h
	$(GCC) $< ../nif_file.cpp ../../tools/stringops.cpp -o $@

clean:
	rm niftool
