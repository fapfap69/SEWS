CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lpthread -lssl -lcrypto

SRCDIR = src
OBJDIR = obj
WWWDIR = www
TESTDIR = tests

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(OBJDIR)/%.o)
TARGET = sews
TEST_TARGET = run_tests

.PHONY: all clean setup test

all: setup $(TARGET)

setup:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(WWWDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(OBJDIR)/main.o, $(OBJECTS))
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_TARGET)

install-www:
	@echo "Installing static files..."
	@mkdir -p $(WWWDIR)
	@cp -r www/* $(WWWDIR)/
