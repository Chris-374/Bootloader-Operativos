SHELL := /bin/bash

#
# Configuración de carpetas y rutas
# Aquí definimos dónde están los archivos del backend y usamos el compilador 
# GCC nativo de Linux para cumplir con el requerimiento de la tarea.
#
CURDIR_ABS := $(CURDIR)
BACKEND_DIR := ../Backend
GCC_FOLDER_NAME := /usr
BINUTILS_FOLDER_NAME := /usr
EFI_FOLDER_NAME := gnu-efi-3.0.9
LINKER_SCRIPT := $(EFI_FOLDER_NAME)/gnuefi/elf_x86_64_efi.lds

CC := $(GCC_FOLDER_NAME)/bin/gcc
SIZE := $(BINUTILS_FOLDER_NAME)/bin/size
OBJCOPY := $(BINUTILS_FOLDER_NAME)/bin/objcopy

export PATH := $(BINUTILS_FOLDER_NAME)/bin:$(PATH)

#
# Búsqueda de los archivos .h (headers)
#
BASE_HFILES := -I$(CURDIR_ABS)/inc/ -I$(CURDIR_ABS)/startup/
EXTRA_HFILES := $(shell sed '/^[[:space:]]*$$/d' h_files.txt | sed 's|^|$(BACKEND_DIR)/|' | sed 's|^|-I|')
HFILES := $(BASE_HFILES) $(EXTRA_HFILES)

#
# Banderas del compilador (CFLAGS)
# Son súper importantes porque configuran GCC para compilar código "freestanding"
# (sin la librería estándar de C de Linux) y usando la convención de llamadas de Microsoft (MS ABI), 
# que es lo que espera la BIOS UEFI.
#
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

CFLAGS_BACKEND := $(CFLAGS_COMMON)
CFLAGS_USER := $(CFLAGS_COMMON) -Wpedantic

# Banderas para archivos en ensamblador (.S)
ASFLAGS_BACKEND := -DGNU_EFI_USE_MS_ABI \
    -m64 \
    -maccumulate-outgoing-args \
    --std=c11 \
    -c

#
# Banderas del Linker (LDFLAGS)
# Le dicen al enlazador cómo juntar todo usando el linker script especial para EFI.
#
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

# Extracción de archivos a compilar desde archivos de texto
RAW_BACKEND_FILES := $(shell sed '/^[[:space:]]*$$/d' c_files_linux.txt)

BACKEND_C_REL := $(filter %.c,$(RAW_BACKEND_FILES))
BACKEND_S_REL := $(filter %.S,$(RAW_BACKEND_FILES))

BACKEND_C_SRC := $(addprefix $(BACKEND_DIR)/,$(BACKEND_C_REL))
BACKEND_S_SRC := $(addprefix $(BACKEND_DIR)/,$(BACKEND_S_REL))

BACKEND_C_OBJ := $(patsubst %.c,%.o,$(BACKEND_C_SRC))
BACKEND_S_OBJ := $(patsubst %.S,%.o,$(BACKEND_S_SRC))

BACKEND_C_DEP := $(patsubst %.c,%.d,$(BACKEND_C_SRC))
USER_CFILES := $(wildcard src/*.c)
USER_OFILES := $(patsubst %.c,%.o,$(USER_CFILES))
USER_DFILES := $(patsubst %.c,%.d,$(USER_CFILES))

ALL_BACKEND_OBJ_REL := $(patsubst $(BACKEND_DIR)/%,%,$(BACKEND_C_OBJ) $(BACKEND_S_OBJ))
PROJECT_DIR_NAME := $(notdir $(CURDIR_ABS))
USER_OFILES_FOR_LINK := $(addprefix ../$(PROJECT_DIR_NAME)/,$(USER_OFILES))
ALL_LINK_OBJECTS := $(ALL_BACKEND_OBJ_REL) $(USER_OFILES_FOR_LINK)

.PHONY: all clean recompile prepare_lists showvars

#
# Regla principal (all)
# Es el comando que se ejecuta cuando solo pones "make" en la terminal.
# Genera el archivo final booteable.
#
all: prepare_lists $(BACKEND_DIR)/BOOTX64.EFI

prepare_lists:
    @perl -pi -e 's/\r\n/\n/g' c_files_linux.txt
    @perl -pi -e 's/\r\n/\n/g' h_files.txt

#
# Generación del ejecutable EFI
# Toma el archivo .so compilado y usa objcopy para extraer solo las secciones 
# necesarias y empaquetarlas en el formato de una app efi-app-x86_64.
#
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

#
# Enlazado (Linking)
# Junta todos los objetos (.o) del usuario y del backend en un solo Shared Object (.so)
#
$(BACKEND_DIR)/program.so: $(BACKEND_C_OBJ) $(BACKEND_S_OBJ) $(USER_OFILES)
    @echo "Linking program.so..."
    cd $(BACKEND_DIR) && \
    $(CC) $(LDFLAGS) -o program.so $(ALL_LINK_OBJECTS)

#
# Compilación de los archivos C del backend a objetos (.o)
#
$(BACKEND_DIR)/%.o: $(BACKEND_DIR)/%.c
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS_BACKEND) \
        -Wa,-adhln="$(basename $@).out" \
        -MF"$(basename $@).d" \
        -MT"$@" \
        -o "$@" "$<"

#
# Compilación de los archivos Ensamblador del backend
#
$(BACKEND_DIR)/%.o: $(BACKEND_DIR)/%.S
    @mkdir -p $(dir $@)
    $(CC) $(ASFLAGS_BACKEND) -o "$@" "$<"

#
# Compilación de tus archivos C (src/*.c)
#
src/%.o: src/%.c
    @mkdir -p $(dir $@)
    $(CC) $(CFLAGS_USER) \
        -Wa,-adhln="src/$*.out" \
        -MF"src/$*.d" \
        -MT"src/$@" \
        -o "src/$*.o" "src/$*.c"

#
# Limpieza (make clean)
# Borra todos los archivos generados (.o, .d, .EFI, etc) para compilar desde cero la próxima vez.
#
clean:
    @echo "Cleaning..."
    @find . -name "*.d" -delete
    @find . -name "*.o" -delete
    @find . -name "*.out" -delete
    @rm -f $(BACKEND_DIR)/program.so
    @rm -f $(BACKEND_DIR)/BOOTX64.EFI
    @rm -f $(BACKEND_DIR)/output.map

#
# Recompilar todo (make recompile)
# Llama a clean y luego a all
#
recompile: clean all

showvars:
    @echo "BACKEND_C_REL=$(BACKEND_C_REL)"
    @echo "BACKEND_S_REL=$(BACKEND_S_REL)"
    @echo "USER_CFILES=$(USER_CFILES)"

-include $(wildcard $(BACKEND_C_DEP) $(USER_DFILES))