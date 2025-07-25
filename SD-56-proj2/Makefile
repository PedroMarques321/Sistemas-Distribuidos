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
PROTOBUF_GEN=protobuf_generated

CC=gcc
#CFLAGS = -O0 -g -Wall -I$(INC_DIR)
CFLAGS = -O0 -g -Wall -I$(INC_DIR) -I$(PROTOBUF_GEN)

OBJECTS=$(addprefix $(OBJ_DIR)/, htmessages.pb-c.o server_hashtable.o client_hashtable.o server_network.o server_skeleton.o message.o)
OBJECTS+=$(addprefix $(OBJ_DIR)/, client_stub.o client_network.o)

LIBS=$(addprefix $(LIB_DIR)/, libtable.a)

BINARIES=$(addprefix $(BIN_DIR)/, client_hashtable server_hashtable libtable)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -DOUTPUT_INFO

$(OBJ_DIR)/%.o: $(PROTOBUF_GEN)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


all: client_hashtable server_hashtable libtable

server_hashtable: $(BIN_DIR)/server_hashtable
$(BIN_DIR)/server_hashtable: $(LIB_DIR)/libtable.a $(addprefix $(OBJ_DIR)/, htmessages.pb-c.o server_hashtable.o server_network.o server_skeleton.o message.o)
	$(CC) $(CFLAGS) -Llib $(addprefix $(OBJ_DIR)/, htmessages.pb-c.o server_hashtable.o server_network.o server_skeleton.o message.o) -ltable -o $@ -lprotobuf-c

client_hashtable: $(BIN_DIR)/client_hashtable
$(BIN_DIR)/client_hashtable: $(LIB_DIR)/libtable.a $(addprefix $(OBJ_DIR)/, client_hashtable.o client_stub.o client_network.o message.o htmessages.pb-c.o)
	$(CC) $(CFLAGS) -Llib $(addprefix $(OBJ_DIR)/, client_hashtable.o client_stub.o client_network.o message.o htmessages.pb-c.o) -ltable -o $@ -lprotobuf-c

libtable: $(LIB_DIR)/libtable.a
$(LIB_DIR)/libtable.a: $(addprefix $(OBJ_DIR)/, block.o entry.o list.o table.o)
	ar -rcs $@ $^


$(OBJ_DIR)/htmessages.pb-c.o : $(PROTOBUF_GEN)/htmessages.pb-c.c
$(OBJ_DIR)/server_network.o : $(SRC_DIR)/server_network.c $(INC_DIR)/server_network.h $(PROTOBUF_GEN)/htmessages.pb-c.h
$(OBJ_DIR)/server_hashtable.o : $(SRC_DIR)/server_hashtable.c $(PROTOBUF_GEN)/htmessages.pb-c.h
$(OBJ_DIR)/message.o : $(SRC_DIR)/message.c $(INC_DIR)/message-private.h $(PROTOBUF_GEN)/htmessages.pb-c.h
$(OBJ_DIR)/server_skeleton.o : $(SRC_DIR)/server_skeleton.c $(INC_DIR)/server_skeleton.h $(PROTOBUF_GEN)/htmessages.pb-c.h
$(OBJ_DIR)/client_stub.o : $(SRC_DIR)/client_stub.c $(INC_DIR)/client_stub.h $(PROTOBUF_GEN)/htmessages.pb-c.h
$(OBJ_DIR)/client_network.o : $(SRC_DIR)/client_network.c $(INC_DIR)/client_network.h
$(OBJ_DIR)/client_hashtable.o : $(SRC_DIR)/client_hashtable.c $(PROTOBUF_GEN)/htmessages.pb-c.h

protobuf: $(PROTOBUF_GEN)/htmessages.pb-c.h
$(PROTOBUF_GEN)/htmessages.pb-c.c: $(PROTOBUF_GEN)/htmessages.pb-c.h
$(PROTOBUF_GEN)/htmessages.pb-c.h: htmessages.proto
	if ! test -e $(PROTOBUF_GEN); then mkdir $(PROTOBUF_GEN); fi
	protoc-c --c_out=$(PROTOBUF_GEN) htmessages.proto

$(OBJECTS): | $(OBJ_DIR)
$(BINARIES): | $(BIN_DIR)
$(LIBS): | $(LIB_DIR)

clean:
	rm -vf $(BINARIES)
	rm -vf $(OBJECTS)
	rm -vf $(LIBS)
	rm -vf $(PROTOBUF_GEN)/htmessages*

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)