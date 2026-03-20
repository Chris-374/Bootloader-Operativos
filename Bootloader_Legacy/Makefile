ASM := nasm                     # Define la variable ASM con el ensamblador que se va a usar: nasm
RM  := rm -f                    # Define la variable RM con el comando para borrar archivos 

.PHONY: all clean rebuild       # Indica que all, clean y rebuild no son archivos reales, sino nombres de acciones/targets

all: image.bin                  # Target principal: cuando se corre "make", el objetivo por defecto es generar image.bin

first_stage.bin: first_stage.asm    # Regla: first_stage.bin depende del archivo fuente first_stage.asm
# Ensambla first_stage.asm en formato binario plano y lo guarda como first_stage.bin
	$(ASM) -f bin first_stage.asm -o first_stage.bin   

second_stage.bin: second_stage.asm  # Regla: second_stage.bin depende del archivo fuente second_stage.asm
# Ensambla second_stage.asm en formato binario plano y lo guarda como second_stage.bin
	$(ASM) -f bin second_stage.asm -o second_stage.bin 

image.bin: first_stage.bin second_stage.bin   # Regla: image.bin depende de que ya existan first_stage.bin y second_stage.bin
# Une ambos binarios en orden: primero la etapa 1 y luego la etapa 2, creando image.bin
	cat first_stage.bin second_stage.bin > image.bin   

clean:                          # Target de limpieza
# Borra los archivos generados por el proceso de compilación
	$(RM) first_stage.bin second_stage.bin image.bin   

rebuild: clean all              # Target de recompilación total: primero limpia y luego vuelve a construir todo