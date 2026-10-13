# Awale

Jeu d’Awalé multijoueur en réseau développé en **C**. Le projet repose sur une architecture client–serveur utilisant des sockets TCP : le serveur gère les connexions, les joueurs, les défis, les parties et les règles du jeu ; les clients communiquent avec lui depuis un terminal.

## Sommaire

- [Présentation](#présentation)
- [Fonctionnalités](#fonctionnalités)
- [Prérequis](#prérequis)
- [Compilation](#compilation)
- [Lancement du serveur](#lancement-du-serveur)
- [Lancement du client](#lancement-du-client)
- [Manuel d’utilisation du client](#manuel-dutilisation-du-client)
- [Déroulement d’une partie](#déroulement-dune-partie)
- [Architecture du projet](#architecture-du-projet)
- [Limites actuelles](#limites-actuelles)
- [Dépannage](#dépannage)

## Présentation

Awale est une adaptation en ligne du jeu traditionnel africain de stratégie Awalé. Plusieurs clients peuvent se connecter au même serveur afin de discuter, se défier, jouer une partie, observer une partie en cours et consulter des replays.

Le projet utilise :

- le langage **C** ;
- les sockets **TCP/IPv4** ;
- `select()` pour gérer plusieurs connexions simultanément ;
- `make` pour automatiser la compilation ;
- une interface en ligne de commande.

## Fonctionnalités

### Fonctionnalités du serveur

Le serveur permet de :

- accepter plusieurs connexions TCP ;
- enregistrer les joueurs avec un pseudo ;
- afficher la liste des joueurs présents dans le lobby ;
- gérer les défis entre joueurs ;
- créer et gérer des parties d’Awalé ;
- contrôler les tours de jeu ;
- valider les coups joués ;
- diffuser l’état du plateau aux joueurs ;
- envoyer des messages d’erreur ou de succès ;
- gérer le chat public du lobby ;
- enregistrer une biographie pour chaque joueur ;
- consulter la biographie d’un autre joueur ;
- afficher la liste des parties en cours ;
- permettre à un joueur d’observer une partie ;
- gérer une liste d’amis ;
- rendre une partie privée ;
- sauvegarder les parties terminées ;
- afficher et consulter les replays.

### Fonctionnalités du client

Le client permet de :

- se connecter à un serveur avec son adresse IP et son port ;
- choisir un pseudo ;
- consulter les joueurs disponibles ;
- défier un autre joueur ;
- accepter ou refuser un défi ;
- jouer un coup en indiquant le numéro d’un trou ;
- recevoir automatiquement les mises à jour du plateau ;
- discuter dans le chat public ;
- définir et consulter une biographie ;
- consulter les parties en cours ;
- observer une partie ;
- ajouter un joueur à ses amis ;
- rendre sa partie privée ;
- sauvegarder une partie terminée ;
- consulter les replays disponibles ;
- quitter proprement le serveur.

## Prérequis

Le projet est prévu pour un environnement Linux ou compatible POSIX.

Les outils suivants sont nécessaires :

- GCC ;
- GNU Make ;
- les bibliothèques système POSIX pour les sockets ;
- un terminal.

Sur Debian, Ubuntu ou une distribution similaire, vous pouvez installer les outils nécessaires avec :

```bash
sudo apt update
sudo apt install build-essential
```

## Compilation

Placez-vous dans le répertoire du projet :

```bash
cd Awale
```

Compilez le serveur et le client avec :

```bash
make
```

Le `Makefile` produit deux exécutables :

- `server` : programme serveur ;
- `client` : programme client.

La compilation équivalente sans `make` est :

```bash
gcc -Wall -g -o server server.c session.c network.c awale.c
gcc -Wall -g -o client client.c network.c
```

Pour supprimer les exécutables compilés :

```bash
make clean
```

## Lancement du serveur

### Port par défaut

Le serveur écoute par défaut sur le port `8080` :

```bash
./server
```

Un message similaire à celui-ci doit apparaître :

```text
Server listening on port 8080...
```

### Choisir un autre port

Vous pouvez fournir un port en argument :

```bash
./server 9000
```

Dans ce cas, le serveur écoute sur le port `9000`.

### Rôle du serveur

Le serveur doit être démarré avant les clients. Il reste actif et attend :

- de nouvelles connexions ;
- des demandes de connexion avec un pseudo ;
- des défis ;
- des coups ;
- des messages de chat ;
- des demandes d’observation ou de replay.

Pour arrêter le serveur, utilisez généralement `Ctrl+C` dans le terminal où il est lancé.

## Lancement du client

Le client doit recevoir deux arguments :

```bash
./client <adresse_ip_du_serveur> <port>
```

### Connexion locale

Si le serveur est lancé sur le même ordinateur :

```bash
./client 127.0.0.1 8080
```

### Connexion à un serveur distant

Si le serveur se trouve sur une autre machine, remplacez `127.0.0.1` par son adresse IPv4 :

```bash
./client 192.168.1.20 8080
```

Il est possible d’ouvrir plusieurs terminaux et de lancer plusieurs clients afin de tester une partie multijoueur. Chaque client doit utiliser un pseudo différent.

## Manuel d’utilisation du client

Après la connexion, le client affiche la liste des commandes disponibles et attend une commande après le symbole `>`.

### 1. Se connecter avec un pseudo

```text
/login <pseudo>
```

Exemple :

```text
/login Alice
```

Le pseudo doit être disponible. Après une connexion réussie, le joueur rejoint le lobby.

### 2. Afficher les joueurs du lobby

```text
/list
```

Cette commande affiche les joueurs connectés et disponibles pour une partie.

### 3. Défier un joueur

```text
/challenge <pseudo>
```

Exemple :

```text
/challenge Bob
```

Le joueur ciblé reçoit une notification de défi.

### 4. Accepter ou refuser un défi

Pour accepter un défi :

```text
/accept <pseudo>
```

Exemple :

```text
/accept Alice
```

Pour refuser un défi :

```text
/reject <pseudo>
```

Exemple :

```text
/reject Alice
```

Le nom indiqué doit correspondre au joueur qui a envoyé le défi.

### 5. Jouer un coup

```text
/play <numéro_du_trou>
```

Exemple :

```text
/play 3
```

Le numéro du trou dépend de la représentation du plateau affichée par le serveur. Un joueur ne peut jouer que pendant son tour. Lorsque ce n’est pas son tour, le client bloque temporairement la saisie des commandes de jeu.

### 6. Utiliser le chat public

```text
/chat <message>
```

Exemple :

```text
/chat Bonjour à tous !
```

Le message est envoyé aux autres joueurs connectés au lobby.

### 7. Définir sa biographie

```text
/bio <texte>
```

Exemple :

```text
/bio Joueur passionné d'Awalé
```

La biographie est associée au joueur pendant la session.

### 8. Consulter la biographie d’un joueur

```text
/whois <pseudo>
```

Exemple :

```text
/whois Bob
```

### 9. Afficher les parties en cours

```text
/games
```

Cette commande affiche les parties actuellement ouvertes et les joueurs qui y participent.

### 10. Observer une partie

```text
/spectate <pseudo>
```

Exemple :

```text
/spectate Alice
```

La commande demande à rejoindre en tant que spectateur la partie dans laquelle se trouve le joueur indiqué. Le nombre de spectateurs par partie est limité.

### 11. Ajouter un ami

```text
/friend <pseudo>
```

Exemple :

```text
/friend Bob
```

Le joueur est ajouté à votre liste d’amis si la limite n’est pas atteinte.

### 12. Rendre une partie privée

```text
/private
```

Cette commande rend la partie actuelle privée. Selon la logique du serveur, l’accès à une partie privée est réservé aux amis autorisés.

### 13. Sauvegarder une partie

```text
/save
```

Cette commande permet de demander la sauvegarde de la partie terminée ou en cours selon l’état géré par le serveur. Les replays sont enregistrés côté serveur dans des fichiers locaux.

### 14. Afficher les replays

```text
/replays
```

Le serveur renvoie la liste des replays disponibles.

### 15. Consulter un replay

```text
/replay <id>
```

Exemple :

```text
/replay 1
```

L’identifiant correspond à celui affiché par la commande `/replays`.

### 16. Quitter le client

```text
/quit
```

Exemple :

```text
/quit
```

Le client ferme la connexion TCP et quitte le programme.

## Déroulement d’une partie

Un scénario classique est le suivant :

### Terminal 1 : serveur

```bash
./server 8080
```

### Terminal 2 : premier joueur

```bash
./client 127.0.0.1 8080
```

Puis :

```text
/login Alice
/list
/challenge Bob
```

### Terminal 3 : deuxième joueur

```bash
./client 127.0.0.1 8080
```

Puis :

```text
/login Bob
/accept Alice
```

Lorsque la partie commence, les joueurs reçoivent :

- une notification de début de partie ;
- l’affichage du plateau ;
- une indication lorsque c’est leur tour ;
- une indication lorsque la saisie est bloquée pendant le tour adverse.

Le joueur qui peut jouer saisit ensuite, par exemple :

```text
/play 3
```

Le serveur vérifie le coup, met à jour le plateau et transmet le nouvel état aux joueurs concernés.

## Architecture du projet

```text
.
├── Makefile       # Règles de compilation
├── awale.c        # Règles et logique du jeu d’Awalé
├── awale.h        # Structures et fonctions du plateau
├── network.c      # Envoi et réception fiables des paquets
├── network.h      # Types de commandes et structure Packet
├── session.c      # Joueurs, parties, défis, amis et replays
├── session.h      # Structures Player et GameSession
├── server.c       # Programme serveur et boucle réseau
├── client.c       # Programme client et interface terminal
└── readme.md      # Documentation du projet
```

### `server.c`

Le serveur :

1. crée une socket TCP ;
2. s’attache à une adresse et à un port ;
3. écoute les nouvelles connexions ;
4. utilise `select()` pour gérer plusieurs sockets ;
5. reçoit les paquets des clients ;
6. transmet les demandes aux fonctions de `session.c` ;
7. renvoie les réponses et les mises à jour.

### `client.c`

Le client :

1. se connecte à l’adresse IP et au port du serveur ;
2. lit les commandes saisies dans le terminal ;
3. les convertit en paquets réseau ;
4. affiche les réponses du serveur ;
5. bloque ou autorise la saisie pendant les tours de jeu.

### `network.c` et `network.h`

Ces fichiers définissent le protocole interne utilisé entre le client et le serveur. Les communications utilisent une structure `Packet` composée de :

- un type de commande ;
- la taille utile des données ;
- une charge utile de 512 caractères maximum.

Les fonctions `send_packet()` et `receive_packet()` effectuent des envois et des réceptions complètes afin de limiter les problèmes liés aux paquets TCP partiels.

### `awale.c` et `awale.h`

Ces fichiers contiennent la logique du jeu :

- création du plateau ;
- déplacement et distribution des graines ;
- capture des graines ;
- validation des coups ;
- calcul du score ;
- détection de la fin de partie ;
- affichage du plateau.

### `session.c` et `session.h`

Ces fichiers gèrent :

- les joueurs connectés ;
- les pseudos ;
- les états des joueurs ;
- les défis ;
- les parties ;
- les spectateurs ;
- les amis ;
- les biographies ;
- les historiques de coups ;
- les sauvegardes et replays.

## Limites actuelles

Les limites principales sont définies dans `session.h` :

| Élément | Limite |
|---|---:|
| Clients connectés | 30 |
| Parties simultanées | 15 |
| Spectateurs par partie | 10 |
| Amis par joueur | 10 |
| Coups enregistrés par partie | 100 |
| Taille maximale d’un pseudo | 31 caractères utiles |
| Taille maximale d’une bio | 255 caractères utiles |
| Taille maximale d’un paquet utile | 512 caractères |

Autres remarques :

- le protocole transmet directement une structure C ; il est donc surtout adapté à des environnements compatibles ;
- les données des joueurs et des parties sont principalement conservées en mémoire ;
- les parties et les listes en mémoire ne sont pas restaurées automatiquement après un redémarrage du serveur ;
- les replays sont enregistrés dans le répertoire de travail du serveur ;
- les tests automatisés ne sont pas fournis dans le `Makefile` actuel ;
- il est recommandé d’utiliser plusieurs clients pour tester les scénarios multijoueurs.

## Dépannage

### `make: command not found`

Installez GNU Make et GCC :

```bash
sudo apt update
sudo apt install build-essential
```

### `Bind failed`

Le port est peut-être déjà utilisé. Choisissez un autre port :

```bash
./server 9000
```

Puis connectez le client sur le même port :

```bash
./client 127.0.0.1 9000
```

### `Connection failed`

Vérifiez :

- que le serveur est lancé ;
- que l’adresse IP est correcte ;
- que le port du client correspond à celui du serveur ;
- que le pare-feu autorise les connexions TCP sur ce port.

### Le pseudo est refusé

Choisissez un pseudo différent. Un pseudo déjà utilisé par un joueur connecté ne peut pas être réutilisé.

### Un joueur ne reçoit pas de défi

Vérifiez que :

- le nom est correctement écrit ;
- le joueur est connecté ;
- le joueur se trouve dans le lobby ;
- il n’est pas déjà engagé dans une autre partie.

### Le serveur se déconnecte

Le client affiche alors un message de déconnexion. Relancez le serveur puis reconnectez les clients.

## Licence

Aucune licence open source n’est actuellement déclarée dans le dépôt. Si le projet doit être distribué ou réutilisé, il est recommandé d’ajouter un fichier `LICENSE`.
