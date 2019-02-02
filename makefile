#PROJ_DIR will be added with build script automatically
TARGET ?= a.out
SRC_DIRS ?= .
CC=g++

SRCS := $(shell find $(SRC_DIRS) -mindepth 0 -maxdepth 1 -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(addsuffix .o,$(basename $(SRCS)))
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(PROJ_DIR) -mindepth 0 -maxdepth 0 -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LOADLIBES) $(LDLIBS) -pthread

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)