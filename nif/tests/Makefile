GCC=g++

all: niftool nif_bsa_test

niftool: niftool.cpp ../nif_file.h ../nif_file.cpp ../record.h
	$(GCC) $< ../nif_file.cpp ../../tools/stringops.cpp -o $@

nif_bsa_test: nif_bsa_test.cpp ../nif_file.cpp ../../bsa/bsa_file.cpp ../../tools/stringops.cpp
	$(GCC) $^ -o $@

clean:
	rm niftool *_test
