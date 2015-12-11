SHELL := /bin/bash
BIN := cr2

BUILD_DIR := build
SRC_DIR := include main elf-parser disassembler module function basic-block instruction map-table
DISASM_DIR := distorm3
DISASM_AR := distorm3.a

INCLUDE := -I./include -I./${DISASM_DIR}/include

# compiler and flags
OPTIMIZE := -O0 -g
EXTRA_FLAGS := -D_DEBUG -D_GNU_SOURCE 
CFLAGS := $(OPTIMIZE) -Wall $(EXTRA_FLAGS)
LDFLAGS := 
LIBS := 
CXX := g++ 
CC := gcc

#########################################
### DO NOT MODIFY THE FOLLOWING LINES ###          
#########################################

SRC := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.cpp))
SRC += $(foreach d,${SRC_DIR},$(wildcard ${d}/*.c))

OBJ := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.cpp,%.o,$(wildcard ${d}/*.cpp)))))
OBJ += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.o,$(wildcard ${d}/*.c)))))

DEP := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.cpp,%.d,$(wildcard ${d}/*.cpp)))))
DEP += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.d,$(wildcard ${d}/*.c)))))

EDIT_FILES := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.h))
EDIT_FILES += ${SRC}

.PHONY: all clean 

vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

all: lines disasm $(OBJ)
	@if \
	$(CXX) $(OBJ) $(DISASM_DIR)/$(DISASM_AR)  $(LDFLAGS) -o $(BIN) $(LIBS);\
	then echo -e "[\e[34;1mLINK\e[m] \e[33m$(DISASM_DIR)/$(DISASM_AR) $(OBJ)\e[m \e[36m->\e[m \e[32;1m$(BIN)\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$(DISASM_DIR)/$(DISASM_AR) $(OBJ)\e[m \e[36m->\e[m \e[32;1m$(BIN)\e[m"; exit -1; fi;

disasm:
	@make -s -C $(DISASM_DIR)

clean:
	@echo -e "[\e[34;1mCLEAN\e[m] \e[33m$(BIN) $(BUILD_DIR)/\e[m"
	@rm -rf $(BIN) build  
	@make clean -s -C $(DISASM_DIR)
lines:
	@echo -e "[\e[34;1mECHO\e[m] \e[32;1mUntil\e[m \e[37;1m`date`\e[m\e[32;1m, you had already edited\e[m \e[37;1m`cat ${EDIT_FILES} | wc -l`\e[m \e[32;1mlines!\e[m"
sinclude $(DEP)

$(BUILD_DIR)/%.d: %.cpp Makefile
	@mkdir -p $(BUILD_DIR);
	@$(CXX) ${CFLAGS} ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@;

$(BUILD_DIR)/%.d: %.c Makefile
	@mkdir -p $(BUILD_DIR);
	@$(CC) ${CFLAGS} ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\//g' $@;


$(BUILD_DIR)/%.o: %.cpp Makefile
	@if \
	$(CXX) ${CFLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.o: %.c Makefile
	@if \
	$(CC) ${CFLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

