CC = gcc
CFLAGS = -Wall -g -Iinclude
OBJDIR = build
SRCDIR = src
BINDIR = .
TARGET = $(BINDIR)/RonDB

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run, test, clean

run:
	make && ./RonDB mydb.db
clean:
		rm RonDB
		rm mydb.db
		rm test.db
		rm -rf build/*
test:
	make && bundle exec rspec
