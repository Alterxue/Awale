#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include "session.h"
#include "network.h" // Includes the Packet structure and send_packet function

// Actual global data is stored here
Player all_players[MAX_CLIENTS];
GameSession all_games[MAX_GAMES];

static int next_game_id = 1;

// ==========================================
// Network Helper Utility
// ==========================================
/**
 * @brief Helper function to easily send a string message wrapped in a Packet.
 */
void send_simple_packet(int fd, CommandType type, const char *msg) {
    Packet p;
    p.type = type;
    if (msg != NULL) {
        strncpy(p.payload, msg, sizeof(p.payload) - 1);
        p.payload[sizeof(p.payload) - 1] = '\0';
        p.data_length = strlen(p.payload);
    } else {
        p.payload[0] = '\0';
        p.data_length = 0;
    }
    send_packet(fd, &p);
}

// Helper lookup functions
Player* find_player_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (all_players[i].fd == fd) return &all_players[i];
    }
    return NULL;
}

Player* find_player_by_name(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (all_players[i].fd != 0 && strcmp(all_players[i].pseudo, name) == 0) {
            return &all_players[i];
        }
    }
    return NULL;
}
// Using ERROR locally just to mark it as invalid
GameSession* find_empty_game_slot() {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (all_games[i].session_id == 0) return &all_games[i];
    }
    return NULL;
}

GameSession* find_game_by_id(int game_id) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (all_games[i].session_id == game_id) return &all_games[i];
    }
    return NULL;
}


// Player lifecycle and management

void init_player(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (all_players[i].fd == 0) {
            all_players[i].fd = fd;
            memset(all_players[i].pseudo, 0, sizeof(all_players[i].pseudo));
            strcpy(all_players[i].bio, "Pas de bio.");
            all_players[i].state = STATE_CONNECTED;
            all_players[i].current_game_id = -1;
            return;
        }
    }
    // Server is full, notify the client and close the connection
    send_simple_packet(fd, CMD_ERROR, "Erreur: Le serveur est complet.");
    close(fd); 
}

void remove_player(int fd) {
    Player *p = find_player_by_fd(fd);
    if (p == NULL) return;

    // Robustness check: if player disconnects during a game, opponent wins
    if (p->state == STATE_PLAYING && p->current_game_id != -1) {
        GameSession *game = find_game_by_id(p->current_game_id);
        if (game) {
            int opponent_fd = (game->player1_fd == fd) ? game->player2_fd : game->player1_fd;
            Player *opponent = find_player_by_fd(opponent_fd);
            
            if (opponent) {
                // Notify opponent of victory due to disconnection
                send_simple_packet(opponent->fd, CMD_SUCCESS, "Votre adversaire s'est déconnecté. Vous gagnez !");
                opponent->state = STATE_LOBBY_IDLE; 
                opponent->current_game_id = -1;
            }
            // Clean up the room
            game->session_id = 0; 
        }
    }
    
    // If challenged but disconnected, reset the target (simplified)
    // Completely clear the slot
    p->fd = 0;
    p->current_game_id = -1;
}

void register_player_name(int fd, const char *name) {
    Player *p = find_player_by_fd(fd);
    if (!p) return;

    // Check for duplicate names
    if (find_player_by_name(name) != NULL) {
        send_simple_packet(fd, CMD_ERROR, "Erreur: Ce pseudo est déjà utilisé.");
        return;
    }

    strcpy(p->pseudo, name);
    p->state = STATE_LOBBY_IDLE;
    
    // Send success confirmation
    send_simple_packet(fd, CMD_SUCCESS, "Inscrit avec succès. Bienvenue dans le lobby !");
}

void send_lobby_list(int fd) {
    Player *p = find_player_by_fd(fd);
    if (!p) return;

    char list_buffer[512] = "Joueurs en ligne: ";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (all_players[i].fd != 0 && all_players[i].state == STATE_LOBBY_IDLE) {
            strcat(list_buffer, all_players[i].pseudo);
            strcat(list_buffer, " | ");
        }
    }
    
    send_simple_packet(fd, CMD_PLAYERS_LIST, list_buffer);
}

// Challenge system and matchmaking
void handle_challenge_request(int challenger_fd, const char *target_name) {
    Player *challenger = find_player_by_fd(challenger_fd);
    Player *target = find_player_by_name(target_name);

    if (!challenger || !target) {
        send_simple_packet(challenger_fd, CMD_ERROR, "Erreur: Joueur introuvable.");
        return;
    }

    if (target->fd == challenger_fd) {
        send_simple_packet(challenger_fd, CMD_ERROR, "Erreur: Vous ne pouvez pas vous défier.");
        return;
    }
    
    if (target->state != STATE_LOBBY_IDLE) {
        send_simple_packet(challenger_fd, CMD_ERROR, "Erreur: Ce joueur est occupé.");
        return;
    }

    // Lock both players' states
    challenger->state = STATE_LOBBY_WAITING;
    target->state = STATE_LOBBY_WAITING;

    // Send challenge request to target
    send_simple_packet(target->fd, CMD_CHALLENGE_INCOMING, challenger->pseudo);
}

void handle_challenge_reject(int rejector_fd, const char *challenger_name) {
    Player *rejector = find_player_by_fd(rejector_fd);
    Player *challenger = find_player_by_name(challenger_name);

    if (rejector) rejector->state = STATE_LOBBY_IDLE;
    if (challenger) {
        challenger->state = STATE_LOBBY_IDLE;
        send_simple_packet(challenger->fd, CMD_ERROR, "Votre adversaire a refusé le défi.");
    }
}

void create_game_session(int acceptor_fd, const char *challenger_name) {
    Player *acceptor = find_player_by_fd(acceptor_fd);
    Player *challenger = find_player_by_name(challenger_name);

    if (!acceptor || !challenger) return;

    GameSession *new_game = find_empty_game_slot();
    if (!new_game) {
        // Server is full for games
        send_simple_packet(acceptor->fd, CMD_ERROR, "Erreur: Le serveur de jeu est plein.");
        send_simple_packet(challenger->fd, CMD_ERROR, "Erreur: Le serveur de jeu est plein.");
        acceptor->state = STATE_LOBBY_IDLE;
        challenger->state = STATE_LOBBY_IDLE;
        return;
    }

    // Initialize game session data
    new_game->session_id = next_game_id++;
    new_game->player1_fd = challenger->fd;
    new_game->player2_fd = acceptor->fd;
    new_game->spectator_count = 0;
    
    // Initialize Awalé board logic (Uncomment when awale.c is ready)
    // init_board(&(new_game->board));

    // Update states
    acceptor->state = STATE_PLAYING;
    challenger->state = STATE_PLAYING;
    acceptor->current_game_id = new_game->session_id;
    challenger->current_game_id = new_game->session_id;

    // Notify both players that the game has started
    send_simple_packet(challenger->fd, CMD_GAME_START, "La partie commence ! Vous êtes Joueur 1.");
    send_simple_packet(acceptor->fd, CMD_GAME_START, "La partie commence ! Vous êtes Joueur 2.");

    // Broadcast initial board (Placeholder payload until awale is implemented)
    send_simple_packet(challenger->fd, CMD_BOARD_STATE, "[ PLATEAU AWALE INITIAL ]");
    send_simple_packet(acceptor->fd, CMD_BOARD_STATE, "[ PLATEAU AWALE INITIAL ]");
}

// In-game move processing

void handle_play_move(int fd, int pit_index) {
    
}

// system of bio 
void set_player_bio(int fd, const char *bio) {
    Player *p = find_player_by_fd(fd);
    if (!p) return;

    strncpy(p->bio, bio, sizeof(p->bio) - 1);
    p->bio[sizeof(p->bio) - 1] = '\0';
    
    send_simple_packet(fd, CMD_SUCCESS, "Bio mise à jour avec succès.");
}

void send_player_bio(int requester_fd, const char *target_name) {
    Player *target = find_player_by_name(target_name);
    if (!target) {
        send_simple_packet(requester_fd, CMD_ERROR, "Erreur: Joueur introuvable.");
        return;
    }
    
    char buffer[512];
    // group the name of the player and the Bio 
    snprintf(buffer, sizeof(buffer), "=== Bio de %s ===\n%s\n=================", target->pseudo, target->bio);
    send_simple_packet(requester_fd, CMD_SHOW_BIO, buffer);
}

// system of spectateur

// checking the match which are playing
void send_games_list(int fd) {
    char list_buffer[512] = "=== Parties en cours ===\n";
    int active_games = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        // if the room is using
        if (all_games[i].session_id != 0) {
            Player *p1 = find_player_by_fd(all_games[i].player1_fd);
            Player *p2 = find_player_by_fd(all_games[i].player2_fd);
            
            if (p1 && p2) {
                char line[128];
                // format
                snprintf(line, sizeof(line), " [Salon %d] %s vs %s (%d spectateurs)\n", 
                         all_games[i].session_id, p1->pseudo, p2->pseudo, all_games[i].spectator_count);
                strcat(list_buffer, line);
                active_games++;
            }
        }
    }

    if (active_games == 0) {
        strcat(list_buffer, " Aucune partie en cours.\n");
    }
    strcat(list_buffer, "========================");

    send_simple_packet(fd, CMD_GAMES_LIST, list_buffer);
}

void join_game_as_spectator(int fd, const char *target_name) {
    Player *spectator = find_player_by_fd(fd);
    Player *target = find_player_by_name(target_name);

    if (!spectator || !target) {
        send_simple_packet(fd, CMD_ERROR, "Erreur: Joueur introuvable.");
        return;
    }
    
    if (target->state != STATE_PLAYING || target->current_game_id == -1) {
        send_simple_packet(fd, CMD_ERROR, "Erreur: Ce joueur n'est pas en partie.");
        return;
    }
    
    GameSession *game = find_game_by_id(target->current_game_id);
    if (!game) return;
    
    if (game->spectator_count >= MAX_SPECTATORS) {
        send_simple_packet(fd, CMD_ERROR, "Erreur: Le salon est plein de spectateurs.");
        return;
    }
    
    // add the fd to the game_spectator
    game->spectators[game->spectator_count++] = fd;
    spectator->state = STATE_SPECTATING;
    spectator->current_game_id = game->session_id;
    
    send_simple_packet(fd, CMD_SUCCESS, "Vous observez maintenant la partie.");
    // todo : send the board to the spectator 
}