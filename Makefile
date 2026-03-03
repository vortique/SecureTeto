CC = gcc

CFLAGS = -Wall -Wextra -I./src/include
CFLAGS += -I/mingw64/include

LDFLAGS = -L/mingw64/lib -lsodium

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = build
LIB_DIR = $(BIN_DIR)/lib
TEST_DIR = tests

SRCS = $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS = $(SRC_DIR)/archiver.c $(SRC_DIR)/argparse.c
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

ifeq ($(OS), Windows_NT)
	TARGET = $(BIN_DIR)/secu_engine.exe
else
	TARGET = $(BIN_DIR)/secu_engine
endif

ifeq ($(MODE), release)
    DEBUG_FLAGS = -O2
else
    DEBUG_FLAGS = -g
endif

# ifeq ($(OS), Windows_NT)
#     LDFLAGS = -lucrt
# else
#     LDFLAGS = # -lssl -lcrypto
# endif

all: setup $(TARGET)

setup:
ifeq ($(OS), Windows_NT)
	mkdir $(BIN_DIR) $(OBJ_DIR) $(LIB_DIR) 2>nul || exit 0
else
	mkdir -p $(BIN_DIR) $(OBJ_DIR) $(LIB_DIR)
endif

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | setup
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

lib-win: setup
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -shared -o $(LIB_DIR)/libsecu_engine.dll $(LIB_SRCS) $(LDFLAGS)

lib-linux: setup
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -fPIC -shared -o $(LIB_DIR)/libsecu_engine.so $(LIB_SRCS) $(LDFLAGS)

lib-mac: setup
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -dynamiclib -o $(LIB_DIR)/libsecu_engine.dylib $(LIB_SRCS) $(LDFLAGS)

lib-all: lib-win lib-linux lib-mac

prepare-test-linux: all lib-linux
	rm -rf $(TEST_DIR)
	mkdir -p $(TEST_DIR)/gui
	cp -r gui/* $(TEST_DIR)/gui/
	mkdir -p $(TEST_DIR)/bin/lib
	cp $(LIB_DIR)/* $(TEST_DIR)/bin/lib/
	@echo "Done."

prepare-test-win: all lib-win
	rm -rf $(TEST_DIR)
	mkdir -p $(TEST_DIR)/gui
	cp -r gui/* $(TEST_DIR)/gui/
	mkdir -p $(TEST_DIR)/bin/lib
	cp $(LIB_DIR)/* $(TEST_DIR)/bin/lib/
	@echo "Done."

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all setup clean lib-win lib-linux lib-mac lib-all prepare-test-linux prepare-test-win