
CC = g++
PROTOC = protoc
CFLAGS = -std=c++11 -Wno-deprecated -Wall -c $(INCLUDES)


Q = @


ifdef DEBUG
	OPTFLAGS += -O0 -g -pg
else
	OPTFLAGS += -O3
endif


BUILD_DIR = build
SRC_DIR = src
PROTO_DIR = src/proto

PROTO_INCLUDES = $(BUILD_DIR)/$(PROTO_DIR)/RLGlue.pb.h
PROTO_SRCS     = $(BUILD_DIR)/$(PROTO_DIR)/RLGlue.pb.cc
PROTO_OBJS     = $(BUILD_DIR)/$(PROTO_DIR)/RLGlue.pb.o


LIB_OBJS = $(BUILD_DIR)/$(SRC_DIR)/Experiment.o $(BUILD_DIR)/$(SRC_DIR)/RLGlue++.o $(PROTO_OBJS)

INCLUDES = -Iinclude/ -I$(BUILD_DIR) -I$(BUILD_DIR)/$(PROTO_DIR) -I$(BUILD_DIR)/$(SRC_DIR)


.PRECIOUS: %.pb.cc %.pb.h build/src/proto/RLGlue.pb.h



LIB = lib/librlgluexx.a
all: $(LIB)


$(LIB): $(LIB_OBJS)
	@ echo AR $@
	$(Q) ar cr $@ $^

# PROTO
$(BUILD_DIR)/$(PROTO_DIR)/%.pb.h $(BUILD_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@ echo PROTOC $@
	$(Q) $(PROTOC) --cpp_out $(BUILD_DIR) $^

# PROTO OBJ
build/src/proto/RLGlue.pb.o: build/src/proto/RLGlue.pb.cc build/src/proto/RLGlue.pb.h
	@ echo CC $@
	$(Q) $(CC) $(OPTFLAGS) $(CFLAGS) $(INCLUDES) -o $@ $<

# OBJS
$(BUILD_DIR)/%.o: %.cpp $(PROTO_INCLUDES) $(PROTO_OBJS)
	@ echo CC $@
	$(Q) $(CC) $(OPTFLAGS) $(CFLAGS) $(INCLUDES) -o $@ $<





clean:
	rm -f $(LIB) $(LIB_OBJS) $(PROTO_SRCS) $(PROTO_INCLUDES)

