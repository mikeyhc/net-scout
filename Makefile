CC := 					gcc
COMPILE.so =		$(CC) $(CFLAGS) $(CPPFLAGS) $(SHARED_FLAGS) -o $(LIB_DIR)/$@
SHARED_FLAGS :=	-shared

SRC_DIR := 			src
OBJ_DIR := 			$(SRC_DIR)
INCLUDE_DIR := 	include
LIB_DIR := 			lib
BIN_DIR :=			bin

LOADLIBES := $(LOADLIBES) -L$(LIB_DIR)
CFLAGS := $(CFLAGS) -I$(INCLUDE_DIR)
LINKER := -Wl,-rpath,$(LIB_DIR)
VPATH := $(SRC_DIR) $(OBJ_DIR) $(INCLUDE_DIR) $(LIB_DIR) $(BIN_DIR)

all: libnetscout.so tools
tools: map_local
libnetscout.so: netscout-common.o

map_local: LDLIBS += -lnetscout

%.so:
		$(COMPILE.so) $(addprefix $(OBJ_DIR)/,$(notdir $^))

%: %.c
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o bin/$@ $(LINKER)

.c.o:
		$(COMPILE.c) -o $(OBJ_DIR)/$(notdir $@) $(addprefix $(OBJ_DIR)/,$(notdir $^))

clean:
		rm -f *.o */*.o
		rm -f *.so */*.so
		rm -f bin/*
