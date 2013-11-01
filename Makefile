CC := 					gcc
COMPILE.so =		$(CC) $(CFLAGS) $(CPPFLAGS) $(SHARED_FLAGS) -o $(LIB_DIR)/$@
SHARED_FLAGS :=	-shared -fPIC

SRC_DIR := 			src
OBJ_DIR := 			$(SRC_DIR)
INCLUDE_DIR := 	include
LIB_DIR := 			lib
BIN_DIR :=			bin

VPATH := $(SRC_DIR) $(OBJ_DIR) $(INCLUDE_DIR) $(LIB_DIR) $(BIN_DIR)

%.so:
		$(COMPILE.so) $(addprefix $(OBJ_DIR)/,$(notdir $^))

.c.o:
		$(COMPILE.c) -o $(OBJ_DIR)/$(notdir $@) $(addprefix $(OBJ_DIR)/,$(notdir $^))

all: libnetscout.so
libnetscout.so: netscout-common.o

clean:
		rm -f *.o */*.o
		rm -f *.so */*.so
