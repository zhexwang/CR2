SHELL := /bin/bash
SRC_DIR := src
BUILD_DIR := build
TARGET := distorm3.a

CC := gcc
CFLAGS	= -fPIC -O3 -DSUPPORT_64BIT_OFFSET -DDISTORM_STATIC

SRC := $(foreach d,${SRC_DIR},$(wildcard ${d}/*.c))
OBJ := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.o,$(wildcard ${d}/*.c)))))
DEP := $(foreach d,${SRC_DIR},$(addprefix $(BUILD_DIR)/,$(notdir $(patsubst %.c,%.d,$(wildcard ${d}/*.c)))))

vpath %.c $(SRC_DIR)

all: ${OBJ}
	@ar rs ${TARGET} ${OBJ} 2> /dev/null
	@echo -e "[\e[34;1mARRS\e[m] \e[33mdistorm3/${OBJ}\e[m \e[36m->\e[m \e[33;1mdistorm3/${TARGET}\e[m"

clean:
	@echo -e "[\e[34;1mCLEAN\e[m] \e[33mdistorm3/$(TARGET) distorm3/build/\e[m"
	@rm -rf build ${TARGET}

sinclude ${DEP}

$(BUILD_DIR)/%.o: %.c Makefile
	@mkdir -p $(BUILD_DIR);
	@if \
		${CC} ${CFLAGS} ${VERSION} -c $< -o $@;\
	then echo -e "[\e[32mCC  \e[m] \e[33mdistorm3/$<\e[m \e[36m->\e[m \e[33;1mdistorm3/$@\e[m"; \
	else echo -e "[\e[31mFAIL\e[m] \e[33mdistorm3/$<\e[m \e[36m->\e[m \e[33;1mdistorm3/$@\e[m"; exit -1; fi;

$(BUILD_DIR)/%.d: %.c Makefile
	@mkdir -p $(BUILD_DIR);
	@$(CC)  -MM $< > $@;


