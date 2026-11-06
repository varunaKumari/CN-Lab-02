/*
 * Server1.c  —  CS302 Computer Networks  |  Lab 2, Task 1
 * File Transfer Server with Acknowledgement (ACK) Mechanism
 *
 * Protocol:
 *   For each chunk the client sends:
 *     [4-byte uint32 header = chunk length, big-endian]  [chunk bytes]
 *   Server writes chunk to file, then replies with 3 bytes: "ACK"
 *   When header value is 0 (EOF signal), server sends a final "ACK" and exits.
 *
 * Build:
 *   gcc -Wall -Wextra -o Server1 Server1.c -lws2_32
 * Run:
 *   Server1.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>

#define PORT         8080
#define CHUNK_SIZE   5000
#define OUTPUT_FILE  "received_file.txt"

/* -----------------------------------------------------------------------
 * send_all()
 * Sends exactly 'n' bytes from 'buf' to 'sock'.
 * TCP may send fewer bytes than requested in a single call; this loop
 * keeps sending until all bytes are delivered.
 * Returns 0 on success, -1 on socket error.
 * ----------------------------------------------------------------------- */
static int send_all(SOCKET sock, const char *buf, int n)
{
    int sent = 0;
    while (sent < n) {
        int s = send(sock, buf + sent, n - sent, 0);
        if (s == SOCKET_ERROR) {
            fprintf(stderr, "[SERVER] send() error: %d\n", WSAGetLastError());
            return -1;
        }
        sent += s;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * recv_all()
 * Receives exactly 'n' bytes into 'buf' from 'sock'.
 * TCP may deliver fewer bytes per recv() call; this loop accumulates
 * bytes until the full requested length has been received.
 * Returns 0 on success, -1 if the connection closed prematurely or error.
 * ----------------------------------------------------------------------- */
static int recv_all(SOCKET sock, char *buf, int n)
{
    int received = 0;
    while (received < n) {
        int r = recv(sock, buf + received, n - received, 0);
        if (r == 0) {
            fprintf(stderr, "[SERVER] Client disconnected unexpectedly.\n");
            return -1;
        }
        if (r == SOCKET_ERROR) {
            fprintf(stderr, "[SERVER] recv() error: %d\n", WSAGetLastError());
            return -1;
        }
        received += r;
    }
    return 0;
}

/* -----------------------------------------------------------------------
 * main()
 * ----------------------------------------------------------------------- */
int main(void)
{
    WSADATA wsa;
    SOCKET  listen_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    int    client_len;
    int    opt = 1;

    /* ----- 1. Initialise Winsock 2.2 ----- */
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[SERVER] WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    /* ----- 2. Create TCP socket ----- */
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        fprintf(stderr, "[SERVER] socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    /* Allow immediate reuse of the port after the server restarts */
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    /* ----- 3. Bind to port ----- */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[SERVER] bind() failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    /* ----- 4. Listen for incoming connections ----- */
    if (listen(listen_sock, 1) == SOCKET_ERROR) {
        fprintf(stderr, "[SERVER] listen() failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }
    printf("[SERVER] Listening on port %d ...\n", PORT);
    printf("[SERVER] Waiting for client to connect...\n\n");

    /* ----- 5. Accept one client connection ----- */
    client_len  = sizeof(client_addr);
    client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
    if (client_sock == INVALID_SOCKET) {
        fprintf(stderr, "[SERVER] accept() failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }
    printf("[SERVER] Client connected from %s.\n\n", inet_ntoa(client_addr.sin_addr));

    /* ----- 6. Open the output file for writing ----- */
    FILE *fp = fopen(OUTPUT_FILE, "wb");
    if (!fp) {
        fprintf(stderr, "[SERVER] Cannot open output file '%s'.\n", OUTPUT_FILE);
        closesocket(client_sock);
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }
    printf("[SERVER] Writing received data to '%s'.\n\n", OUTPUT_FILE);

    /* ----- 7. Receive chunks, write to file, send ACK ----- */
    char     data_buf[CHUNK_SIZE];
    int      chunk_num   = 0;
    long     total_bytes = 0;

    for (;;) {
        /* Step A: receive the 4-byte chunk-length header */
        uint32_t hdr_net;
        if (recv_all(client_sock, (char *)&hdr_net, sizeof(hdr_net)) < 0) {
            fprintf(stderr, "[SERVER] Error reading chunk header.\n");
            break;
        }

        /* Convert from network byte order to host byte order */
        uint32_t chunk_len = ntohl(hdr_net);

        /* Step B: chunk_len == 0 means EOF — no more data */
        if (chunk_len == 0) {
            printf("[SERVER] Received EOF signal from client.\n");
            /* Send final ACK and stop */
            if (send_all(client_sock, "ACK", 3) == 0) {
                printf("[SERVER] Final ACK sent.\n");
            }
            break;
        }

        /* Sanity check: reject unreasonably large chunks */
        if (chunk_len > CHUNK_SIZE) {
            fprintf(stderr, "[SERVER] Invalid chunk length %u — too large.\n", chunk_len);
            break;
        }

        /* Step C: receive exactly chunk_len bytes of data */
        if (recv_all(client_sock, data_buf, (int)chunk_len) < 0) {
            fprintf(stderr, "[SERVER] Error reading chunk #%d data.\n", chunk_num + 1);
            break;
        }

        /* Step D: write the chunk to the output file */
        size_t written = fwrite(data_buf, 1, (size_t)chunk_len, fp);
        if ((uint32_t)written != chunk_len) {
            fprintf(stderr, "[SERVER] File write error on chunk #%d.\n", chunk_num + 1);
            break;
        }

        chunk_num++;
        total_bytes += (long)chunk_len;
        printf("[SERVER] Received chunk #%d  (%u bytes)  |  total so far: %ld bytes\n",
               chunk_num, chunk_len, total_bytes);

        /* Step E: send ACK to client */
        if (send_all(client_sock, "ACK", 3) < 0) {
            fprintf(stderr, "[SERVER] Error sending ACK for chunk #%d.\n", chunk_num);
            break;
        }
        printf("[SERVER] ACK sent for chunk #%d.\n\n", chunk_num);
    }

    /* ----- 8. Clean up ----- */
    printf("[SERVER] -------------------------------------------\n");
    printf("[SERVER] Transfer complete. Chunks received : %d\n",   chunk_num);
    printf("[SERVER] Total bytes written to file        : %ld\n",  total_bytes);
    printf("[SERVER] Output file                        : %s\n",   OUTPUT_FILE);
    printf("[SERVER] -------------------------------------------\n");

    fclose(fp);
    closesocket(client_sock);
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}
