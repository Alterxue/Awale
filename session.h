#ifndef SESSION_H
#define SESSION_H

// Include basic network and game rule definitions
// #include "network.h" 
// #include "awale.h" 

#define MAX_CLIENTS 30
#define MAX_GAMES 15
#define MAX_SPECTATORS 10

// Player state machine enumeration
typedef enum {
    STATE_CONNECTED = 0,    // Just connected, nickname not yet registered
    STATE_LOBBY_IDLE,       // Idling in lobby (can be challenged)
    STATE_LOBBY_WAITING,    // Processing a challenge request (locked state)
    STATE_PLAYING,          // Currently playing a game
    STATE_SPECTATING        // Currently spectating
} PlayerState;

// Player structure
typedef struct {
    int fd;                 // Socket descriptor, 0 means slot is free
    char pseudo[32];        // Player nickname
    char bio[256];          // Personal biography
    PlayerState state;      // Current state
    int current_game_id;    // Associated game room ID, -1 means not in a room
} Player;

// Game room (session) structure
typedef struct {
    int session_id;         // Room ID, 0 means room is free
    int player1_fd;         // Player 1 (first to move)
    int player2_fd;         // Player 2 (second to move)
    // AwaleBoard board;    // Placeholder: board data structure from awale.h
    int spectators[MAX_SPECTATORS]; // Array of spectator FDs
    int spectator_count;
} GameSession;

// External declaration of global state arrays (memory allocated in session.c)
extern Player all_players[MAX_CLIENTS];
extern GameSession all_games[MAX_GAMES];


// Lifecycle management
void init_player(int fd);
void remove_player(int fd);

// Player operations
void register_player_name(int fd, const char *name);
void send_lobby_list(int fd);

// Challenge system
void handle_challenge_request(int challenger_fd, const char *target_name);
void handle_challenge_reject(int rejector_fd, const char *challenger_name);
void create_game_session(int acceptor_fd, const char *challenger_name);

// In-game operations
void handle_play_move(int fd, int pit_index);

// Internal helper utilities
Player* find_player_by_fd(int fd);
Player* find_player_by_name(const char *name);

#endif