#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

// Include our custom network protocol
#include "network.h"

#define BUFFER_SIZE 1024

/**
 * @brief Helper function to parse user input and build a Packet
 */
void parse_user_input(char *input, Packet *p) {
    memset(p, 0, sizeof(Packet));
    
    // Remove the trailing newline character
    input[strcspn(input, "\n")] = 0;

    if (strncmp(input, "/login ", 7) == 0) {
        p->type = CMD_LOGIN;
        strcpy(p->payload, input + 7);
        p->data_length = strlen(p->payload);
    } 
    else if (strcmp(input, "/list") == 0) {
        p->type = CMD_LIST_PLAYERS;
    } 
    else if (strncmp(input, "/challenge ", 11) == 0) {
        p->type = CMD_CHALLENGE;
        strcpy(p->payload, input + 11);
        p->data_length = strlen(p->payload);
    } 
    else if (strncmp(input, "/accept ", 8) == 0) {
        p->type = CMD_CHALLENGE_ACCEPT;
        strcpy(p->payload, input + 8);
        p->data_length = strlen(p->payload);
    } 
    else if (strncmp(input, "/reject ", 8) == 0) {
        p->type = CMD_CHALLENGE_REJECT;
        strcpy(p->payload, input + 8);
        p->data_length = strlen(p->payload);
    } 
    else if (strncmp(input, "/play ", 6) == 0) {
        p->type = CMD_PLAY_MOVE;
        strcpy(p->payload, input + 6);
        p->data_length = strlen(p->payload);
    } 
    else if (strncmp(input, "/chat ", 6) == 0) {
        p->type = CMD_CHAT_ALL;
        strcpy(p->payload, input + 6);
        p->data_length = strlen(p->payload);
    } 
    else {
        // Unknown command, we will let the user know locally
        p->type = CMD_ERROR; // Using ERROR locally just to mark it as invalid
    }
}

void print_help_menu() {
    printf("\n=== Commandes Disponibles ===\n");
    printf(" /login <pseudo>       - Se connecter avec un pseudo\n");
    printf(" /list                 - Voir les joueurs en ligne\n");
    printf(" /challenge <pseudo>   - Défier un joueur\n");
    printf(" /accept <pseudo>      - Accepter un défi\n");
    printf(" /reject <pseudo>      - Refuser un défi\n");
    printf(" /play <num_trou>      - Jouer un coup (ex: /play 3)\n");
    printf(" /chat <message>       - Envoyer un message public\n");
    printf("=============================\n\n");
}

int main(int argc, char** argv) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    fd_set read_fds; 

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Client starting...\n");

    // initialise the server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        exit(EXIT_FAILURE);
    }

    // create the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    // connect to the socket
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server!\n");
    print_help_menu();
    printf("> ");
    fflush(stdout);

    // the loop 
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // read the input of client
        FD_SET(sockfd, &read_fds);       // read the input of the server

        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        // wait until the input
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            break;
        }

        // --- BRANCH A: receive the response from the server ---
        if (FD_ISSET(sockfd, &read_fds)) {
            Packet p;
            
            // Use the robust receive function from network.c
            int valread = receive_packet(sockfd, &p);
            
            if (valread <= 0) {
                printf("\nServer disconnected.\n");
                break;
            }
            
            // The logic to do based on server's CommandType
            printf("\r"); // Clear current line for cleaner printing
            switch (p.type) {
                case CMD_SUCCESS:
                    printf("[SUCCÈS] %s\n", p.payload);
                    break;
                case CMD_ERROR:
                    printf("[ERREUR] %s\n", p.payload);
                    break;
                case CMD_PLAYERS_LIST:
                    printf("\n%s\n", p.payload);
                    break;
                case CMD_CHALLENGE_INCOMING:
                    printf("\n[ALERTE] Le joueur '%s' vous a défié !\n", p.payload);
                    printf("Tapez /accept %s ou /reject %s\n", p.payload, p.payload);
                    break;
                case CMD_GAME_START:
                    printf("\n>>> %s <<<\n", p.payload);
                    break;
                case CMD_BOARD_STATE:
                    printf("\n%s\n", p.payload); // In the future, this will print the ASCII board
                    break;
                case CMD_CHAT_ALL:
                    printf("[CHAT PUBLIC] %s\n", p.payload);
                    break;
                default:
                    printf("[MESSAGE INCONNU] Type: %d\n", p.type);
                    break;
            }
            printf("> ");
            fflush(stdout);
        }

        // --- BRANCH B: receive the input of the client ---
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                
                // Do not process empty lines (just pressing Enter)
                if (buffer[0] == '\n') {
                    printf("> ");
                    fflush(stdout);
                    continue;
                }

                Packet p;
                parse_user_input(buffer, &p);

                // Check if parsing failed locally
                if (p.type == CMD_ERROR) {
                    printf("Commande inconnue. Veuillez utiliser /login, /challenge, etc.\n");
                } else {
                    // Send the formatted packet to the server
                    if (send_packet(sockfd, &p) < 0) {
                        perror("Send failed");
                        break;
                    }
                }
                
                // Print a prompt for the next input
                printf("> ");
                fflush(stdout);
            }
        }
    }

    close(sockfd);
    printf("Client exiting.\n");
    return 0;
}