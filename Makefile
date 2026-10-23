CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

console: src/main.c src/personnage.c src/sauvegarde.c
	$(CC) $(CFLAGS) $^ -o donjon

sdl: src/main_sdl.c src/personnage.c src/sauvegarde.c
	$(CC) $(CFLAGS) $^ -o donjon_sdl $(shell sdl2-config --cflags --libs) -lSDL2_ttf

# Supprime les executables et la sauvegarde
clean:
	rm -f donjon donjon_sdl donjon.bin
