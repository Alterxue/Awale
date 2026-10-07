#ifndef NETWORK_H
#define NETWORK_H

// Define command types (Opcode) for communication
typedef enum {
    CMD_LOGIN = 1,          // Login / Register nickname
    CMD_LIST_PLAYERS,       // Request the list of online players in the lobby
    CMD_PLAYERS_LIST,       // Server response containing the online players list
    CMD_CHALLENGE,          // Initiate a challenge
    CMD_CHALLENGE_INCOMING, // Notification of an incoming challenge
    CMD_CHALLENGE_ACCEPT,   // Accept a challenge
    CMD_CHALLENGE_REJECT,   // Reject a challenge
    CMD_GAME_START,         // Notification that the game has started
    CMD_PLAY_MOVE,          // Client sends a move (pit index)
    CMD_BOARD_STATE,        // Server broadcasts the updated board state
    CMD_CHAT_ALL,           // Global chat message in the lobby
    CMD_SET_BIO,            // modify the bio
    CMD_GET_BIO,            // request to check the Bio of someone else
    CMD_SHOW_BIO,           // the server gives the Bio
    CMD_SPECTATE,           // request to watch a match
    CMD_LIST_GAMES,         // client request the list of match
    CMD_GAMES_LIST,         // server: respond with the list of the match   
    CMD_ERROR,              // Error message (e.g., invalid move, name taken)
    CMD_SUCCESS,            // Success confirmation message
    CMD_FRIEND_ADD,         // Add a friend 
    CMD_SET_PRIVATE,        // Set a game to mode private
    CMD_SAVE_GAME,          // Save the current game
    CMD_LIST_REPLAYS,       // Check all the games in replay list
    CMD_REPLAYS_LIST,       // The server sends the replay list
    CMD_REPLAY              // The client asks to play one certain replay
} CommandType;

// Define the standard network packet structure
typedef struct {
    CommandType type;       // Type of the command
    int data_length;        // Length of the valid string in the payload
    char payload[512];      // Actual data (nickname, chat content, board ASCII, etc.)
} Packet;

// Network utility function prototypes
int send_packet(int fd, const Packet *p);
int receive_packet(int fd, Packet *p);

#endif