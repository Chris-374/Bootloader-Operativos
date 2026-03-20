[bits 16]
[org 0x7c00]

SECOND_STAGE_LOADING_ADDRESS equ 0x7E00
SECOND_STAGE_SECTOR_SIZE     equ 40

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7BFF
    sti

    ; BIOS drive number viene en DL, guárdalo
    mov [boot_drive], dl

    ; leer segunda etapa desde disco
    mov ah, 0x02
    mov al, SECOND_STAGE_SECTOR_SIZE
    mov ch, 0x00
    mov cl, 0x02
    mov dh, 0x00
    mov dl, [boot_drive]
    mov bx, SECOND_STAGE_LOADING_ADDRESS
    int 0x13
    jc disk_error

    jmp 0x0000:SECOND_STAGE_LOADING_ADDRESS

disk_error:
    mov si, msg_disk_error
.print:
    lodsb
    or  al, al
    jz  .hang
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .print

.hang:
    cli
    hlt
    jmp .hang

boot_drive db 0
msg_disk_error db 'Disk read error', 0

times 510 - ($-$$) db 0
dw 0xAA55