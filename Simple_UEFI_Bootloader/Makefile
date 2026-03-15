SHELL := /bin/bash

# =========================
# Configuración
# =========================
CURDIR_ABS := $(CURDIR)
BACKEND_DIR := ../Backend
GCC_FOLDER_NAME := /usr
BINUTILS_FOLDER_NAME := /usr
EFI_FOLDER_NAME := gnu-efi-3.0.9
LINKER_SCRIPT := $(EFI_FOLDER_NAME)/gnuefi/elf_x86_64_efi.lds

CC := $(GCC_FOLDER_NAME)/bin/gcc
OBJCOPY := $(BINUTILS_FOLDER_NAME)/bin/objcopy
SIZE := $(BINUTILS_FOLDER_NAME)/bin/size

export PATH := $(BINUTILS_FOLDER_NAME)/bin:$(PATH)

# =========================
# Includes
# =========================
BASE_HFILES := -I$(CURDIR_ABS)/inc/ -I$(CURDIR_ABS)/startup/
EXTRA_HFILES := $(shell sed 's|^|-I|' h_files.txt)
HFILES := $(BASE_HFILES) $(EXTRA_HFILES)

# =========================
# Flags
# =========================
CFLAGS_COMMON := -DGNU_EFI_USE_MS_ABI \
	-mno-avx \
	-mcmodel=small \
	-mno-stack-arg-probe \
	-m64 \
	-mno-red-zone \
	-maccumulate-outgoing-args \
	-Og \
	-ffreestanding \
	-fshort-wchar \
	-fpie \
	-fomit-frame-pointer \
	-fno-delete-null-pointer-checks \
	-fno-zero-initialized-in-bss \
	-fno-common \
	-fno-exceptions \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-strict-aliasing \
	-fno-merge-all-constants \
	-fno-merge-constants \
	--std=c11 \
	-g3 \
	-Wall \
	-Wextra \
	-Wdouble-promotion \
	-fmessage-length=0 \
	-c \
	-MMD \
	-MP \
	$(HFILES)

CFLAGS_USER := $(CFLAGS_COMMON) -Wpedantic
CFLAGS_BACKEND := $(CFLAGS_COMMON)

LDFLAGS := -T$(LINKER_SCRIPT) \
	-nostdlib \
	-s \
	-static-pie \
	-Wl,--warn-common \
	-Wl,--no-undefined \
	-Wl,-z,text \
	-Wl,-z,norelro \
	-Wl,-z,now \
	-Wl,-Map=output.map

# =========================
# Archivos
# =========================
BACKEND_CFILES := $(shell cat c_files_linux.txt)
BACKEND_OFILES := $(patsubst %.c,%.o,$(BACKEND_CFILES))
BACKEND_DFILES := $(patsubst %.c,%.d,$(BACKEND_CFILES))
BACKEND_OUTFILES := $(patsubst %.c,%.out,$(BACKEND_CFILES))

USER_CFILES := $(wildcard src/*.c)
USER_OFILES := $(patsubst %.c,%.o,$(USER_CFILES))
USER_DFILES := $(patsubst %.c,%.d,$(USER_CFILES))
USER_OUTFILES := $(patsubst %.c,%.out,$(USER_CFILES))

ALL_OFILES := $(BACKEND_OFILES) $(USER_OFILES)
ALL_DFILES := $(BACKEND_DFILES) $(USER_DFILES)

# =========================
# Targets principales
# =========================
.PHONY: all clean recompile showvars prepare_lists

all: prepare_lists $(BACKEND_DIR)/BOOTX64.EFI

prepare_lists:
	@perl -pi -e 's/\r\n/\n/g' c_files_linux.txt
	@perl -pi -e 's/\r\n/\n/g' h_files.txt

$(BACKEND_DIR)/BOOTX64.EFI: $(BACKEND_DIR)/program.so
	@echo
	@echo Generating binary and Printing size information:
	@echo
	cd $(BACKEND_DIR) && \
	$(OBJCOPY) \
	-j .text \
	-j .sdata \
	-j .data \
	-j .dynamic \
	-j .dynsym \
	-j .rel \
	-j .rela \
	-j .rel.* \
	-j .rela.* \
	-j .rel* \
	-j .rela* \
	-j .reloc \
	--target=efi-app-x86_64 \
	program.so BOOTX64.EFI
	@$(SIZE) $(BACKEND_DIR)/BOOTX64.EFI
	@echo

$(BACKEND_DIR)/program.so: $(ALL_OFILES)
	@echo "Linking program.so..."
	cd $(BACKEND_DIR) && \
	$(CC) $(LDFLAGS) -o program.so $(addprefix ../,$(ALL_OFILES))

# =========================
# Compilación backend
# =========================
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_BACKEND) -Wa,-adhln="$(basename $@).out" -MF"$(basename $@).d" -MT"$@" -o "$@" "$<"

# =========================
# Compilación user src/
# =========================
src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_USER) -Wa,-adhln="src/$*.out" -MF"src/$*.d" -MT"src/$*.o" -o "src/$*.o" "src/$*.c"

# =========================
# Utilidades
# =========================
clean:
	@echo "Cleaning..."
	@rm -f $(BACKEND_OFILES) $(BACKEND_DFILES) $(BACKEND_OUTFILES)
	@rm -f $(USER_OFILES) $(USER_DFILES) $(USER_OUTFILES)
	@rm -f $(BACKEND_DIR)/program.so
	@rm -f $(BACKEND_DIR)/BOOTX64.EFI
	@rm -f $(BACKEND_DIR)/output.map
	@rm -f $(BACKEND_DIR)/objects.list

recompile: clean all

showvars:
	@echo "CURDIR_ABS=$(CURDIR_ABS)"
	@echo "BACKEND_DIR=$(BACKEND_DIR)"
	@echo "BACKEND_CFILES=$(BACKEND_CFILES)"
	@echo "USER_CFILES=$(USER_CFILES)"

-include $(ALL_DFILES)