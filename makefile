
CXX:= gcc
CFLAGS:= -fPIC -I./ -D_DEBUG -lstdc++

TAR:= AModule
LIB_DEPEND:= -lpthread

vpath %.h ./
vpath %.o ./build
vpath %.d ./build

sources := $(wildcard ./*.cpp) $(wildcard ./base/*.cpp)
objects := $(patsubst %.cpp, ./build/%.o, $(notdir $(sources)))
dependence := $(patsubst %.o, %.d, $(objects))

all: $(objects)
	$(CXX) $(CFLAGS) $^ -o ./$(TAR) $(LIB_DEPEND) 

./build/%.o: %.cpp 
	$(CXX) -c $(CFLAGS) $< -o $@ 

./build/%.o: ./base/%.cpp
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
