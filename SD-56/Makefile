#  ____  _     _                                        
# / ___|(_)___| |_ ___ _ __ ___   __ _ ___              
# \___ \| / __| __/ _ \ '_ ` _ \ / _` / __|             
#  ___) | \__ \ ||  __/ | | | | | (_| \__ \             
# |____/|_|___/\__\___|_| |_| |_|\__,_|___/ _           
# |  _ \(_)___| |_ _ __(_) |__  _   _/_/ __| | ___  ___ 
# | | | | / __| __| '__| | '_ \| | | | |/ _` |/ _ \/ __|
# | |_| | \__ \ |_| |  | | |_) | |_| | | (_| | (_) \__ \
# |____/|_|___/\__|_|  |_|_.__/_\__,_|_|\__,_|\___/|___/
# |___ \ / _ \___ \| || |   / /___ \| ___|              
#   __) | | | |__) | || |_ / /  __) |___ \              
#  / __/| |_| / __/|__   _/ /  / __/ ___) |             
# |_____|\___/_____|  |_|/_/  |_____|____/
#            
# Grupo 56
# Pedro Marques nº48674

SHELL=/bin/bash

OBJ_DIR=object
SRC_DIR=source
INC_DIR=include
BIN_DIR=binary
LIB_DIR=lib

CC=gcc
CFLAGS = -O0 -g -Wall -I$(INC_DIR)

OBJECTS=$(addprefix $(OBJ_DIR)/, block.o entry.o list.o serialization.o table.o) 
OBJECTS += $(addprefix $(OBJ_DIR)/, test_block.o test_entry.o test_list.o test_serialization.o test_table.o)

BINARIES=$(addprefix $(BIN_DIR)/, test_block test_entry test_list test_serialization test_table)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(BINARIES)

$(BIN_DIR)/test_block: $(addprefix $(OBJ_DIR)/, test_block.o block.o)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/test_entry: $(addprefix $(OBJ_DIR)/, block.o entry.o test_entry.o)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/test_list: $(addprefix $(OBJ_DIR)/, block.o entry.o list.o test_list.o)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/test_serialization: $(addprefix $(OBJ_DIR)/, block.o entry.o list.o serialization.o test_serialization.o)
	$(CC) $(CFLAGS) $^ -o $@
$(BIN_DIR)/test_table: $(addprefix $(OBJ_DIR)/, test_table.o table.o block.o list.o entry.o)
	$(CC) $(CFLAGS) $^ -o $@


$(OBJECTS): | $(OBJ_DIR)
$(BINARIES): | $(BIN_DIR)

$(OBJ_DIR)/block.o: $(SRC_DIR)/block.c $(addprefix $(INC_DIR)/, block.h)
$(OBJ_DIR)/entry.o: $(SRC_DIR)/entry.c $(addprefix $(INC_DIR)/, entry.h)
$(OBJ_DIR)/list.o: $(SRC_DIR)/list.c $(addprefix $(INC_DIR)/, list-private.h)
$(OBJ_DIR)/serialization.o: $(SRC_DIR)/serialization.c $(addprefix $(INC_DIR)/, serialization.h)
$(OBJ_DIR)/table.o: $(SRC_DIR)/table.c $(addprefix $(INC_DIR)/, table.h)


$(OBJ_DIR)/test_block.o: $(SRC_DIR)/test_block.c
$(OBJ_DIR)/test_entry.o: $(SRC_DIR)/test_entry.c
$(OBJ_DIR)/test_list.o: $(SRC_DIR)/test_list.c
$(OBJ_DIR)/test_serialization.o: $(SRC_DIR)/test_serialization.c
$(OBJ_DIR)/test_table.o: $(SRC_DIR)/test_table.c



clean:
	rm -vf $(BINARIES)
	rm -vf $(OBJECTS)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)


