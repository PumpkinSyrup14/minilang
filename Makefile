CC      = gcc
CFLAGS  = -Wall -Wno-unused-function -g
SRC     = src
BUILD   = build
TARGET  = minilang

.PHONY: all clean test

all: $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/parser.tab.c $(BUILD)/parser.tab.h: $(SRC)/parser.y | $(BUILD)
	bison -d -o $(BUILD)/parser.tab.c $(SRC)/parser.y

$(BUILD)/lex.yy.c: $(SRC)/lexer.l $(BUILD)/parser.tab.h | $(BUILD)
	flex -o $(BUILD)/lex.yy.c $(SRC)/lexer.l

$(TARGET): $(BUILD)/parser.tab.c $(BUILD)/lex.yy.c $(SRC)/ast.c $(SRC)/symtab.c $(SRC)/semantic.c $(SRC)/main.c
	$(CC) $(CFLAGS) -I$(SRC) -I$(BUILD) -o $(TARGET) \
		$(BUILD)/parser.tab.c $(BUILD)/lex.yy.c \
		$(SRC)/ast.c $(SRC)/symtab.c $(SRC)/semantic.c $(SRC)/main.c

clean:
	rm -rf $(BUILD) $(TARGET)

# run every sample program in tests/
test: $(TARGET)
	@for f in tests/*.ml; do \
		echo "===== $$f ====="; \
		./$(TARGET) $$f; \
		echo; \
	done
