/*
 * Client.c  —  CS302 Computer Networks  |  Lab 2, Task 1
 * File Transfer Client with Acknowledgement (ACK) Mechanism
 *
 * Protocol:
 *   For each 5000-byte chunk read from the file:
 *     1. Send [4-byte uint32 header = chunk length, big-endian]
 *     2. Send [chunk bytes]
 *     3. Wait for server to reply with 3 bytes: "ACK"
 *   When the file is exhausted, send header = 0 (EOF signal),
 *   then wait for the server's final "ACK".
 *
 * Build:
 *   gcc -Wall -Wextra -o Client Client.c -lws2_32
 * Run:
 *   Client.exe <path-to-file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define CHUNK_SIZE  5000

/* -----------------------------------------------------------------------
 * send_all()
 * Sends exactly 'n' bytes from 'buf' to 'sock'.
 * Returns 0 on success, -1 on error.
 * ----------------------------------------------------------------------- */
static int send_all(SOCKET sock, const char *buf, int n)
{
    int sent = 0;
    while (sent < n) {
        int s = send(sock, buf + sent, n - sent, 0);
        if (s == SOCKET_ERROR) {
            fprintf(stderr, "[CLIENT] send() error: %d\n", WSAGetLastError());
            return -1;
        }
        sent += s;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * recv_all()
 * Receives exactly 'n' bytes into 'buf' from 'sock'.
 * Returns 0 on success, -1 on error or premature disconnect.
 * ----------------------------------------------------------------------- */
static int recv_all(SOCKET sock, char *buf, int n)
{
    int received = 0;
    while (received < n) {
        int r = recv(sock, buf + received, n - received, 0);
        if (r == 0) {
            fprintf(stderr, "[CLIENT] Server closed the connection unexpectedly.\n");
            return -1;
        }
        if (r == SOCKET_ERROR) {
            fprintf(stderr, "[CLIENT] recv() error: %d\n", WSAGetLastError());
            return -1;
        }
        received += r;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * main()
 * ----------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    /* ----- 1. Initialise Winsock 2.2 ----- */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[CLIENT] WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    /* ----- 2. Open the file to transfer ----- */
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "[CLIENT] Cannot open file '%s'.\n", argv[1]);
        WSACleanup();
        return 1;
    }

    /* Report file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);
    printf("[CLIENT] File '%s' opened. Size: %ld bytes.\n\n", argv[1], file_size);

    /* ----- 3. Create a TCP socket ----- */
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "[CLIENT] socket() failed: %d\n", WSAGetLastError());
        fclose(fp);
        WSACleanup();
        return 1;
    }

    /* ----- 4. Connect to Server1 ----- */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    printf("[CLIENT] Connecting to %s:%d ...\n", SERVER_IP, SERVER_PORT);
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[CLIENT] connect() failed: %d\n", WSAGetLastError());
        closesocket(sock);
        fclose(fp);
        WSACleanup();
        return 1;
    }
    printf("[CLIENT] Connected to server.\n\n");

    /* ----- 5. Send file in CHUNK_SIZE chunks ----- */
    char    data_buf[CHUNK_SIZE];
    char    ack_buf[4];
    int     chunk_num  = 0;
    long    total_sent = 0;

    for (;;) {
        /* Step A: read up to CHUNK_SIZE bytes from the file */
        int bytes_read = (int)fread(data_buf, 1, CHUNK_SIZE, fp);

        /* Step B: send 4-byte length header (network byte order) */
        uint32_t hdr = htonl((uint32_t)bytes_read);
        if (send_all(sock, (char *)&hdr, (int)sizeof(hdr)) < 0) {
            fprintf(stderr, "[CLIENT] Failed to send header.\n");
            break;
        }

        /* bytes_read == 0 means we have reached the end of the file */
        if (bytes_read == 0) {
            printf("[CLIENT] EOF reached. Sent EOF signal (header = 0) to server.\n");
            /* Wait for the server's final ACK */
            if (recv_all(sock, ack_buf, 3) == 0) {
                ack_buf[3] = '\0';
                printf("[CLIENT] Server acknowledged EOF: \"%s\"\n", ack_buf);
            }
            break;
        }

        /* Step C: send the chunk data */
        if (send_all(sock, data_buf, bytes_read) < 0) {
            fprintf(stderr, "[CLIENT] Failed to send chunk #%d data.\n", chunk_num + 1);
            break;
        }

        chunk_num++;
        total_sent += bytes_read;
        printf("[CLIENT] Sent chunk #%d  (%d bytes)  |  total so far: %ld / %ld bytes\n",
               chunk_num, bytes_read, total_sent, file_size);
        printf("[CLIENT] Waiting for ACK...\n");

        /* Step D: wait for exactly 3 bytes "ACK" from server */
        if (recv_all(sock, ack_buf, 3) < 0) {
            fprintf(stderr, "[CLIENT] Did not receive ACK for chunk #%d.\n", chunk_num);
            break;
        }
        ack_buf[3] = '\0';
        printf("[CLIENT] ACK received: \"%s\"\n\n", ack_buf);
    }

    /* ----- 6. Summary ----- */
    printf("\n[CLIENT] -------------------------------------------\n");
    printf("[CLIENT] File transfer complete.\n");
    printf("[CLIENT] Chunks sent      : %d\n", chunk_num);
    printf("[CLIENT] Total bytes sent : %ld\n", total_sent);
    printf("[CLIENT] -------------------------------------------\n");

    /* ----- 7. Clean up ----- */
    closesocket(sock);
    fclose(fp);
    WSACleanup();
    return 0;
}
