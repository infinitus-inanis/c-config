CC = gcc
AR = ar

CCF = -Werror -Wall -Wextra

CFG_DIR = lib
CFG_LIB = $(CFG_DIR)/libcfg.a
CFG_DEP = $(CFG_DIR)/cfg-base.h
CFG_OBJ = $(CFG_DIR)/cfg-base.o
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