#Makefile for sr
#Author: Rémi Venant
COMP = gcc

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

LDOPTIONS := -lcap -lcap-ng
SR_LDOPTIONS := -lpam -lpam_misc
EXECUTABLES := sr sr_aux

OBJS := $(addprefix $(SRC_DIR)/,capabilities.o roles.o sr.o sr_aux.o sraux_management.o user.o)
BINS := $(addprefix $(BIN_DIR)/,sr sr_aux)

all: $(BINS)

.PHONY: clean

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(COMP) -c $<

$(OBJS): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR)/sr: $(addprefix $(SRC_DIR)/,capabilities.o roles.o sr.o sraux_management.o user.o)
	$(COMP) -o $@ $^ $(LDOPTIONS) $(SR_LDOPTIONS)

$(BIN_DIR)/sr_aux: $(addprefix $(SRC_DIR)/,capabilities.o sr_aux.o)
	$(COMP) -o $@ $^ $(LDOPTIONS) 

$(BINS): | $(BIN_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

install: $(addprefix $(BIN_DIR)/,sr sr_aux)
	cp $(BIN_DIR)/sr /usr/bin/sr
	setcap cap_setfcap,cap_setpcap+p /usr/bin/sr
	cp $(BIN_DIR)/sr_aux /usr/bin/sr_aux

uninstall:
	rm -f /usr/bin/sr /usr/bin/sr_aux

clean:
	@rm -rf $(BIN_DIR) $(OBJ_DIR)
	