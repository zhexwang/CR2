SHELL := /bin/bash
BIN := cr2

BIN_DIR := bin
SCRIPT_DIR := script
BUILD_DIR := build
SRC_DIR := include main elf-parser disassembler module basic-block instruction map-table random instr-generator
DISASM_DIR := distorm3
DISASM_AR := distorm3.a

# kernel module
MODULE_DIR := kernel-module
# pin tools
PIN_DIR := pin-tools
PIN_SCRIPT := ppin
PIN_LIB := indirect.so
PIN_BIN := ${HOME}/pin-2.14/pin 
PIN_PATH := ${HOME}/pin-2.14/source/tools/profile_indirect

INCLUDE := -I./include -I./${DISASM_DIR}/include

# compiler and flags
LDFLAGS := 
LIBS := 
CXX := g++ 
CC := gcc
EXTRA_FLAGS := -D_GNU_SOURCE 

OS_VERSION = $(shell uname -r)
VM_VERSION = 3.13.0-32-generic
C10_VERSION = 3.2.0-24-generic
                                                                                                                                                                         
ifneq (, $(findstring ${VM_VERSION}, ${OS_VERSION}))
  EXTRA_FLAGS += -D_VM
else
  ifneq (, $(findstring ${C10_VERSION}, ${OS_VERSION}))
    EXTRA_FLAGS += -D_C10
  else
    EXTRA_FLAGS += -D_UNK
  endif
endif

# release
RELEASE_FLAGS := -O3 -Wall $(EXTRA_FLAGS)
RELEASE_DIR := release
# debug
DEBUG_FLAGS := -O0 -g -Wall -D_DEBUG ${EXTRA_FLAGS}
DEBUG_DIR := debug+asserts


#########################################
### DO NOT MODIFY THE FOLLOWING LINES ###          
#########################################

SRC := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.cpp))
SRC += $(foreach d,${SRC_DIR},$(wildcard ${d}/*.c))

RELEASE_OBJ := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${RELEASE_DIR}/,$(notdir $(patsubst %.cpp,%.o,$(wildcard ${d}/*.cpp)))))
RELEASE_OBJ += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${RELEASE_DIR}/,$(notdir $(patsubst %.c,%.o,$(wildcard ${d}/*.c)))))

DEBUG_OBJ := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${DEBUG_DIR}/,$(notdir $(patsubst %.cpp,%.o,$(wildcard ${d}/*.cpp)))))
DEBUG_OBJ += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${DEBUG_DIR}/,$(notdir $(patsubst %.c,%.o,$(wildcard ${d}/*.c)))))

RELEASE_DEP := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${RELEASE_DIR}/,$(notdir $(patsubst %.cpp,%.d,$(wildcard ${d}/*.cpp)))))
RELEASE_DEP += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${RELEASE_DIR}/,$(notdir $(patsubst %.c,%.d,$(wildcard ${d}/*.c)))))

DEBUG_DEP := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${DEBUG_DIR}/,$(notdir $(patsubst %.cpp,%.d,$(wildcard ${d}/*.cpp)))))
DEBUG_DEP += $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/${DEBUG_DIR}/,$(notdir $(patsubst %.c,%.d,$(wildcard ${d}/*.c)))))


PIN_SRC := $(foreach d,${PIN_DIR},$(wildcard ${d}/*.cpp))
MODULE_SRC := $(foreach d,${MODULE_DIR}/src,$(wildcard ${d}/*.c)) 
MODULE_SRC += $(foreach d,${MODULE_DIR}/src,$(wildcard ${d}/*.h)) 
#add include pin module 
EDIT_FILES := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.h))
EDIT_FILES += ${SRC}
EDIT_FILES += ${PIN_SRC}
EDIT_FILES += ${MODULE_SRC}

.PHONY: all clean 

vpath %.cpp $(SRC_DIR)
vpath %.c $(SRC_DIR)

all: lines disasm pin lkm $(RELEASE_OBJ) ${DEBUG_OBJ}
	@mkdir -p ${BIN_DIR}/${RELEASE_DIR} ${BIN_DIR}/${DEBUG_DIR};
	@if \
	$(CXX) $(RELEASE_OBJ) $(DISASM_DIR)/$(DISASM_AR)  $(LDFLAGS) -o ${BIN_DIR}/${RELEASE_DIR}/$(BIN)-release $(LIBS);\
	then echo -e "[\e[34;1mLINK\e[m] \e[33m$(DISASM_DIR)/$(DISASM_AR) $(RELEASE_OBJ)\e[m \e[36m->\e[m \e[32;1m${BIN_DIR}/${RELEASE_DIR}/$(BIN)-release\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$(DISASM_DIR)/$(DISASM_AR) $(RELEASE_OBJ)\e[m \e[36m->\e[m \e[32;1m${BIN_DIR}/${RELEASE_DIR}/$(BIN)-release\e[m"; exit -1; fi;
	@if \
	$(CXX) $(DEBUG_OBJ) $(DISASM_DIR)/$(DISASM_AR)  $(LDFLAGS) -o ${BIN_DIR}/${DEBUG_DIR}/$(BIN)-debug $(LIBS);\
	then echo -e "[\e[34;1mLINK\e[m] \e[33m$(DISASM_DIR)/$(DISASM_AR) $(DEBUG_OBJ)\e[m \e[36m->\e[m \e[32;1m${BIN_DIR}/${DEBUG_DIR}/$(BIN)-debug\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$(DISASM_DIR)/$(DISASM_AR) $(DEBUG_OBJ)\e[m \e[36m->\e[m \e[32;1m${BIN_DIR}/${DEBUG_DIR}/$(BIN)-debug\e[m"; exit -1; fi;

lkm:
	@make -s -C $(MODULE_DIR)

disasm:
	@make -s -C $(DISASM_DIR)

clean:
	@echo -e "[\e[34;1mCLEAN\e[m] \e[33m$(BIN_DIR) $(BUILD_DIR)/\e[m"
	@rm -rf $(BIN_DIR) ${BUILD_DIR}
	@make clean -s -C $(DISASM_DIR)
	@echo -e "[\e[34;1mCLEAN\e[m] \e[33mpin.log $(PIN_LIB) $(PIN_PATH)/obj-intel64/ \e[m"
	@make clean -s -C ${PIN_PATH} 
	@echo -e "[\e[34;1mCLEAN\e[m] \e[33mkernel module objects \e[m"
	@make clean -s -C $(MODULE_DIR)

lines: 
	@echo -e "[\e[34;1mECHO\e[m] \e[32;1mUntil\e[m \e[37;1m`date`\e[m\e[32;1m, you have already edited\e[m \e[37;1m`cat ${EDIT_FILES} | wc -l`\e[m \e[32;1mlines!\e[m"

pin:
	@cp -r ${PIN_SRC} ${PIN_PATH}
	@if \
	make -s -C ${PIN_PATH};\
	then echo -e "[\e[34;1mMPIN\e[m] \e[33mCompile pin tools\e[m \e[36m->\e[m \e[32;1m$(PIN_LIB)\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33mCompile pin tools\e[m \e[36m->\e[m \e[32;1m$(PIN_LIB)\e[m"; exit -1; fi;
	@mkdir -p ${BIN_DIR}/${SCRIPT_DIR};
	@echo -e "#!/bin/bash\n${PIN_BIN} -t ${PIN_PATH}/obj-intel64/${PIN_LIB} -- \\"  > ${BIN_DIR}/${SCRIPT_DIR}/${PIN_SCRIPT}
	@echo '$$*' >> ${BIN_DIR}/${SCRIPT_DIR}/${PIN_SCRIPT}
	@echo 'rm -rf pin.log' >> ${BIN_DIR}/${SCRIPT_DIR}/${PIN_SCRIPT}
	@chmod a+x ${BIN_DIR}/${SCRIPT_DIR}/${PIN_SCRIPT}

sinclude $(RELEASE_DEP)
sinclude $(DEBUG_DEP)

$(BUILD_DIR)/${RELEASE_DIR}/%.d: %.cpp Makefile
	@mkdir -p $(BUILD_DIR) ${BUILD_DIR}/${RELEASE_DIR} ${BUILD_DIR}/${DEBUG_DIR};
	@$(CXX) ${RELEASE_FLAGS} ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\/${RELEASE_DIR}\//g' $@;

$(BUILD_DIR)/${DEBUG_DIR}/%.d: %.cpp Makefile
	@mkdir -p $(BUILD_DIR) ${BUILD_DIR}/${RELEASE_DIR} ${BUILD_DIR}/${DEBUG_DIR};
	@$(CXX) ${DEBUG_FLAGS} ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\/${DEBUG_DIR}\//g' $@;

$(BUILD_DIR)/${RELEASE_DIR}/%.d: %.c Makefile
	@mkdir -p $(BUILD_DIR) ${BUILD_DIR}/${RELEASE_DIR} ${BUILD_DIR}/${DEBUG_DIR};
	@$(CC) ${RELEASE_FLAGS} ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\/${RELEASE_DIR}\//g' $@;

$(BUILD_DIR)/${DEBUG_DIR}/%.d: %.c Makefile
	@mkdir -p $(BUILD_DIR) ${BUILD_DIR}/${RELEASE_DIR} ${BUILD_DIR}/${DEBUG_DIR};
	@$(CC) ${DEBUG_FLAGS} ${INCLUDE} -MM $< > $@ && sed -i '1s/^/$(BUILD_DIR)\/${DEBUG_DIR}\//g' $@;


$(BUILD_DIR)/${RELEASE_DIR}/%.o: %.cpp Makefile
	@if \
	$(CXX) ${RELEASE_FLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/${DEBUG_DIR}/%.o: %.cpp Makefile
	@if \
	$(CXX) ${DEBUG_FLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCXX \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;


$(BUILD_DIR)/${RELEASE_DIR}/%.o: %.c Makefile
	@if \
	$(CC) ${RELEASE_FLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

$(BUILD_DIR)/${DEBUG_DIR}/%.o: %.c Makefile
	@if \
	$(CC) ${DEBUG_FLAGS} ${INCLUDE} -c $< -o $@; \
	then echo -e "[\e[32mCC  \e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33m$<\e[m \e[36m->\e[m \e[33;1m$@\e[m"; exit -1; fi;

