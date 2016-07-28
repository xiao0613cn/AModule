
CXX:= g++
CFLAGS:= -fPIC -I./ -D_DEBUG -lstdc++

TAR:= ./build/AModule
LIB_DEPEND:= -lpthread

vpath %.h ./

sources := $(wildcard ./*.cpp) $(wildcard ./base/*.cpp) $(wildcard ./io/*.cpp) $(wildcard ./SyncControl/*.cpp) $(wildcard ./PVDClient/*.cpp) $(filter-out %m3u8.cpp, $(wildcard ./proxy/*.cpp))
objects := $(patsubst %.cpp, ./build/%.o, $(notdir $(sources)))
dependence := $(patsubst %.o, %.d, $(objects))

all: $(objects)
	$(CXX) $(CFLAGS) $^ -o ./$(TAR) $(LIB_DEPEND) 2>./build/log

./build/%.o: %.cpp 
	$(CXX) -c $(CFLAGS) $< -o $@ 

./build/%.o: ./base/%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@ 

./build/%.o: ./io/%.cpp 
	$(CXX) -c $(CFLAGS) $< -o $@ 

./build/%.o: ./SyncControl/%.cpp 
	$(CXX) -c $(CFLAGS) $< -o $@ 

./build/%.o: ./PVDClient/%.cpp 
	$(CXX) -c $(CFLAGS) $< -o $@ 

./build/%.o: $(filter-out ./proxy/AModule_m3u8.cpp, ./proxy/%.cpp)
	$(CXX) -c $(CFLAGS) $< -o $@ 
	
define gen_dep 
	set -e; rm -f $@; \
	$(CXX) -MM $(CFLAGS) $< > $@.tmp; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@; \
	rm -f $@.tmp
endef

./build/%.d: %.cpp
	$(gen_dep)

./build/%.d: ./base/%.cpp
	$(gen_dep)

-include $(dependence)

.PHONY: clean echo debug
clean:
	rm -f ./build/*
	rm -f ./$(TAR)
	
echo:   # debug util
	@echo sources=$(sources)  
	@echo objects=$(objects)  
	@echo dependence=$(dependence)  
	@echo CFLAGS=$(CFLAGS)
