#ifndef SESSION_H
#define SESSION_H

// Include basic network and game rule definitions
#include "network.h" 
#include "awale.h" 

#define MAX_CLIENTS 30
#define MAX_GAMES 15
#define MAX_SPECTATORS 10
#define MAX_FRIENDS 10
#define MAX_MOVES 100

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
    char friends[MAX_FRIENDS][32];
    int friend_count;
} Player;

// Game room (session) structure
typedef struct {
    int session_id;                      // Room ID, 0 means room is free
    int player1_fd;                      // Player 1 (first to move)
    int player2_fd;                      // Player 2 (second to move)
    int is_private;                      // Public = 0, Private = 1;
    int spectators[MAX_SPECTATORS];      // Array of spectator FDs
    int spectator_count;
    int move_count;                      // record the number of total moves
    const char* board_history[MAX_MOVES];      // record every board
    char* player1_name;      
    char* player2_name;
    Board* board;
} GameSession;

// External declaration of global state arrays (memory allocated in session.c)
extern Player all_players[MAX_CLIENTS];
extern GameSession all_games[MAX_GAMES];

// Network utils
void send_simple_packet(int fd, CommandType type, const char *msg);
void send_packet_to_player_and_spectator(GameSession* game, int fd, CommandType type, const char *msg);

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

// fonction of bio
void set_player_bio(int fd, const char *bio);
void send_player_bio(int requester_fd, const char *target_name);

// fonction of friend
void add_friend(int fd, const char *friend_name);
void toggle_private_mode(int fd);
int is_friend(Player *owner, const char *spectator_name);

// fonction of spectator
void send_games_list(int fd);
void join_game_as_spectator(int fd, const char *target_name);

// In-game operations
void handle_play_move(int fd, int pit_index);
void handle_victory(GameSession* game, int winning_player_fd);

// Board visualization
char* printScoreAndBoard(GameSession* game, int fromViewOfPlayerId, int currentPlayerTurn);

// Internal helper utilities
Player* find_player_by_fd(int fd);
Player* find_player_by_name(const char *name);
GameSession* find_empty_game_slot();
GameSession* find_game_by_id(int game_id);
GameSession* find_game_by_player_id(int player_id);
int get_player_number(GameSession* game, int player_id);

#endif