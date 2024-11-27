CC = gcc
AR = ar

CCF = \
	-Wall -Wextra \
	-g -ggdb -gdwarf \
	-fsanitize=address -fno-omit-frame-pointer -static-libasan -lrt

CFG_DIR = lib
CFG_LIB = $(CFG_DIR)/libxcfg.a

CFG_DEP = \
	$(CFG_DIR)/inc/api/xcfg-utils.h     \
	$(CFG_DIR)/inc/api/xcfg-types.h     \
	$(CFG_DIR)/inc/api/xcfg-rtti.h      \
	$(CFG_DIR)/inc/api/xcfg.h           \
	$(CFG_DIR)/inc/api/xcfg-data.h      \
	$(CFG_DIR)/inc/api/xcfg-file.h      \
	$(CFG_DIR)/src/utils/hashtable.h    \
	$(CFG_DIR)/src/utils/traverse.h     \
	$(CFG_DIR)/src/utils/inotify-file.h \
	$(CFG_DIR)/src/utils/file-monitor.h \
	$(CFG_DIR)/src/xcfg-impl.h          \
	$(CFG_DIR)/src/xcfg-tree.h

CFG_OBJ = \
	$(CFG_DIR)/src/utils/hashtable.o    \
	$(CFG_DIR)/src/utils/traverse.o     \
	$(CFG_DIR)/src/utils/inotify-file.o \
	$(CFG_DIR)/src/utils/file-monitor.o \
	$(CFG_DIR)/src/xcfg-types.o         \
	$(CFG_DIR)/src/xcfg-tree.o          \
	$(CFG_DIR)/src/xcfg.o               \
	$(CFG_DIR)/src/xcfg-data.o          \
	$(CFG_DIR)/src/xcfg-file-save.o     \
	$(CFG_DIR)/src/xcfg-file-load.o     \
	$(CFG_DIR)/src/xcfg-file-monitor.o

CFG_CCF = -I$(CFG_DIR)/inc -I$(CFG_DIR)/inc/api -I$(CFG_DIR)/src

$(CFG_DIR)/%.o: $(CFG_DIR)/%.c $(CFG_DEP)
	$(CC) -c -o $@ $< $(CFG_CCF) $(CCF)

$(CFG_LIB): $(CFG_OBJ)
	$(AR) rcs $@ $^

TEST_BIN = test
TEST_DEP = sig-thread.h test-cfg.h
TEST_OBJ = sig-thread.o test-cfg.o test.o
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