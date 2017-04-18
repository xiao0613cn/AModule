
CXX = g++
CXXFLAGS = -fPIC -I./ -D_DEBUG -fpermissive -g
#CXXFLAGS = -fPIC -I./ -fpermissive -O2
LD_FLAGS = -lstdc++ -lpthread

CC = gcc
CFLAGS = $(CXXFLAGS)

TARGET_BIN = ./bin/AModule
LOG_PATH = 2>./bin/$(BUILD_DIR).log
BUILD_DIR = Debug

#vpath %.h ./

sources_cpp = $(wildcard ./*.cpp) $(wildcard ./base/*.cpp) \
	$(wildcard ./io/*.cpp) $(wildcard ./http/*.cpp) \
	$(wildcard ./SyncControl/*.cpp) $(wildcard ./PVDClient/*.cpp) \
	$(filter-out %m3u8.cpp, $(wildcard ./proxy/*.cpp)) \
	$(wildcard ./media/*.cpp) $(wildcard ./iot/*.cpp) 
objects_cpp = $(patsubst %.cpp, ./$(BUILD_DIR)/%.o, $(notdir $(sources_cpp)))
	
sources_c = $(wildcard ./base/*.c) $(wildcard ./http/*.c) $(wildcard ./iot/*.c) \
	$(wildcard ./crypto/*.c) 
objects_c = $(patsubst %.c, ./$(BUILD_DIR)/%.o, $(notdir $(sources_c)))

objects = $(objects_cpp) $(objects_c)

dependence = $(patsubst %.o, %.d, $(objects))

all: $(objects)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET_BIN) $(LD_FLAGS) $(LOG_PATH)

./$(BUILD_DIR)/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./base/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./io/%.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)
	
./$(BUILD_DIR)/%.o: ./http/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./SyncControl/%.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./PVDClient/%.cpp 
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: $(filter-out ./proxy/AModule_m3u8.cpp, ./proxy/%.cpp)
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./media/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./iot/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./base/%.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./http/%.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./iot/%.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LOG_PATH)

./$(BUILD_DIR)/%.o: ./crypto/%.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LOG_PATH)

define gen_dep_cpp
	set -e; rm -f $@; \
	$(CXX) -MM $(CXXFLAGS) $< > $@.tmp $(LOG_PATH); \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@; \
	rm -f $@.tmp
endef

define gen_dep_c
	set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.tmp $(LOG_PATH); \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.tmp > $@; \
	rm -f $@.tmp
endef

#./$(BUILD)/%.d: %.cpp
#	$(gen_dep)

#./$(BUILD)/%.d: ./base/%.cpp
#	$(gen_dep)

-include $(dependence)

.PHONY: clean echo debug
clean:
	-rm -f ./$(BUILD_DIR)/*
	-rm -f $(TARGET_BIN)
	
echo:   # debug util
	@echo sources_cpp = $(sources_cpp)
	@echo
	@echo sources_c = $(sources_c)
	@echo
	@echo objects = $(objects) 
	@echo
	@echo dependence = $(dependence) 
	@echo
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo
	@echo LD_FLAGS = $(LD_FLAGS)
