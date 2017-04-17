
CXX = g++
CXXFLAGS = -fPIC -I./ -D_DEBUG -fpermissive -g
#CXXFLAGS = -fPIC -I./ -fpermissive -O2
LD_FLAGS = -lstdc++ -lpthread

TARGET_PATH = ./bin/AModule
LOG_PATH = 2>./bin/$(BUILD_DIR).log
BUILD_DIR = Debug

vpath %.h ./

sources := $(wildcard ./*.cpp) $(wildcard ./base/*.cpp) $(wildcard ./io/*.cpp) \
	$(wildcard ./SyncControl/*.cpp) $(wildcard ./PVDClient/*.cpp) \
	$(filter-out %m3u8.cpp, $(wildcard ./proxy/*.cpp))
objects := $(patsubst %.cpp, ./$(BUILD_DIR)/%.o, $(notdir $(sources)))
dependence := $(patsubst %.o, %.d, $(objects))

all: $(objects)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET_PATH) $(LD_FLAGS) $(LOG_PATH)

./$(BUILD_DIR)/%.o: %.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./base/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./io/%.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./SyncControl/%.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./PVDClient/%.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: $(filter-out ./proxy/AModule_m3u8.cpp, ./proxy/%.cpp)
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)
	
define gen_dep 
	set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $< > $@.tmp $(LOG_PATH); \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@; \
	rm -f $@.tmp
endef

#./$(BUILD_DIR)/%.d: %.cpp
#	$(gen_dep)

#./$(BUILD_DIR)/%.d: ./base/%.cpp
#	$(gen_dep)

-include $(dependence)

.PHONY: clean echo debug
clean:
	rm -f ./$(BUILD_DIR)/*
	rm -f $(TARGET_PATH)
	
echo:   # debug util
	@echo sources=$(sources)  
	@echo objects=$(objects)  
	@echo dependence=$(dependence)  
	@echo CXXFLAGS=$(CXXFLAGS)
	@echo LD_FLAGS=$(LD_FLAGS)
