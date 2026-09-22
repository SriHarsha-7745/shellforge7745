CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lreadline

SRC = src/main.c src/lexer.c src/token.c src/parser.c src/expand.c src/builtin.c src/pipeline_exec.c
TARGET = shellforge

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
