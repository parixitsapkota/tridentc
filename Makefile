# --- Colors ---
COLOR_RESET  := \033[0m
COLOR_RED    := \033[1;31m
COLOR_GREEN  := \033[1;32m
COLOR_YELLOW := \033[1;33m
COLOR_BLUE   := \033[1;34m

# --- Configuration ---
PROJECT := trident
CC      := clang

DEBUG   := -fsanitize=address -g -O0
RELEASE := -O3
CFLAGS  := -Isrc -Wall -Wextra -Werror

MODE    ?= debug
BUILD   ?=

ifeq ($(MODE),release)
    CFLAGS += $(RELEASE)
    BUILD  := build/release
else
    CFLAGS += $(DEBUG)
    BUILD  := build/debug
endif

KEYWORDS  := src/keywords.h
C_SOURCES := $(wildcard src/*.c)
H_HEADERS := $(filter-out $(KEYWORDS), $(wildcard src/*.h))
SRCFILES  := $(C_SOURCES) $(H_HEADERS)

OBJECTS := $(patsubst src/%.c, $(BUILD)/%.o, $(C_SOURCES))

all: format $(PROJECT)

# Link the main exe
$(PROJECT): $(OBJECTS)
	@echo "$(COLOR_GREEN)[#] Linking $(PROJECT) $(COLOR_BLUE)$(MODE)$(COLOR_GREEN) mode...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(PROJECT)

$(KEYWORDS): res/keywords.gperf
	@echo "$(COLOR_YELLOW)[*] Gperf generating word_set...$(COLOR_RESET)"
	@gperf -N get_keyword_kind -t res/keywords.gperf > $(KEYWORDS)

$(OBJECTS): | $(KEYWORDS)

# Compile sourcefile
$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "$(COLOR_GREEN)[+] Compiling $<...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "$(COLOR_BLUE)[-] Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf build/ asm/ src/keywords.h a.out $(PROJECT)

# Format sourcefile
format:
	@echo "$(COLOR_BLUE)[-] Formatting source files...$(COLOR_RESET)"
	@clang-format -i $(SRCFILES)

.PHONY: all clean format
