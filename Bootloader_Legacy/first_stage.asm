[bits 16]                      ; Indica que el código se ensamblará en 16 bits
[org 0x7c00]                   ; Cargado por BIOS en la dirección 0x7C00

SECOND_STAGE_LOADING_ADDRESS equ 0x7E00   ; Dirección de memoria donde se cargará la segunda etapa
SECOND_STAGE_SECTOR_SIZE     equ 40       ; Cantidad de sectores que se leerán desde disco para la segunda etapa

start:                         
    cli                        ; Deshabilita interrupciones para evitar problemas en configuracion de segmentos y stack
    xor ax, ax                 ; AX = 0
    mov ds, ax                 ; DS = 0, segmento de datos en 0
    mov es, ax                 ; ES = 0, segmento extra en 0
    mov ss, ax                 ; SS = 0, segmento de stack en 0
    mov sp, 0x7BFF            ; Coloca el puntero de stack justo debajo de 0x7C00 para no chocar con el bootloader
    sti                        ; Vuelve a habilitar interrupciones

    
    mov [boot_drive], dl       ; Guarda en memoria el número de unidad desde la que arrancó BIOS

    ; lectura de segunda etapa desde disco
    mov ah, 0x02               ; Función 02h de INT 13h: leer sectores del disco
    mov al, SECOND_STAGE_SECTOR_SIZE ; Número de sectores a leer
    mov ch, 0x00               ; Cilindro 0
    mov cl, 0x02               ; Sector 2 
    mov dh, 0x00               ; Cabeza 0
    mov dl, [boot_drive]       ; Recupera el número de unidad guardado antes
    mov bx, SECOND_STAGE_LOADING_ADDRESS ; Dirección de memoria donde BIOS colocará la segunda etapa
    int 0x13                   ; Llama al servicio BIOS de disco para leer los sectores
    jc disk_error              ; Si el carry flag queda activado, hubo error al leer y se salta a disk_error

    jmp 0x0000:SECOND_STAGE_LOADING_ADDRESS ; Salto lejano a la segunda etapa cargada en memoria

disk_error:                    ; Etiqueta a la que se entra si falló la lectura del disco
    mov si, msg_disk_error     ; SI apunta al texto del mensaje de error
.print:                        ; Bucle para imprimir el mensaje carácter por carácter
    lodsb                      ; Carga en AL el byte apuntado por SI y luego incrementa SI
    or  al, al                 ; Prueba si AL es 0
    jz  .hang                  ; Si AL es 0, era el fin del string, entonces cuelga la máquina
    mov ah, 0x0E               ; Función teletipo de INT 10h para imprimir un carácter
    mov bh, 0x00               ; Página de video 0
    mov bl, 0x07               ; Atributo/color gris claro sobre negro
    int 0x10                   ; Llama a BIOS video para mostrar el carácter en pantalla
    jmp .print                 ; Repite para el siguiente carácter

.hang:                         ; Etiqueta de bloqueo infinito
    cli                        ; Deshabilita interrupciones
    hlt                        ; Detiene la CPU hasta la siguiente interrupción
    jmp .hang                  ; Vuelve a saltar aquí para quedarse bloqueado permanentemente

boot_drive db 0                ; Reserva 1 byte para guardar el número de unidad de arranque
msg_disk_error db 'Disk read error', 0 ; String de error terminado en cero

times 510 - ($-$$) db 0        ; Rellena con ceros hasta que el sector mida exactamente 510 bytes
dw 0xAA55                      ; Firma obligatoria del boot sector para que BIOS lo reconozca como booteable