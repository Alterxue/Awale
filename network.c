#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "network.h"

/**
 * @brief Reliably sends a complete Packet over the socket.
 * @param fd The destination socket file descriptor.
 * @param p Pointer to the Packet structure to be sent.
 * @return The total number of bytes sent on success, or -1 on failure.
 */
int send_packet(int fd, const Packet *p) {
    int total_sent = 0;
    int bytes_left = sizeof(Packet);
    int n;

    // Use a while loop to prevent TCP partial send issues.
    // Ensures the entire memory block of the Packet is sent before returning.
    while (total_sent < bytes_left) {
        n = send(fd, (char*)p + total_sent, bytes_left - total_sent, 0);
        if (n == -1) {
            perror("send_packet error");
            return -1;
        }
        total_sent += n;
    }
    return total_sent;
}

/**
 * @brief Reliably receives a complete Packet from the socket.
 * @param fd The source socket file descriptor.
 * @param p Pointer to the Packet structure to store the received data.
 * @return Total bytes received on success, 0 if client disconnected, or -1 on failure.
 */
int receive_packet(int fd, Packet *p) {
    int total_received = 0;
    int bytes_left = sizeof(Packet);
    int n;

    // Clear the structure before receiving to prevent garbage memory data.
    memset(p, 0, sizeof(Packet));

    // Ensures exactly one full Packet is received before returning.
    while (total_received < bytes_left) {
        n = recv(fd, (char*)p + total_received, bytes_left - total_received, 0);
        
        if (n == 0) {
            // 0 means the client has gracefully closed the connection.
            return 0;
        }
        if (n == -1) {
            perror("receive_packet error");
            return -1;
        }
        total_received += n;
    }
    return total_received; 
}