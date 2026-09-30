#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "network.h"
#include "session.h"

#define PORT 8080    

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Array to keep track of all connected client sockets
    int client_sockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow the port to be reused immediately after restart
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind socket to IP and Port
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    
    int port = (argc == 2) ? atoi(argv[1]) : PORT;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. Start listening
    if (listen(server_fd, 5) < 0) {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    fd_set read_fds;

    // 5. Main event loop
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_sd = server_fd;

        // Add child sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &read_fds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for an activity on one of the sockets
        if (select(max_sd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            continue;
        }

        // --- BRANCH A: New connection coming in ---
        if (FD_ISSET(server_fd, &read_fds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept error");
                continue;
            }
            
            printf("New connection: FD is %d, IP is %s, Port is %d\n",
                   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add new socket to array and initialize player session
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Added to client list at index %d\n", i);
                    
                    // Call session.c to initialize player state
                    init_player(new_socket);
                    break;
                }
            }
        }

        // --- BRANCH B: Data coming from existing clients ---
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (sd > 0 && FD_ISSET(sd, &read_fds)) {
                Packet p;
                
                // Use our robust receive_packet function instead of raw recv
                int valread = receive_packet(sd, &p);
                
                if (valread == 0) {
                    // Client disconnected gracefully
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Client disconnected: IP %s, Port %d\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                           
                    // Call session.c to handle game forfeit and cleanup
                    remove_player(sd); 
                    
                    close(sd);
                    client_sockets[i] = 0; // Free the slot
                    
                } else if (valread > 0) {
                    // Route the packet to the correct session logic
                    switch(p.type) {
                        case CMD_LOGIN:
                            printf("FD %d requests nickname: %s\n", sd, p.payload);
                            register_player_name(sd, p.payload);
                            break;
                            
                        case CMD_LIST_PLAYERS:
                            printf("FD %d requested lobby list\n", sd);
                            send_lobby_list(sd);
                            break;

                        case CMD_CHALLENGE:
                            printf("FD %d challenged target: %s\n", sd, p.payload);
                            handle_challenge_request(sd, p.payload);
                            break;

                        case CMD_CHALLENGE_ACCEPT:
                            printf("FD %d accepted challenge from: %s\n", sd, p.payload);
                            create_game_session(sd, p.payload);
                            break;

                        case CMD_CHALLENGE_REJECT:
                            printf("FD %d rejected challenge from: %s\n", sd, p.payload);
                            handle_challenge_reject(sd, p.payload);
                            break;

                        case CMD_PLAY_MOVE:
                            printf("FD %d requests move at pit: %s\n", sd, p.payload);
                            handle_play_move(sd, atoi(p.payload));
                            break;
                            
                        case CMD_CHAT_ALL:
                            printf("FD %d sent global chat: %s\n", sd, p.payload);
                            Player *sender = find_player_by_fd(sd);
                            char sender_name[32] = "Anonyme";

                            if (sender != NULL && strlen(sender->pseudo) > 0) {
                                strcpy(sender_name, sender->pseudo);
                            }
                            char formatted_message[512];
                            snprintf(formatted_message, sizeof(formatted_message), "[%s] : %s", sender_name, p.payload);
                            strncpy(p.payload, formatted_message, sizeof(p.payload) - 1);
                            p.payload[sizeof(p.payload) - 1] = '\0'; 
                            p.data_length = strlen(p.payload);

                            // Broadcast chat message to everyone in the lobby
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                int target_sd = client_sockets[j];
                                // Send to all connected clients except the sender
                                if (target_sd > 0 && target_sd != sd) {
                                    send_packet(target_sd, &p); 
                                }
                            }
                            break;

                        default:
                            printf("Received unknown command type: %d\n", p.type);
                            break;
                    }
                }
            }
        }
    }

    return 0;
}