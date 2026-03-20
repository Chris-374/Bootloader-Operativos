[bits 16]
[org 0x7E00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    sti

main_menu:
    call clear_screen
    call draw_welcome

.wait_confirm:
    xor ah, ah
    int 16h                ; espera tecla

    cmp al, 13             ; ENTER
    je start_game

    cmp al, 27             ; ESC
    je exit_program

    jmp .wait_confirm

start_game:
    mov byte [orientation], 0
    call randomize_position
    call draw_scene

game_loop:
    xor ah, ah
    int 16h                ; espera tecla
                           ; AL = ASCII, AH = scan code

    cmp al, 27             ; ESC
    je exit_program

    cmp al, 'r'
    je restart_game
    cmp al, 'R'
    je restart_game

    cmp ah, 4Bh            ; left
    je set_left
    cmp ah, 4Dh            ; right
    je set_right
    cmp ah, 48h            ; up
    je set_up
    cmp ah, 50h            ; down
    je set_down

    jmp game_loop

restart_game:
    mov byte [orientation], 0
    call randomize_position
    call draw_scene
    jmp game_loop

set_left:
    mov byte [orientation], 1
    call draw_scene
    jmp game_loop

set_right:
    mov byte [orientation], 2
    call draw_scene
    jmp game_loop

set_up:
    mov byte [orientation], 3
    call draw_scene
    jmp game_loop

set_down:
    mov byte [orientation], 4
    call draw_scene
    jmp game_loop

exit_program:
    call clear_screen
    mov dh, 10
    mov dl, 28
    call set_cursor
    mov si, msg_bye
    call print_string
.hang:
    cli
    hlt
    jmp .hang

; --------------------------------------------------
; Dibujo principal
; orientation:
; 0 = normal
; 1 = left  (vertical upward)
; 2 = right (vertical downward)
; 3 = up    (horizontal reversed)
; 4 = down  (180 approx: horizontal reversed + names swapped)
; --------------------------------------------------

draw_scene:
    call clear_screen
    call draw_header

    mov al, [orientation]
    cmp al, 0
    je draw_normal
    cmp al, 1
    je draw_left
    cmp al, 2
    je draw_right
    cmp al, 3
    je draw_up
    cmp al, 4
    je draw_down

    ret

draw_normal:
    mov dh, [rand_row]
    mov dl, [rand_col]
    call set_cursor
    mov si, name1
    call print_string

    mov dh, [rand_row]
    inc dh
    mov dl, [rand_col]
    call set_cursor
    mov si, name2
    call print_string
    ret

draw_left:
    ; nombre1 vertical hacia arriba
    mov al, [rand_row]
    add al, name1_len_minus1
    mov [temp_row], al
    mov al, [rand_col]
    mov [temp_col], al
    mov si, name1
    call print_vertical_up

    ; nombre2 vertical hacia arriba, separado 2 columnas
    mov al, [rand_row]
    add al, name2_len_minus1
    mov [temp_row], al
    mov al, [rand_col]
    add al, 2
    mov [temp_col], al
    mov si, name2
    call print_vertical_up
    ret

draw_right:
    ; nombre1 vertical hacia abajo
    mov al, [rand_row]
    mov [temp_row], al
    mov al, [rand_col]
    mov [temp_col], al
    mov si, name1
    call print_vertical_down

    ; nombre2 vertical hacia abajo, separado 2 columnas
    mov al, [rand_row]
    mov [temp_row], al
    mov al, [rand_col]
    add al, 2
    mov [temp_col], al
    mov si, name2
    call print_vertical_down
    ret

draw_up:
    ; aproximación: invertir horizontalmente
    mov dh, [rand_row]
    mov dl, [rand_col]
    call set_cursor
    mov si, name1
    call print_string_reverse

    mov dh, [rand_row]
    inc dh
    mov dl, [rand_col]
    call set_cursor
    mov si, name2
    call print_string_reverse
    ret

draw_down:
    ; aproximación de 180: invertir y además intercambiar orden de líneas
    mov dh, [rand_row]
    mov dl, [rand_col]
    call set_cursor
    mov si, name2
    call print_string_reverse

    mov dh, [rand_row]
    inc dh
    mov dl, [rand_col]
    call set_cursor
    mov si, name1
    call print_string_reverse
    ret

; --------------------------------------------------
; Pantallas
; --------------------------------------------------

draw_welcome:
    mov dh, 4
    mov dl, 24
    call set_cursor
    mov si, title_msg
    call print_string

    mov dh, 8
    mov dl, 16
    call set_cursor
    mov si, welcome_msg1
    call print_string

    mov dh, 10
    mov dl, 16
    call set_cursor
    mov si, welcome_msg2
    call print_string

    mov dh, 13
    mov dl, 16
    call set_cursor
    mov si, controls_msg1
    call print_string

    mov dh, 14
    mov dl, 16
    call set_cursor
    mov si, controls_msg2
    call print_string

    mov dh, 15
    mov dl, 16
    call set_cursor
    mov si, controls_msg3
    call print_string

    mov dh, 16
    mov dl, 16
    call set_cursor
    mov si, controls_msg4
    call print_string

    mov dh, 17
    mov dl, 16
    call set_cursor
    mov si, controls_msg5
    call print_string

    mov dh, 20
    mov dl, 16
    call set_cursor
    mov si, confirm_msg
    call print_string
    ret

draw_header:
    mov dh, 0
    mov dl, 0
    call set_cursor
    mov si, hud_msg
    call print_string
    ret

; --------------------------------------------------
; BIOS utilidades
; --------------------------------------------------

clear_screen:
    mov ax, 0x0003     ; modo texto 80x25
    int 10h
    ret

set_cursor:
    ; DH=row, DL=col
    mov ah, 0x02
    mov bh, 0x00
    int 10h
    ret

print_char:
    ; AL = char
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 10h
    ret

print_string:
.next:
    lodsb
    or  al, al
    jz  .done
    call print_char
    jmp .next
.done:
    ret

print_string_reverse:
    push si
    call find_end
    dec si
.rev_loop:
    cmp si, [string_start_ptr]
    jb  .done
    mov al, [si]
    call print_char
    dec si
    jmp .rev_loop
.done:
    pop si
    ret

find_end:
    mov [string_start_ptr], si
.find_loop:
    lodsb
    or  al, al
    jnz .find_loop
    dec si
    ret

print_vertical_down:
.next:
    lodsb
    or  al, al
    jz  .done

    mov dh, [temp_row]
    mov dl, [temp_col]
    call set_cursor
    call print_char

    inc byte [temp_row]
    jmp .next
.done:
    ret

print_vertical_up:
.next:
    lodsb
    or  al, al
    jz  .done

    mov dh, [temp_row]
    mov dl, [temp_col]
    call set_cursor
    call print_char

    dec byte [temp_row]
    jmp .next
.done:
    ret

; --------------------------------------------------
; Random BIOS usando ticks
; genera fila y columna válidas
; --------------------------------------------------

randomize_position:
.row_try:
    mov ah, 00h
    int 1Ah
    mov al, dl
    and al, 0x0F         ; 0..15
    add al, 5            ; 5..20
    mov [rand_row], al

.col_try:
    mov ah, 00h
    int 1Ah
    mov al, dl
    and al, 0x1F         ; 0..31
    add al, 20           ; 20..51 aprox
    mov [rand_col], al
    ret

; --------------------------------------------------
; Datos
; --------------------------------------------------

orientation db 0
rand_row    db 8
rand_col    db 30
temp_row    db 0
temp_col    db 0
string_start_ptr dw 0

title_msg      db 'MY NAME - LEGACY X86',0
welcome_msg1   db 'This version runs in BIOS legacy mode.',0
welcome_msg2   db 'Press ENTER to start or ESC to exit.',0
controls_msg1  db 'LEFT  = rotate left',0
controls_msg2  db 'RIGHT = rotate right',0
controls_msg3  db 'UP    = upper transform',0
controls_msg4  db 'DOWN  = lower transform',0
controls_msg5  db 'R = restart   ESC = exit',0
confirm_msg    db 'Waiting for confirmation...',0
hud_msg        db 'Arrows=transform  R=restart  ESC=exit',0
msg_bye        db 'Program finished.',0

name1 db 'CHRISTIAN',0
name2 db 'JORGE',0

name1_len_minus1 equ 7
name2_len_minus1 equ 6

times 20480-($-$$) db 0