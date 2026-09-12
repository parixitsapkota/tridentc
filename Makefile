# --- Colors ---
COLOR_RESET   := \033[0m
COLOR_RED     := \033[1;31m
COLOR_GREEN   := \033[1;32m
COLOR_YELLOW  := \033[1;33m
COLOR_BLUE    := \033[1;34m
COLOR_MAGENTA := \033[1;35m

# --- Configuration ---
PROJECT := trident
CC      = clang

DEBUG   := -fsanitize=address -g -O0
RELEASE := -O3
CFLAGS  := -Isrc -Wall -Wextra -Werror
LDFLAGS :=

MODE    ?= debug
BUILD   ?=

ifeq ($(MODE),release)
  CFLAGS += $(RELEASE)
  BUILD  := build/release
else
  CFLAGS += $(DEBUG)
  BUILD  := build/debug
endif

PLATFORM ?= linux
OUTPUT   ?=

ifeq ($(PLATFORM),linux)
  CFLAGS += -D_TUX
	OUTPUT = $(PROJECT)

else ifeq ($(PLATFORM),macos)
  CC     := clang
  CFLAGS += -D_XOS
  OUTPUT := $(PROJECT)

else ifeq ($(PLATFORM),freebsd)
  CC     := clang
  CFLAGS += -D_BSD
  OUTPUT := $(PROJECT)

else ifeq ($(PLATFORM),windows)
	CC = x86_64-w64-mingw32-gcc
  CFLAGS += -D_WIN32 -mwindows
	OUTPUT = $(PROJECT).exe
endif

C_SOURCES := $(wildcard src/*.c)
H_HEADERS := $(wildcard src/*.h)
SRCFILES  := $(C_SOURCES) $(H_HEADERS)

OBJECTS := $(patsubst src/%.c, $(BUILD)/%.o, $(C_SOURCES))

all: format $(OUTPUT)

# Link the main exe
$(OUTPUT): $(OBJECTS)
	@echo "$(COLOR_GREEN)[#] Linking $(OUTPUT) $(COLOR_BLUE)$(MODE)$(COLOR_GREEN) mode...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(OUTPUT)

$(OBJECTS):

# Compile sourcefile
$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "$(COLOR_GREEN)[+] Compiling $<...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifact
clean:
	@echo "$(COLOR_BLUE)[-] Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf build/ examples/*.o examples/*.asm examples/*.out $(PROJECT) $(PROJECT).exe

# Format sourcefile
format:
	@echo "$(COLOR_BLUE)[-] Formatting source files...$(COLOR_RESET)"
	@clang-format -i $(SRCFILES)

EXAMPLE    ?= $(wildcard examples/*.b)
run:
	@printf "\n\n"; 
	@for file in $(EXAMPLE); do \
		base=$$(basename $$file); \
		printf "$(COLOR_MAGENTA)[+] Compiling $$file...$(COLOR_RESET)\n"; \
		./$(OUTPUT) $$file || exit 1; \
		printf "$(COLOR_GREEN)[+] Assembling examples/$$base.asm...$(COLOR_RESET)\n"; \
		nasm -f elf64 examples/$$base.asm -o examples/$$base.o || exit 1; \
		printf "$(COLOR_YELLOW)[#] Linking examples/$$base.o...$(COLOR_RESET)\n"; \
		ld -o examples/$$base.out examples/$$base.o || exit 1; \
		./examples/$$base.out; status=$$?; \
		printf "$(COLOR_BLUE)[+] $$base.out : Exit-code : $(COLOR_RED)%d$(COLOR_RESET)\n\n" $$status; \
	done

.PHONY: all clean format
