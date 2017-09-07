BUILD_DIR = Debug
SRC_DIR = base io http SyncControl media iot device_agent #srs-librtmp-master PVDClient proxy
OUT_CPP = iot/AModule_iot.cpp media/libPSutil.cpp SyncControl/AModule_Stream.cpp

include common_makefile

CXXFLAGS += -m32
LIB_DEPEND += -shared -Lbin -lavcodec -lavformat -lavutil -lswresample -Wl,-rpath=./ \
              -lcurl -lrtmp

#vpath %.h ./

TARGET_BIN = ./bin/libAModule.so
#TARGET_BIN = ./bin/AModule

objects = $(OBJ_CPP) $(OBJ_C)
dependence = $(patsubst %.o,%.d,$(objects))

all: $(objects)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET_BIN) $(LIB_DEPEND) $(LOG_PATH)

include common_makefile_end
