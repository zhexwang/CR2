SHELL := /bin/bash
BIN := cr2

BUILD_DIR := build
SRC_DIR := include main elf-parser disassembler module function basic-block instruction map-table
DISASM_DIR := distorm3
DISASM_AR := distorm3.a

# pin tools
PIN_DIR := pin-tools
PIN_SCRIPT := ppin
PIN_LIB := indirect.so
PIN_BIN := ${HOME}/pin-2.14/pin 
PIN_PATH := ${HOME}/pin-2.14/source/tools/profile_indirect

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

PIN_SRC := $(foreach d,${PIN_DIR},$(wildcard ${d}/*.cpp))
#add include and pin-tools
EDIT_FILES := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.h))
EDIT_FILES += ${SRC}
EDIT_FILES += ${PIN_SRC}

.PHONY: all clean 

vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

all: lines disasm pin $(OBJ)
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
	@echo -e "[\e[34;1mCLEAN\e[m] \e[33m${PIN_SCRIPT} pin.log $(PIN_LIB) $(PIN_PATH)/obj-intel64/ \e[m"
	@make clean -s -C ${PIN_PATH} 
	@rm -rf pin.log ${PIN_SCRIPT}

lines: 
	@echo -e "[\e[34;1mECHO\e[m] \e[32;1mUntil\e[m \e[37;1m`date`\e[m\e[32;1m, you had already edited\e[m \e[37;1m`cat ${EDIT_FILES} | wc -l`\e[m \e[32;1mlines!\e[m"

pin:
	@cp -r ${PIN_SRC} ${PIN_PATH}
	@if \
	make -s -C ${PIN_PATH};\
	then echo -e "[\e[34;1mMPIN\e[m] \e[33mCompile pin tools\e[m \e[36m->\e[m \e[32;1m$(PIN_LIB)\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33mCompile pin tools\e[m \e[36m->\e[m \e[32;1m$(PIN_LIB)\e[m"; exit -1; fi;
	@echo -e "#!/bin/bash\n${PIN_BIN} -t ${PIN_PATH}/obj-intel64/${PIN_LIB} -- \\"  > ${PIN_SCRIPT}
	@echo '$$*' >> ${PIN_SCRIPT}
	@chmod a+x ${PIN_SCRIPT}

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

