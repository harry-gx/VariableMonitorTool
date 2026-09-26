CC = gcc
AR = ar
ARFLAGS ?= rcs
BUILD_DIR ?= ../build/vm_com_make
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

LIB := $(BUILD_DIR)/libvm_communication.a

SOURCES := \
    src/vm_communication.c \
    src/com/vm_value_codec.c \
    src/driver/vm_can_transport.c \
    src/protocol/vm_custom_protocol.c \
    src/protocol/vm_protocol_registry.c \
    src/route/vm_router.c \
    src/transport/vm_loopback_transport.c \
    src/transport/vm_serial_transport.c \
    src/transport/vm_transport_registry.c

OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SOURCES))

PUBLIC_HEADERS := \
    include/vm_communication.h \
    include/vm_status.h

PUBLIC_HEADER_OUTPUTS := $(patsubst include/%,$(PUBLIC_INCLUDE_DIR)/%,$(PUBLIC_HEADERS))

TEST_EXES := \
    $(TEST_DIR)/vm_test_registry$(EXE_EXT) \
    $(TEST_DIR)/vm_test_custom_protocol$(EXE_EXT) \
    $(TEST_DIR)/vm_test_loopback$(EXE_EXT)

.PHONY: all lib install_headers test clean

all: lib

lib: $(LIB) install_headers

install_headers: $(PUBLIC_HEADER_OUTPUTS)

$(PUBLIC_INCLUDE_DIR)/%.h: include/%.h
	$(call make_dir,$(@D))
	$(call copy_file,$<,$@)

$(LIB): $(OBJECTS)
	$(call make_dir,$(@D))
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: %.c
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(TEST_DIR)/vm_test_registry$(EXE_EXT): test/vm_test_registry.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(TEST_DIR)/vm_test_custom_protocol$(EXE_EXT): test/vm_test_custom_protocol.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

$(TEST_DIR)/vm_test_loopback$(EXE_EXT): test/vm_test_loopback.c $(LIB)
	$(call make_dir,$(@D))
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIB) $(LDFLAGS)

test: all $(TEST_EXES)
	$(TEST_DIR)/vm_test_registry$(EXE_EXT)
	$(TEST_DIR)/vm_test_custom_protocol$(EXE_EXT)
	$(TEST_DIR)/vm_test_loopback$(EXE_EXT)

clean:
	$(call remove_dir,$(BUILD_DIR))
