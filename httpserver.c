#include <fcntl.h>
#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"
#include "queue.h"
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/file.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void handle_connection(uintptr_t);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *worker_func();
queue_t *task_queue = NULL;

int main(int argc, char **argv) {
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int number_threads = 4;
    int opt;
    int port;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't': number_threads = atoi(optarg); break;
        default: printf("Usage: %s [-t threads] <port>\n", argv[0]); exit(EXIT_FAILURE);
        }
    }
    if (optind >= argc) {
        printf("Expected argument after options\n");
        printf("Usage: %s [-t threads] <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    port = atoi(argv[optind]);
    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    listener_init(&sock, port);
    pthread_t worker_threads[number_threads];
    task_queue = queue_new(number_threads);
    for (int i = 0; i < number_threads; i++) {
        pthread_create(&worker_threads[i], NULL, &worker_func, NULL);
    }
    while (true) {
        uintptr_t connfd = listener_accept(&sock);
        queue_push(task_queue, (void *) connfd);
    }
    return 0;
}
void *worker_func() {
    while (1) {
        void *elem;
        queue_pop(task_queue, &elem);
        uintptr_t val = (uintptr_t) elem;
        handle_connection(val);
    }
    return NULL;
}
void handle_connection(uintptr_t connfd) {
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
    close(connfd);
}
void handle_get(conn_t *conn) {
    const Response_t *res = NULL;
    char *uri = conn_get_uri(conn);
    pthread_mutex_lock(&lock);
    bool existed = access(uri, F_OK) == 0;
    int fd = open(uri, O_RDONLY);
    flock(fd, LOCK_SH);
    pthread_mutex_unlock(&lock);
    if (fd == -1) {
        if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
            conn_send_response(conn, res);
        } else if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
            conn_send_response(conn, res);
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            conn_send_response(conn, res);
        }
    } else {
        struct stat file_stat;
        fstat(fd, &file_stat);
        uint64_t file_size = file_stat.st_size;
        if (stat(uri, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                res = &RESPONSE_FORBIDDEN;
                conn_send_response(conn, res);
            } else {
                res = conn_send_file(conn, fd, file_size);
                close(fd);
                if (res == NULL && existed) {
                    res = &RESPONSE_OK;
                } else if (res == NULL && !existed) {
                    res = &RESPONSE_CREATED;
                }
            }
        }
    }
    char *b = "Request-Id";
    char *a = conn_get_header(conn, b);
    fprintf(stderr, "%s,/%s,%d,%s\n", "GET", uri, response_get_code(res), a);
}

void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
    char *uri = conn_get_uri(conn);
    char *b = "Request-Id";
    char *a = conn_get_header(conn, b);
    fprintf(stderr, "/%s,%d,%s\n", uri, response_get_code(&RESPONSE_NOT_IMPLEMENTED), a);
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    pthread_mutex_lock(&lock);
    bool existed = access(uri, F_OK) == 0;
    int fd = open(uri, O_CREAT | O_WRONLY, 0600);
    flock(fd, LOCK_EX);
    pthread_mutex_unlock(&lock);
    if (fd < 0) {
        debug("%s: %d", uri, errno);
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            goto out;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            goto out;
        }
    }
    ftruncate(fd, 0);
    if (res == NULL && existed) {
        res = &RESPONSE_OK;
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
    }
    conn_recv_file(conn, fd);
    char *b = "Request-Id";
    char *a = conn_get_header(conn, b);
    fprintf(stderr, "%s,/%s,%d,%s\n", "PUT", uri, response_get_code(res), a);
    close(fd);
out:
    conn_send_response(conn, res);
}
