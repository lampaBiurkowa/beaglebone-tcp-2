#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT (12345)
#define BUFFER_SIZE (1024)
#define INTERVAL (3)

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int shared_counter = 1;
int server_fd;

void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    free(client_socket);
    char buffer[BUFFER_SIZE];

    while (1) {
        pthread_mutex_lock(&counter_mutex);
        int current_value = shared_counter++;
        pthread_mutex_unlock(&counter_mutex);

        snprintf(buffer, BUFFER_SIZE, "%d\n", current_value);
        write(sock, buffer, strlen(buffer));
        sleep(INTERVAL);
    }

    close(sock);
    printf("Client disconnected\n");
    return NULL;
}

void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("\nSIGINT received. Shutting down server...\n");
        close(server_fd);
        exit(0);
    }
}

int main() {
    int new_socket, *client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("error initializing socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("error binding to an address");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 3);
    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("error during accepting of a connectoin");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        printf("New connection accepted\n");

        client_socket = (int *)malloc(sizeof(int));
        *client_socket = new_socket;

        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_socket) != 0) {
            perror("error creating a thread");
            free(client_socket);
            close(new_socket);
        }

        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
