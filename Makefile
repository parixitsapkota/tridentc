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

# --- Information ---
VERSION       := 0.0.1 $(MODE)-$(shell date "+%d%m%Y")
TIME_INFO     := $(shell date "+%Y/%m/%d %H:%M:%S:%p")
COMPILER_INFO := $(shell $(CC) --version | head -n 1 | cut -d' ' -f1-3)

CFLAGS += -DVERSION_INFO="\"$(VERSION)\"" -DTIME_INFO="\"$(TIME_INFO)\""
CFLAGS += -DCC_INFO="\"$(COMPILER_INFO)\""

# --- Platform ---
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

SHI_SRC   := shi_arena.h shi_hs.h shi_flags.h shi_file.h
SHI_FILES := $(patsubst %.h, src/shi/%.h, $(SHI_SRC))

.PHONY: all dependency format

.DELETE_ON_ERROR:

all: format dependency $(OUTPUT)

dependency: $(SHI_FILES)

$(SHI_FILES):

# rule to download missing SHI headers
src/shi/%.h:
	@mkdir -p $(dir $@)
	@printf "$(COLOR_MAGENTA)[+] Downloading $@...$(COLOR_RESET)\n"
	@wget -q https://raw.githubusercontent.com/parixitsapkota/SHI/refs/heads/main/$(notdir $@) -O $@ || (rm -f $@ && exit 1)

# Link the main exe
$(OUTPUT): $(OBJECTS)
	@echo -e "$(COLOR_GREEN)[#] Linking $(OUTPUT) $(COLOR_BLUE)$(MODE)$(COLOR_GREEN) mode...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(OUTPUT)

$(OBJECTS):

# Compile sourcefile
$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo -e "$(COLOR_GREEN)[+] Compiling $<...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifact
clean:
	@echo -e "$(COLOR_BLUE)[-] Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf build/ src/shi/ examples/*.o examples/*.asm examples/*.bin $(PROJECT) $(PROJECT).exe

# Format sourcefile
format:
	@echo -e "$(COLOR_BLUE)[-] Formatting source files...$(COLOR_RESET)"
	@clang-format -i $(SRCFILES)

EXAMPLE ?= $(wildcard examples/*.b)

run:
	@printf "\n\n"
	@./$(OUTPUT) -v
	@printf "\n\n"
	@for file in $(EXAMPLE); do \
		name=$$(basename "$$file" .b); \
		printf "$(COLOR_MAGENTA)[+] Compiling $$file...$(COLOR_RESET)\n"; \
		./$(OUTPUT) -i "$$file" -o "examples/$$name.asm" || exit 1; \
		printf "$(COLOR_GREEN)[+] Assembling examples/$$name.asm...$(COLOR_RESET)\n"; \
		nasm -f elf64 "examples/$$name.asm" -o "examples/$$name.o" || exit 1; \
		printf "$(COLOR_YELLOW)[#] Linking examples/$$name.o...$(COLOR_RESET)\n"; \
		ld -o "examples/$$name.bin" "examples/$$name.o" || exit 1; \
		./examples/$$name.bin; status=$$?; \
		printf "$(COLOR_BLUE)[+] $$name.b : Exit-code : $(COLOR_RED)%d$(COLOR_RESET)\n\n" $$status; \
	done

.PHONY: all clean format
