# Awale

Projet Awale en architecture client-serveur, écrit en C avec des sockets TCP.

---

## 1. Compilation du projet

Le projet utilise un `Makefile` pour automatiser la génération des exécutables.
Ouvrez un terminal, placez-vous dans le répertoire racine du projet et exécutez :

```bash
make
```

Cette commande génère deux exécutables :

- `server` : le programme gérant la logique de jeu et les connexions.
- `client` : l'interface permettant aux joueurs de se connecter et de jouer.

> **Note :** pour nettoyer l'espace de travail et supprimer les exécutables compilés, utilisez `make clean`.

---

## 2. Lancement de l'application    

### 2.1 Démarrer le serveur

Le serveur écoute par défaut sur le port TCP **8080**.

```bash
./server
```

Pour utiliser un autre port (par exemple si le 8080 est déjà occupé) :

```bash
./server 9000
```

Pour arrêter le serveur, utilisez `Ctrl + C`.

### 2.2 Démarrer un client

Ouvrez un nouveau terminal pour chaque joueur. Le client nécessite l'adresse IP du serveur et le port :

```bash
./client <adresse_ip> <port>
```

- Pour jouer en local (sur le même ordinateur) :

```bash
  ./client 127.0.0.1 8080
```

- Pour se connecter à un serveur distant :

```bash
  ./client 192.168.1.50 8080
```

---

## 3. Manuel du joueur (commandes client)

Toutes les interactions avec le jeu se font via des commandes textuelles précédées d'un slash (`/`).

### Étape 1 : Connexion et profil

| Commande | Description |
|---|---|
| `/login <pseudo>` | S'enregistrer sur le serveur (ex : `/login Alice`). **Obligatoire** dès le lancement. |
| `/bio <texte>` | Ajouter une description à votre profil (ex : `/bio Champion régional d'Awalé`). |
| `/whois <pseudo>` | Afficher la biographie d'un joueur spécifique. |
| `/list` | Afficher la liste de tous les joueurs actuellement dans le lobby. |
| `/chat <message>` | Envoyer un message visible par tous les joueurs du lobby. |

### Étape 2 : Lancer une partie

| Commande | Description |
|---|---|
| `/challenge <pseudo>` | Défier un joueur du lobby (ex : `/challenge Bob`). |
| `/accept <pseudo>` | Accepter un défi reçu. La partie démarre immédiatement. |
| `/reject <pseudo>` | Refuser un défi reçu. |
| `/play <num_trou>` | Jouer un coup en choisissant un trou (ex : `/play 3`). Ne fonctionne que si c'est votre tour. |

### Étape 3 : Fonctionnalités sociales et spectateur

| Commande | Description |
|---|---|
| `/friend <pseudo>` | Ajouter un joueur à votre liste d'amis. |
| `/games` | Afficher toutes les parties actuellement en cours sur le serveur. |
| `/spectate <pseudo>` | Rejoindre une partie en tant que spectateur pour observer les coups en direct. |
| `/private` | À utiliser pendant une partie : verrouille votre partie. Seuls les joueurs de votre liste d'amis pourront utiliser `/spectate` pour vous observer. |

### Étape 4 : Sauvegardes et replays

| Commande | Description |
|---|---|
| `/save` | À la fin d'une partie (ou si l'adversaire se déconnecte), enregistre l'historique complet des coups joués. |
| `/replays` | Afficher la liste des identifiants des parties sauvegardées sur le serveur. |
| `/replay <id>` | Visionner le déroulement complet d'une ancienne partie (ex : `/replay 1`). |

### Étape 5 : Quitter

| Commande | Description |
|---|---|
| `/quit` | Se déconnecter proprement du serveur et fermer le client. |

---

## 4. Fonctionnement interne et limites

- **Multiplexage :** le serveur utilise la fonction `select()` pour gérer les requêtes de tous les clients de manière asynchrone sur un seul thread.
- **Protocole :** les communications reposent sur une structure `Packet` de taille fixe (512 octets de charge utile).
- **Limites par défaut :**
  - 30 clients maximum connectés simultanément.
  - 15 parties en simultané.
  - 10 amis maximum par joueur.