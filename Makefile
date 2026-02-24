CC = gcc
CFLAGS = -Wall -Wextra -g -I./src/include
# LDFLAGS = -lssl -lcrypto

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = build

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/secu_engine

# Varsayılan Hedef
all: setup $(TARGET)

# Klasörleri Oluştur
setup:
	mkdir -p $(BIN_DIR)
	mkdir -p $(OBJ_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Temizlik
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all setup clean