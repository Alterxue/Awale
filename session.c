#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <dirent.h>
#include "session.h"
#include "network.h"

// Global data is stored here
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

void send_packet_to_player_and_spectator(GameSession* game, int fd, CommandType type, const char *msg) {
    // To player
    send_simple_packet(fd, type, msg);

    // To spectators
    for (int i = 0; i < game->spectator_count; i++)
    {
        int spectator_fd = game->spectators[i];
        send_simple_packet(spectator_fd, CMD_BOARD_STATE, msg);
    }

    // Update Board History for Replay
    game->board_history[game->move_count] = msg;
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
        if (all_games[i].session_id == game_id) {
            return &all_games[i];
        } 
    }
    return NULL;
}

GameSession* find_game_by_player_id(int player_id) {
    for (int i = 0; i < MAX_GAMES; i++) {
        if (all_games[i].player1_fd == player_id || all_games[i].player2_fd == player_id) {
            return &all_games[i];
        }
    }
    return NULL;
}

int get_player_number(GameSession* game, int player_id) {
    if (game->player1_fd == player_id) return 1;
    return 2;
}

void add_friend(int fd, const char *friend_name) {
    Player *p = find_player_by_fd(fd);
    if (!p) return;
    
    if (p->friend_count >= MAX_FRIENDS) {
        send_simple_packet(fd, CMD_ERROR, "Erreur : Liste d'amis pleine.");
        return;
    }
    
    strcpy(p->friends[p->friend_count++], friend_name);
    send_simple_packet(fd, CMD_SUCCESS, "Ami ajouté avec succès.");
}

void toggle_private_mode(int fd) {
    Player *p = find_player_by_fd(fd);
    if (!p || p->current_game_id == -1) return;
    
    GameSession *game = find_game_by_id(p->current_game_id);
    if (game) {
        game->is_private = 1;
        send_simple_packet(fd, CMD_SUCCESS, "Le salon est maintenant réservé à vos amis.");
    }
}

int is_friend(Player *owner, const char *spectator_name) {
    for (int i = 0; i < owner->friend_count; i++) {
        if (strcmp(owner->friends[i], spectator_name) == 0) return 1;
    }
    return 0;
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

    // If player disconnects during a game, opponent wins
    if (p->state == STATE_PLAYING && p->current_game_id != -1) {
        GameSession *game = find_game_by_id(p->current_game_id);
        if (game) {
            int opponent_fd = (game->player1_fd == fd) ? game->player2_fd : game->player1_fd;
            Player *opponent = find_player_by_fd(opponent_fd);
            
            if (opponent) {
                // Notify opponent of victory due to disconnection
                send_simple_packet(opponent->fd, CMD_GAME_END, "Votre adversaire s'est déconnecté. Vous gagnez !");
                opponent->state = STATE_LOBBY_IDLE; 
                opponent->current_game_id = -1;
            }
            
            game->session_id = 0; 
        }
    }
    
    // Reset player data
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

// Challenge system
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

    // Choose who begins randomly
    int first_player = rand() % 2; 
    Player *player1 = (first_player == 0) ? challenger : acceptor;
    Player *player2 = (first_player == 0) ? acceptor : challenger;

    // Initialize game session data
    new_game->session_id = next_game_id++;
    new_game->player1_fd = player1->fd;
    new_game->player1_name = player1->pseudo;
    new_game->player2_fd = player2->fd;
    new_game->player2_name = player2->pseudo;
    new_game->spectator_count = 0;
    new_game->is_private = 0; // Default to public
    new_game->move_count = 0;
    for (int i = 0; i < MAX_MOVES; i++) {
        new_game->board_history[i] = NULL;
    }
    
    // Initialize board
    Board* board = initBoard();
    new_game->board = board;

    // Update states
    acceptor->state = STATE_PLAYING;
    challenger->state = STATE_PLAYING;
    acceptor->current_game_id = new_game->session_id;
    challenger->current_game_id = new_game->session_id;

    // Notify both players that the game has started
    send_simple_packet(new_game->player1_fd, CMD_GAME_START, NULL);
    send_simple_packet(new_game->player1_fd, INTERN_CMD_ALLOW_INPUT, NULL);

    send_simple_packet(new_game->player2_fd, CMD_GAME_START, NULL);
    send_simple_packet(new_game->player2_fd, INTERN_CMD_BLOCK_INPUT, NULL);
    
    send_packet_to_player_and_spectator(new_game, new_game->player1_fd, CMD_BOARD_STATE, printScoreAndBoard(new_game, 1, new_game->player2_fd));
    send_simple_packet(new_game->player2_fd, CMD_BOARD_STATE, printScoreAndBoard(new_game, 2, new_game->player2_fd));
}

// In-game move processing
void handle_play_move(int player_fd, int pit_index) {
    GameSession *game = find_game_by_player_id(player_fd);
    if (!game) return;

    int playerNb = get_player_number(game, player_fd);
    if (!playerNb) return;

    int moveResult = play(game->board, playerNb, pit_index);

    if (moveResult == -1) {
        send_simple_packet(player_fd, CMD_ERROR, "Coup invalide. Veuillez réessayer.");
        return;
    }

    game->move_count++;

    if (moveResult == 1 || moveResult == 2) {
        
        int winner_fd = (moveResult == 1) ? game->player1_fd : game->player2_fd;
        handle_victory(game, winner_fd);

    } else {

        int other_player_fd = (player_fd == game->player1_fd) ? game->player2_fd : game->player1_fd;

        // Manage Inputs
        send_simple_packet(player_fd, INTERN_CMD_BLOCK_INPUT, NULL);
        send_simple_packet(other_player_fd, INTERN_CMD_ALLOW_INPUT, NULL);

        // Send Visual Data to both players
        send_packet_to_player_and_spectator(game, game->player1_fd, CMD_BOARD_STATE, printScoreAndBoard(game, 1, player_fd));
        send_simple_packet(game->player2_fd, CMD_BOARD_STATE, printScoreAndBoard(game, 2, player_fd));
    }
}

char* printScoreAndBoard(GameSession* game, int fromViewOfPlayerId, int currentPlayerTurnFd) {
	if (fromViewOfPlayerId < 1 || fromViewOfPlayerId > 2) return NULL;
	
	char* display = malloc(sizeof(char)*500);
	int offset = 0;	

	// Score row
    char* nextPlayerTurnName = (currentPlayerTurnFd == game->player1_fd) ?  game->player2_name : game->player1_name;
	offset += sprintf(display + offset, "\n - - - Au Tour de %s - - - \n", nextPlayerTurnName);
	offset += sprintf(display + offset, "\nScore : \n");
	offset += sprintf(display + offset, " -  %s : %d\n", game->player1_name, game->board->scorePlayerOne);
	offset += sprintf(display + offset, " -  %s : %d\n\n", game->player2_name, game->board->scorePlayerTwo);
	
	char * board_view = printBoard(game->board, fromViewOfPlayerId);

	sprintf(display + offset, "%s\n", board_view);
	return display;
}

void handle_victory(GameSession* game, int winning_player_fd) {
    send_simple_packet(winning_player_fd, CMD_GAME_END, "\n- - - YOU WIN - - - \n");

    char* display = malloc(sizeof(char)*20);
    
    if (winning_player_fd == game->player1_fd) {
        sprintf(display, "\n- - - %s WIN - - - \n", game->player1_name);
        send_packet_to_player_and_spectator(game, game->player2_fd, CMD_GAME_END, display);
    } else {
        sprintf(display, "\n- - - %s WIN - - - \n", game->player2_name);
        send_packet_to_player_and_spectator(game, game->player1_fd, CMD_GAME_END, display);
    }
    
    Player *player1 = find_player_by_fd(game->player1_fd);
    Player *player2 = find_player_by_fd(game->player2_fd);

    player1->state = STATE_LOBBY_IDLE;
    player2->state = STATE_LOBBY_IDLE;
    player1->current_game_id = -1;
    player2->current_game_id = -1;
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

// checking the match which are playing
void send_games_list(int fd) {
    char list_buffer[512] = "=== Parties en cours ===\n";
    int active_games = 0;

    for (int i = 0; i < MAX_GAMES; i++) {
        if (all_games[i].session_id != 0) {
            Player *p1 = find_player_by_fd(all_games[i].player1_fd);
            Player *p2 = find_player_by_fd(all_games[i].player2_fd);
            
            if (p1 && p2) {
                char line[128];
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

// system of spectateur
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

    if (game->is_private) {
        Player *p1 = find_player_by_fd(game->player1_fd);
        Player *p2 = find_player_by_fd(game->player2_fd);
        
        if (!is_friend(p1, spectator->pseudo) && !is_friend(p2, spectator->pseudo)) {
            send_simple_packet(fd, CMD_ERROR, "Erreur : Salon privé. Vous n'êtes pas sur la liste d'amis.");
            return;
        }
    }
    
    if (game->spectator_count >= MAX_SPECTATORS) {
        send_simple_packet(fd, CMD_ERROR, "Erreur: Le salon est plein de spectateurs.");
        return;
    }
    
    // add the fd to the game_spectator
    game->spectators[game->spectator_count++] = fd;
    spectator->state = STATE_SPECTATING;
    spectator->current_game_id = game->session_id;

    char msg[256];

    snprintf(msg, sizeof(msg),
        "Vous observez maintenant la partie avec le point de vue du joueur %s.",
        game->player1_name);
    
    send_simple_packet(fd, CMD_SUCCESS, msg);
}

void save_game_to_file(GameSession *game) {
    char filename[64];
    snprintf(filename, sizeof(filename), "replay_game_%d.txt", game->session_id);
    
    FILE *file = fopen(filename, "w");
    if (!file) return;
    
    fprintf(file, "=== %s VS %s ===\n\n", game->player1_name, game->player2_name);
    
    for (int i = 1; i <= game->move_count; i++) {
        fprintf(file, "%s", game->board_history[i]);
    }
    
    fclose(file);
    printf("Visual replay saved to %s\n", filename);
}

void save_current_game(int fd) {
    Player *p = find_player_by_fd(fd);
    if (!p || p->current_game_id == -1) {
        send_simple_packet(fd, CMD_ERROR, "Erreur : Vous n'êtes pas dans une partie à sauvegarder.");
        return;
    }
    
    GameSession *game = find_game_by_id(p->current_game_id);
    if (game) {
        save_game_to_file(game); 
        send_simple_packet(fd, CMD_SUCCESS, "Partie sauvegardée avec succès ! Vous pouvez quitter le salon.");
    }
}

void handle_replay_request(int fd, int game_id) {
    char filename[64];
    snprintf(filename, sizeof(filename), "replay_game_%d.txt", game_id);
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        send_simple_packet(fd, CMD_ERROR, "Erreur : Replay introuvable.");
        return;
    }
    
    send_simple_packet(fd, CMD_SUCCESS, "=== DÉBUT DU REPLAY ===");
    
    char line[128];
    char payload_buffer[512] = "";
    
    while (fgets(line, sizeof(line), file)) {
        if (strlen(payload_buffer) + strlen(line) >= 500) {
            send_simple_packet(fd, CMD_BOARD_STATE, payload_buffer);
            memset(payload_buffer, 0, sizeof(payload_buffer)); 
        }
        strcat(payload_buffer, line); 
    }
    
    if (strlen(payload_buffer) > 0) {
        send_simple_packet(fd, CMD_BOARD_STATE, payload_buffer);
    }
    

    send_simple_packet(fd, CMD_SUCCESS, "=== FIN DU REPLAY ===");
    fclose(file);
}

void list_replays(int fd) {
    DIR *d;
    struct dirent *dir;
    char list_buffer[512] = "=== Replays Disponibles ===\n";
    int count = 0;

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // find only the files which begins with "replay_game"
            if (strncmp(dir->d_name, "replay_game_", 12) == 0) {
                char line[64];
                snprintf(line, sizeof(line), " - %s\n", dir->d_name);
                strcat(list_buffer, line);
                count++;
            }
        }
        closedir(d);
    }

    if (count == 0) {
        strcat(list_buffer, " Aucun replay sauvegardé.\n");
    }
    strcat(list_buffer, "===========================");
    
    send_simple_packet(fd, CMD_REPLAYS_LIST, list_buffer);
}