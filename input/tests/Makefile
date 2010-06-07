GCC=g++

all: funcbind_test dispatch_map_test event_dispatcher_test

funcbind_test: funcbind_test.cpp ../func_binder.hpp
	$(GCC) $< -o $@

dispatch_map_test: dispatch_map_test.cpp ../dispatch_map.hpp
	$(GCC) $< -o $@

event_dispatcher_test: event_dispatcher_test.cpp ../event_dispatcher.hpp
	$(GCC) $< -o $@

clean:
	rm *_test
