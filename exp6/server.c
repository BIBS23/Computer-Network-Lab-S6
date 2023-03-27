#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_PACKET_SIZE 1024
#define WINDOW_SIZE 5

int main(int argc, char *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[MAX_PACKET_SIZE] = {0};
    int expected_seq_num = 0;

    // Create a TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));

    // Bind socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started and listening on port %d...\n", atoi(argv[1]));

    while (1) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("New client connected: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // Receive packets from the client and send acknowledgments
        while (1) {
            // Receive a packet from the client
            valread = recv(new_socket, buffer, MAX_PACKET_SIZE, 0);

            if (valread == 0) {
                // If the client has closed the connection
                printf("Connection closed by client.\n");
                break;
            } else if (valread < 0) {
                // If an error occurred while receiving data
                perror("recv failed");
                exit(EXIT_FAILURE);
            }

            int seq_num = buffer[0];

            if (seq_num == expected_seq_num) {
                // If the received packet has the expected sequence number
                printf("Packet %d received and acknowledged.\n", seq_num);

                // Send acknowledgment to the client
                char ack[4] = "ACK";
                send(new_socket, ack, strlen(ack), 0);

                expected_seq_num = (expected_seq_num + 1) % WINDOW_SIZE;
            } else {
                // If the received packet has a different sequence number
                printf("Packet %d received but not acknowledged. Discarding...\n", seq_num);
            }
        }

        // Close the connection
        close(new_socket);
    }

    return 0;
}
