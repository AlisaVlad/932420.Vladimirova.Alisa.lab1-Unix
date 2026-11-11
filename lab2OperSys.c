#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

volatile sig_atomic_t got_signal = 0;

void sig_handler(int signo) {
    got_signal = 1;
}

int main() {
    int server_fd, client_fd = -1;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        exit(2);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        exit(3);
    }

    if (listen(server_fd, 16) == -1) {
        perror("listen");
        close(server_fd);
        exit(4);
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sig_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGHUP, &action, NULL) == -1) {
        perror("sigaction");
        close(server_fd);
        exit(5);
    }

    sigset_t blocked_mask, original_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) == -1) {
        perror("sigprocmask");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен");
    printf("Ожидание подключений и сигналов...\n");

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
        }

        int activity = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &original_mask);

        if (activity == -1) {
            if (errno == EINTR) {
                if (got_signal) {
                    printf("\n[Сигнал] Получен SIGHUP.\n");
                    got_signal = 0;
                }
            }
            else {
                perror("pselect");
                break;
            }
            continue;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            int new_fd = accept(server_fd, NULL, NULL);
            if (new_fd == -1) {
                if (errno != EINTR) {
                    perror("accept");
                }
            }
            else {
                printf("[Сеть] Новое подключение (fd=%d)\n", new_fd);
                if (client_fd == -1) {
                    client_fd = new_fd;
                    printf("[Сеть] Соединение fd=%d сохранено и активно.\n", client_fd);
                }
                else {
                    printf("[Сеть] Соединение fd=%d закрыто (уже есть активное).\n", new_fd);
                    close(new_fd);
                }
            }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

            if (bytes_read > 0) {
                printf("[Данные] Получено %zd байт от клиента fd=%d.\n", bytes_read, client_fd);
            }
            else if (bytes_read == 0) {
                printf("[Сеть] Клиент fd=%d закрыл соединение.\n", client_fd);
                close(client_fd);
                client_fd = -1;
            }
            else {
                if (errno != EINTR) {
                    perror("recv");
                    close(client_fd);
                    client_fd = -1;
                }
            }
        }
    }

    if (client_fd != -1) {
        close(client_fd);
    }
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, NULL);

    return 0;
}