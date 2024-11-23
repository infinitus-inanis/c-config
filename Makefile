CC = gcc
AR = ar

CCF = \
	-Wall -Wextra \
	-g -ggdb -gdwarf \
	-fsanitize=address -fno-omit-frame-pointer -static-libasan -lrt

CFG_DIR = lib
CFG_LIB = $(CFG_DIR)/libxcfg.a

CFG_DEP = \
	$(CFG_DIR)/utils/hashtable.h \
	$(CFG_DIR)/utils/traverse.h  \
	$(CFG_DIR)/xcfg-utils.h      \
	$(CFG_DIR)/xcfg-types.h      \
	$(CFG_DIR)/xcfg-rtti.h       \
	$(CFG_DIR)/xcfg-tree.h       \
	$(CFG_DIR)/xcfg-file.h       \
	$(CFG_DIR)/xcfg-api.h

CFG_OBJ = \
	$(CFG_DIR)/utils/hashtable.o \
	$(CFG_DIR)/utils/traverse.o  \
	$(CFG_DIR)/xcfg-types.o      \
	$(CFG_DIR)/xcfg-tree.o       \
	$(CFG_DIR)/xcfg-file.o       \
	$(CFG_DIR)/xcfg-api.o

CFG_CCF = -I$(CFG_DIR)

$(CFG_DIR)/%.o: $(CFG_DIR)/%.c $(CFG_DEP)
	$(CC) -c -o $@ $< $(CFG_CCF) $(CCF)

$(CFG_LIB): $(CFG_OBJ)
	$(AR) rcs $@ $^

TEST_BIN = test
TEST_DEP = test-cfg.h
TEST_OBJ = test-cfg.o test.o
TEST_CCF = -I. -L. -l:$(CFG_LIB) $(CFG_CCF)

%.o: %.c $(TEST_DEP)
	$(CC) -c $< $(TEST_CCF) $(CCF) -o $@

$(TEST_BIN): $(TEST_OBJ)
	$(CC) $^ $(TEST_CCF) $(CCF) -o $@

.PHONY: build-lib build clean rebuild run

build-lib: $(CFG_LIB)

build: build-lib $(TEST_BIN)

clean:
	rm -f $(CFG_OBJ) $(CFG_LIB) $(TEST_OBJ) $(TEST_BIN)

rebuild: clean build

run: | build
	./$(TEST_BIN)