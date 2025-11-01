CC = gcc
CFLAGS = -O0 -Wall -Wextra -pedantic -ggdb
OBJ_PATH = obj
BIN_PATH = bin
LIB = $(BIN_PATH)/libshared.a
SRC = $(wildcard *.c)
OBJS = $(addprefix $(OBJ_PATH)/,$(SRC:.c=.o))

all: $(LIB)

$(OBJ_PATH)/%.o: %.c
	@mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	@mkdir -p $(BIN_PATH)
	ar rcs $@ $^

clean:
	rm -rf $(OBJ_PATH) $(BIN_PATH)