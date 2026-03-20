[bits 16]                      ; Indica que esta segunda etapa corre en 16 bits
[org 0x7E00]                   ; Código estará cargado en memoria en la dirección 0x7E00

start:                        
    cli                        ; Deshabilita interrupciones mientras se ajustan registros de segmento
    xor ax, ax                 ; AX = 0
    mov ds, ax                 ; DS = 0 para que los datos se lean desde el segmento base 0
    mov es, ax                 ; ES = 0 por consistencia
    sti                        ; Vuelve a habilitar interrupciones

main_menu:                     
    call clear_screen          ; Limpia la pantalla y deja modo texto 80x25
    call draw_welcome          ; Dibuja la pantalla de bienvenida e instrucciones

.wait_confirm:                 
    xor ah, ah                 ; AH = 0 para usar la función 00h de INT 16h (leer tecla)
    int 16h                    ; Llama al BIOS para esperar una tecla

    cmp al, 13                 ; Compara ASCII en AL con 13 (ENTER)
    je start_game              ; Si fue ENTER, salta para iniciar el juego

    cmp al, 27                 ; Compara ASCII en AL con 27 (ESC)
    je exit_program            ; Si fue ESC, sale del programa

    jmp .wait_confirm          ; Si fue otra tecla, sigue esperando

start_game:                    
    mov byte [orientation], 0  ; Pone la orientación inicial en 0 = normal
    call randomize_position    ; Genera fila y columna aleatorias para ubicar los nombres
    call draw_scene            ; Dibuja la escena inicial

game_loop:                     ; Bucle principal del juego
    xor ah, ah                 ; AH = 0 para leer tecla con BIOS
    int 16h                    ; Espera una tecla
                               

    cmp al, 27                 ; ¿ESC?
    je exit_program            ; Si sí, salir del programa

    cmp al, 'r'                ; ¿r minúscula?
    je restart_game            ; Si sí, reiniciar juego
    cmp al, 'R'                ; ¿R mayúscula?
    je restart_game            ; Si sí, reiniciar juego

    cmp ah, 4Bh                ; Scan code de flecha izquierda
    je set_left                ; Si sí, cambiar a orientación izquierda
    cmp ah, 4Dh                ; Scan code de flecha derecha
    je set_right               ; Si sí, cambiar a orientación derecha
    cmp ah, 48h                ; Scan code de flecha arriba
    je set_up                  ; Si sí, cambiar a orientación arriba
    cmp ah, 50h                ; Scan code de flecha abajo
    je set_down                ; Si sí, cambiar a orientación abajo

    jmp game_loop              ; Si fue otra tecla, seguir esperando

restart_game:                  
    mov byte [orientation], 0  
    call randomize_position    ; Genera nueva posición aleatoria
    call draw_scene            ; Redibuja escena
    jmp game_loop              ; Regresa al bucle principal

set_left:                      
    mov byte [orientation], 1  ; 1 = left
    call draw_scene            ; Redibuja escena con esa orientación
    jmp game_loop              ; Regresa al bucle principal

set_right:                     
    mov byte [orientation], 2  ; 2 = right
    call draw_scene            ; Redibuja escena
    jmp game_loop              ; Regresa al bucle principal

set_up:                        
    mov byte [orientation], 3  ; 3 = up
    call draw_scene            ; Redibuja escena
    jmp game_loop              ; Regresa al bucle principal

set_down:                      
    mov byte [orientation], 4  ; 4 = down
    call draw_scene            ; Redibuja escena
    jmp game_loop              ; Regresa al bucle principal

exit_program:                  
    call clear_screen          ; Limpia pantalla
    mov dh, 10                 ; Fila 10
    mov dl, 28                 ; Columna 28
    call set_cursor            ; Coloca el cursor en esa posición
    mov si, msg_bye            ; SI apunta al mensaje final
    call print_string          ; Imprime el mensaje final
.hang:                         ; Bucle de parada
    cli                        ; Deshabilita interrupciones
    hlt                        ; Detiene CPU
    jmp .hang                  ; Se queda bloqueado para siempre

; --------------------------------------------------
; Dibujo principal
; Orientaciones:
; 0 = normal
; 1 = izquierda  
; 2 = derecha 
; 3 = arriba    
; 4 = abajo  
; --------------------------------------------------

draw_scene:                    
    call clear_screen          ; Limpia la pantalla antes de redibujar
    call draw_header           ; Dibuja la línea de ayuda superior

    mov al, [orientation]      ; Carga la orientación actual en AL
    cmp al, 0                  ; ¿orientación normal?
    je draw_normal             ; Si sí, dibuja normal
    cmp al, 1                  ; ¿izquierda?
    je draw_left               ; Si sí, dibuja izquierda
    cmp al, 2                  ; ¿derecha?
    je draw_right              ; Si sí, dibuja derecha
    cmp al, 3                  ; ¿arriba?
    je draw_up                 ; Si sí, dibuja arriba
    cmp al, 4                  ; ¿abajo?
    je draw_down               ; Si sí, dibuja abajo

    ret                        ; Si por alguna razón no coincide, retorna

draw_normal:                   ; Dibujo normal: dos nombres horizontales
    mov dh, [rand_row]         ; DH = fila aleatoria base
    mov dl, [rand_col]         ; DL = columna aleatoria base
    call set_cursor            ; Coloca cursor
    mov si, name1              ; SI apunta al primer nombre
    call print_string          ; Imprime el primer nombre

    mov dh, [rand_row]         ; DH = misma fila base
    inc dh                     ; Baja una fila para el segundo nombre
    mov dl, [rand_col]         ; Misma columna
    call set_cursor            ; Coloca cursor
    mov si, name2              ; SI apunta al segundo nombre
    call print_string          ; Imprime el segundo nombre
    ret                        

draw_left:                     
    ; nombre1 vertical hacia arriba
    mov al, [rand_row]         ; AL = fila base
    add al, name1_len_minus1   ; Se suma longitud-1 para comenzar abajo y subir al imprimir
    mov [temp_row], al         ; Guarda fila temporal de trabajo
    mov al, [rand_col]         ; AL = columna base
    mov [temp_col], al         ; Guarda columna temporal
    mov si, name1              ; SI apunta al primer nombre
    call print_vertical_up     ; Imprime el primer nombre vertical hacia arriba

    ; nombre2 vertical hacia arriba, separado 2 columnas
    mov al, [rand_row]         ; AL = fila base
    add al, name2_len_minus1   ; Se ajusta para que suba desde abajo
    mov [temp_row], al         ; Guarda fila temporal
    mov al, [rand_col]         ; AL = columna base
    add al, 2                  ; Separa el segundo nombre dos columnas a la derecha
    mov [temp_col], al         ; Guarda columna temporal
    mov si, name2              ; SI apunta al segundo nombre
    call print_vertical_up     ; Imprime el segundo nombre vertical hacia arriba
    ret                       

draw_right:                    
    ; nombre1 vertical hacia abajo
    mov al, [rand_row]         ; AL = fila base
    mov [temp_row], al         ; Guarda fila temporal
    mov al, [rand_col]         ; AL = columna base
    mov [temp_col], al         ; Guarda columna temporal
    mov si, name1              ; SI apunta al primer nombre
    call print_vertical_down   ; Imprime vertical hacia abajo

    ; nombre2 vertical hacia abajo, separado 2 columnas
    mov al, [rand_row]         ; AL = fila base
    mov [temp_row], al         ; Guarda fila temporal
    mov al, [rand_col]         ; AL = columna base
    add al, 2                  ; Separa dos columnas
    mov [temp_col], al         ; Guarda columna temporal
    mov si, name2              ; SI apunta al segundo nombre
    call print_vertical_down   ; Imprime vertical hacia abajo
    ret                        

draw_up:                       
    mov dh, [rand_row]         ; DH = fila base
    mov dl, [rand_col]         ; DL = columna base
    call set_cursor            ; Coloca cursor
    mov si, name1              ; SI apunta al primer nombre
    call print_string_reverse  ; Imprime el primer nombre al revés

    mov dh, [rand_row]         ; DH = fila base
    inc dh                     ; Baja una fila para el segundo nombre
    mov dl, [rand_col]         ; Misma columna
    call set_cursor            ; Coloca cursor
    mov si, name2              ; SI apunta al segundo nombre
    call print_string_reverse  ; Imprime el segundo nombre al revés
    ret                       

draw_down:                     
    mov dh, [rand_row]         ; DH = fila base
    mov dl, [rand_col]         ; DL = columna base
    call set_cursor            ; Coloca cursor
    mov si, name2              ; Empieza con el segundo nombre
    call print_string_reverse  ; Lo imprime al revés

    mov dh, [rand_row]         ; DH = fila base
    inc dh                     ; Baja una fila
    mov dl, [rand_col]         ; Misma columna
    call set_cursor            ; Coloca cursor
    mov si, name1              ; Ahora usa el primer nombre
    call print_string_reverse  ; Lo imprime al revés
    ret                        

; --------------------------------------------------
; Pantallas
; --------------------------------------------------

draw_welcome:                  
    mov dh, 4                  ; Fila 4
    mov dl, 24                 ; Columna 24
    call set_cursor            ; Posiciona cursor
    mov si, title_msg          ; SI apunta al título
    call print_string          ; Imprime título

    mov dh, 8                  ; Fila 8
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, welcome_msg1       ; Primer mensaje de bienvenida
    call print_string          ; Imprime mensaje

    mov dh, 10                 ; Fila 10
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, welcome_msg2       ; Segundo mensaje de bienvenida
    call print_string          ; Imprime mensaje

    mov dh, 13                 ; Fila 13
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, controls_msg1      ; Mensaje de control 1
    call print_string          ; Imprime mensaje

    mov dh, 14                 ; Fila 14
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, controls_msg2      ; Mensaje de control 2
    call print_string          ; Imprime mensaje

    mov dh, 15                 ; Fila 15
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, controls_msg3      ; Mensaje de control 3
    call print_string          ; Imprime mensaje

    mov dh, 16                 ; Fila 16
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, controls_msg4      ; Mensaje de control 4
    call print_string          ; Imprime mensaje

    mov dh, 17                 ; Fila 17
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, controls_msg5      ; Mensaje de control 5
    call print_string          ; Imprime mensaje

    mov dh, 20                 ; Fila 20
    mov dl, 16                 ; Columna 16
    call set_cursor            ; Posiciona cursor
    mov si, confirm_msg        ; Mensaje de confirmación
    call print_string          ; Imprime mensaje
    ret                        ; Termina

draw_header:                   ; Dibuja la barra o texto superior del juego
    mov dh, 0                  ; Fila 0
    mov dl, 0                  ; Columna 0
    call set_cursor            ; Posiciona cursor
    mov si, hud_msg            ; SI apunta al mensaje HUD
    call print_string          ; Imprime mensaje
    ret                        ; Termina

; --------------------------------------------------
; BIOS utilidades
; --------------------------------------------------

clear_screen:                  ; Rutina para limpiar pantalla
    mov ax, 0x0003             ; AX=0003h: modo texto 80x25 estándar
    int 10h                    ; BIOS video: cambia modo y limpia pantalla
    ret                        ; Retorna

set_cursor:                    ; Rutina para posicionar cursor
    ; DH=row, DL=col
    mov ah, 0x02               ; Función BIOS video para posicionar cursor
    mov bh, 0x00               ; Página 0
    int 10h                    ; Llama a BIOS
    ret                        ; Retorna

print_char:                    ; Imprime un solo carácter
    ; AL = char
    mov ah, 0x0E               ; Función teletipo de BIOS video
    mov bh, 0x00               ; Página 0
    mov bl, 0x07               ; Atributo/color gris claro sobre negro
    int 10h                    ; Llama a BIOS para mostrar el carácter
    ret                        ; Retorna

print_string:                  ; Imprime string terminado en cero
.next:                         ; Bucle principal de impresión
    lodsb                      ; Carga el byte apuntado por SI en AL y avanza SI
    or  al, al                 ; Verifica si AL es cero
    jz  .done                  ; Si es cero, fin del string
    call print_char            ; Imprime el carácter actual
    jmp .next                  ; Continúa con el siguiente
.done:
    ret                        ; Termina

print_string_reverse:          ; Imprime un string al revés
    push si                    ; Guarda SI original
    call find_end              ; Busca el final del string
    dec si                     ; Retrocede para apuntar al último carácter válido
.rev_loop:                     ; Bucle de impresión en reversa
    cmp si, [string_start_ptr] ; Compara SI con el inicio del string guardado
    jb  .done                  ; Si ya pasó el inicio, termina
    mov al, [si]               ; Carga el carácter actual
    call print_char            ; Lo imprime
    dec si                     ; Retrocede al carácter anterior
    jmp .rev_loop              ; Repite
.done:
    pop si                     ; Restaura SI original
    ret                        ; Termina

find_end:                      ; Encuentra el final del string apuntado por SI
    mov [string_start_ptr], si ; Guarda la dirección inicial del string
.find_loop:                    ; Bucle de búsqueda
    lodsb                      ; Lee carácter y avanza
    or  al, al                 ; ¿Es cero?
    jnz .find_loop             ; Si no, sigue buscando
    dec si                     ; Ajusta SI para apuntar al cero final
    ret                        ; Retorna

print_vertical_down:           ; Imprime un string de arriba hacia abajo
.next:
    lodsb                      ; Carga siguiente carácter del string
    or  al, al                 ; Verifica fin de string
    jz  .done                  ; Si es cero, termina

    mov dh, [temp_row]         ; DH = fila actual temporal
    mov dl, [temp_col]         ; DL = columna actual temporal
    call set_cursor            ; Posiciona cursor
    call print_char            ; Imprime carácter

    inc byte [temp_row]        ; Baja una fila para el siguiente carácter
    jmp .next                  ; Repite
.done:
    ret                        ; Termina

print_vertical_up:             ; Imprime un string de abajo hacia arriba
.next:
    lodsb                      ; Carga siguiente carácter
    or  al, al                 ; Verifica fin de string
    jz  .done                  ; Si es cero, termina

    mov dh, [temp_row]         ; DH = fila actual temporal
    mov dl, [temp_col]         ; DL = columna actual temporal
    call set_cursor            ; Posiciona cursor
    call print_char            ; Imprime carácter

    dec byte [temp_row]        ; Sube una fila para el siguiente carácter
    jmp .next                  ; Repite
.done:
    ret                        ; Termina

; --------------------------------------------------
; Random BIOS usando ticks
; genera fila y columna válidas
; --------------------------------------------------

randomize_position:            ; Genera posición aleatoria básica usando ticks del BIOS
.row_try:
    mov ah, 00h                ; Función 00h de INT 1Ah = obtener contador de ticks
    int 1Ah                    ; BIOS time-of-day, devuelve ticks desde medianoche
    mov al, dl                 ; Usa la parte baja de DX como fuente pseudoaleatoria
    and al, 0x0F               ; Limita AL a 0..15
    add al, 5                  ; Lo mueve a 5..20 para filas válidas
    mov [rand_row], al         ; Guarda fila aleatoria

.col_try:
    mov ah, 00h                ; Vuelve a pedir ticks
    int 1Ah                    ; BIOS time-of-day
    mov al, dl                 ; Usa de nuevo DL
    and al, 0x1F               ; Limita a 0..31
    add al, 20                 ; Lo mueve a 20..51 aprox para columnas válidas
    mov [rand_col], al         ; Guarda columna aleatoria
    ret                        ; Termina

; --------------------------------------------------
; Datos
; --------------------------------------------------

orientation db 0               ; Variable que guarda la orientación actual
rand_row    db 8               ; Fila aleatoria actual, inicializada en 8
rand_col    db 30              ; Columna aleatoria actual, inicializada en 30
temp_row    db 0               ; Fila temporal usada por impresión vertical
temp_col    db 0               ; Columna temporal usada por impresión vertical
string_start_ptr dw 0          ; Puntero al inicio de string, usado por impresión en reversa

title_msg      db 'MY NAME - LEGACY X86',0                 ; Título principal
welcome_msg1   db 'This version runs in BIOS legacy mode.',0 ; Texto de bienvenida 1
welcome_msg2   db 'Press ENTER to start or ESC to exit.',0   ; Texto de bienvenida 2
controls_msg1  db 'LEFT  = rotate left',0                    ; Control flecha izquierda
controls_msg2  db 'RIGHT = rotate right',0                   ; Control flecha derecha
controls_msg3  db 'UP    = upper transform',0                ; Control flecha arriba
controls_msg4  db 'DOWN  = lower transform',0                ; Control flecha abajo
controls_msg5  db 'R = restart   ESC = exit',0               ; Controles extra
confirm_msg    db 'Waiting for confirmation...',0            ; Mensaje de espera
hud_msg        db 'Arrows=transform  R=restart  ESC=exit',0  ; Barra de ayuda del juego
msg_bye        db 'Program finished.',0                      ; Mensaje al salir

name1 db 'CHRISTIAN',0       ; Primer nombre a mostrar
name2 db 'JORGE',0           ; Segundo nombre a mostrar

name1_len_minus1 equ 7       ; Longitud del primer nombre - 1, usado para impresión vertical hacia arriba
name2_len_minus1 equ 6       ; Longitud del segundo nombre - 1, usado para impresión vertical hacia arriba

times 20480-($-$$) db 0      ; Rellena con ceros hasta que esta segunda etapa mida 20480 bytes