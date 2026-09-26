CC = gcc
AR = ar
ARFLAGS ?= rcs
BUILD_DIR ?= ../build/vm_elf_parser_make
OBJ_DIR := $(BUILD_DIR)/obj
TEST_DIR := $(BUILD_DIR)/test
PUBLIC_INCLUDE_DIR := $(BUILD_DIR)/include
EXE_EXT :=

ifeq ($(OS),Windows_NT)
EXE_EXT := .exe
endif

VM_POSIX_MAKE :=

ifneq ($(MSYSTEM),)
VM_POSIX_MAKE := 1
endif

ifneq ($(OSTYPE),)
ifneq (,$(findstring msys,$(OSTYPE)))
VM_POSIX_MAKE := 1
endif
ifneq (,$(findstring cygwin,$(OSTYPE)))
VM_POSIX_MAKE := 1
endif
endif

ifneq ($(OS),Windows_NT)
VM_POSIX_MAKE := 1
endif

ifeq ($(VM_POSIX_MAKE),1)
MKDIR_P ?= mkdir -p
RM_RF ?= rm -rf
CP_F ?= cp -f

define make_dir
	@$(MKDIR_P) "$(1)"
endef

define remove_dir
	@$(RM_RF) "$(1)"
endef

define copy_file
	@$(CP_F) "$(1)" "$(2)"
endef
else
define make_dir
	@if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
endef

define remove_dir
	@if exist "$(subst /,\,$(1))" rmdir /s /q "$(subst /,\,$(1))"
endef

define copy_file
	@copy /Y "$(subst /,\,$(1))" "$(subst /,\,$(2))" > nul
endef
endif

CFLAGS ?= -std=c11 -O2
CFLAGS += -D__USE_MINGW_ANSI_STDIO=1 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -Iinclude -Iprivate
CLI_CPPFLAGS := -Iinclude

LIB := $(BUILD_DIR)/libvm_elf_parser.a
CLI := $(BUILD_DIR)/vm_elf_parser_cli$(EXE_EXT)

SOURCES := \
    src/vm_adapters.c \
    src/vm_dwarf.c \
    src/vm_dwarf_abbrev.c \
    src/vm_dwarf_die.c \
    src/vm_dwarf_form.c \
    src/vm_elf.c \
    src/vm_monitor_variables.c \
    src/vm_reader.c \
    src/vm_registry.c \
    src/vm_type_graph.c

OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))
CLI_OBJECT := $(OBJ_DIR)/cli/vm_elf_parser_cli.o

PUBLIC_HEADERS := \
    include/vm_monitor_variables.h \
    include/vm_status.h

PUBLIC_HEADER_OUTPUTS := $(patsubst include/%,$(PUBLIC_INCLUDE_DIR)/%,$(PUBLIC_HEADERS))

TEST_EXES := \
    $(TEST_DIR)/vm_test_dwarf$(EXE_EXT) \
    $(TEST_DIR)/vm_test_values$(EXE_EXT) \
    $(TEST_DIR)/vm_test_type_graph$(EXE_EXT) \
    $(TEST_DIR)/vm_test_samples$(EXE_EXT)

.PHONY: all lib cli install_headers test clean

all: lib cli

lib: $(LIB) install_headers

cli: $(CLI)

install_headers: $(PUBLIC_HEADER_OUTPUTS)

$(PUBLIC_INCLUDE_DIR)/%.h: include/%.h
	$(call make_dir,$(@D))
	$(call copy_file,$<,$@)

$(LIB): $(OBJECTS)
	$(call make_dir,$(@D))
	$(AR) $(ARFLAGS) $@ $^

$(CLI): $(CLI_OBJECT) $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(CLI_OBJECT) $(LIB) $(LDFLAGS)

$(CLI_OBJECT): cli/vm_elf_parser_cli.c
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CLI_CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(TEST_DIR)/vm_test_dwarf$(EXE_EXT): test/vm_test_dwarf.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(TEST_DIR)/vm_test_values$(EXE_EXT): test/vm_test_values.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(TEST_DIR)/vm_test_type_graph$(EXE_EXT): test/vm_test_type_graph.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(TEST_DIR)/vm_test_samples$(EXE_EXT): test/integration/vm_test_samples.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

test: all $(TEST_EXES)
	$(TEST_DIR)/vm_test_dwarf$(EXE_EXT)
	$(TEST_DIR)/vm_test_values$(EXE_EXT)
	$(TEST_DIR)/vm_test_type_graph$(EXE_EXT)
	$(TEST_DIR)/vm_test_samples$(EXE_EXT) ../sample/APP.elf 4
	$(TEST_DIR)/vm_test_samples$(EXE_EXT) ../sample/MC_SixStep_DualLoop.axf 3

clean:
	$(call remove_dir,$(BUILD_DIR))
