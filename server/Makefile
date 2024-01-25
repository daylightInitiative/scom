TARGET=scomd
ALIAS=scomd
CC=gcc

SRC_PATH=src
OBJ_PATH=obj
BIN_PATH=bin

SRC=$(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJS=$(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

INCLUDES=
CFLAGS=-O0 -Wall -Wextra -ggdb $(INCLUDES)
##-Wall -Wextra -g $(INCLUDES)
LDFLAGS=

all: $(BIN_PATH)/$(TARGET)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@mkdir -p $(OBJ_PATH)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_PATH)/$(TARGET): $(OBJS)
	@echo : LINK-EXECUTABLE
	@mkdir -p $(BIN_PATH)
	@ln -s $(BIN_PATH)/$(TARGET) ./$(ALIAS)
	$(CC) $(CFLAGS) -o $(BIN_PATH)/$(TARGET) $^ $(LDFLAGS)

help:
	@echo make       - Compiles the target executable
	@echo make run   - Runs the compiled executable
	@echo make help  - Shows this help message again
	@echo make clean - Cleans object and executable files
	@echo make check - Checks the binary for memory leaks
	@echo make debug - Starts a gdb debugging instance
	@echo make slave - Creates a valgrind and gdb instance

check:
	@echo : MEM-CHECK
	@valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./bin/$(TARGET)

debug:
	@echo : DEBUG
	@gdb $(BIN_PATH)/$(TARGET)

slave:
	@echo : SLAVE-PAIRED
	@st -e /bin/bash --rcfile <(echo "cd `pwd .`; valgrind -s --vgdb=yes --vgdb-error=0 --leak-check=full --show-leak-kinds=all --track-origins=yes ./bin/$(TARGET);") &
	@sleep 2s && gdb -ex 'target remote | vgdb' $(BIN_PATH)/$(TARGET)
	@echo "Killed valgrind slave pair."
	@kill -9 `pgrep -f valgrind` &

run:
	@echo : RUN-TARGET
	$(BIN_PATH)/$(TARGET)

clean:
	@echo : CLEAN-STAGING
	-@rm -rf $(OBJ_PATH)/*.o
	-@rm -rf $(BIN_PATH)/*
	-@rm -rf ./$(ALIAS)
	-@rmdir $(BIN_PATH) $(OBJ_PATH)

