# Le Donjon - RPG Manager en C

Application de gestion et simulation de combats au tour par tour pour un RPG médiéval, développée en C avancé (allocation dynamique, pointeurs, persistance binaire) avec une interface graphique interactive propulsée par **SDL2**.

## Fonctionnalités

- **Logique métier (Core C)** :
  - Structures de personnages (Guerrier, Mage, Voleur) avec statistiques différenciées.
  - Calcul de dégâts avec prise en compte de la défense et limitation aux PV restants.
  - Tableau dynamique de personnages avec réallocation (`realloc`) et suppression des entités éliminées.
  - Persistance binaire (`fread` / `fwrite`) pour sauvegarder et restaurer l'état de la partie.
- **Double interface** :
  - Mode console interactif avec validation stricte des entrées utilisateur.
  - Mode graphique **SDL2** avec boucle événementielle non-bloquante (`SDL_PollEvent`), barres de vie dynamiques, saisie au clavier et boutons cliquables.

## Structure du projet

```text
projet/
├── include/          # En-têtes (.h) : personnage, sauvegarde
├── src/              # Sources (.c) : logique, sauvegarde, menus
├── Makefile          # Règles de compilation console, SDL et clean
├── .gitignore
└── README.md

```

## Dépendances (Arch Linux / Manjaro)

sudo pacman -S base-devel sdl2 sdl2_ttf ttf-dejavu

## Compilation et exécution

Version console :

make console && ./donjon

Version graphique (SDL2) :
    

make sdl && ./donjon_sdl

Nettoyage des binaires :
  

make clean
