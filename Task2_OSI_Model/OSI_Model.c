/*
 * OSI_Model.c  —  CS302 Computer Networks  |  Lab 2, Task 2
 * Simulation of the Seven-Layer OSI Protocol Model
 *
 * Each layer function:
 *   1. Receives the message passed from the layer above.
 *   2. Prepends its own protocol header to the message.
 *   3. Prints the newly encapsulated message.
 *   4. Passes the new message to the next lower layer.
 *
 * Message flow:
 *   Application (7) → Presentation (6) → Session (5) → Transport (4)
 *   → Network (3) → Data Link (2) → Physical (1)
 *
 * Build:
 *   gcc -Wall -Wextra -o OSI_Model OSI_Model.c
 * Run:
 *   OSI_Model.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum buffer for the growing encapsulated message.
 * 7 headers × 64 chars max + original message up to 512 chars = well under 1024. */
#define MAX_BUF  2048

/* Each header must be ≤ 64 characters as required by the assignment. */
#define HDR_APP   "APP|"      /* 4  chars */
#define HDR_PRES  "PRES|"     /* 5  chars */
#define HDR_SESS  "SESS|"     /* 5  chars */
#define HDR_TRANS "TRANS|"    /* 6  chars */
#define HDR_NET   "NET|"      /* 4  chars */
#define HDR_DATA  "DATA|"     /* 5  chars */
#define HDR_PHY   "PHY|"      /* 4  chars */

/* -----------------------------------------------------------------------
 * Forward declarations for the lower-layer functions.
 * ----------------------------------------------------------------------- */
void presentation_layer(const char *msg, int size);
void session_layer      (const char *msg, int size);
void transport_layer    (const char *msg, int size);
void network_layer      (const char *msg, int size);
void datalink_layer     (const char *msg, int size);
void physical_layer     (const char *msg, int size);

/* -----------------------------------------------------------------------
 * encapsulate()
 * Helper: prepend 'header' in front of 'msg' (length 'size') and
 * store the result in 'out_buf'. Returns the new message size,
 * or -1 if the buffer would overflow.
 * ----------------------------------------------------------------------- */
static int encapsulate(const char *header, const char *msg, int size,
                       char *out_buf, int out_max)
{
    int hdr_len  = (int)strlen(header);
    int new_size = hdr_len + size;

    if (new_size + 1 > out_max) {          /* +1 for the null terminator */
        fprintf(stderr, "ERROR: Buffer overflow prevented (message too large).\n");
        return -1;
    }

    /* Prepend header, then copy existing message */
    memcpy(out_buf,           header, (size_t)hdr_len);
    memcpy(out_buf + hdr_len, msg,    (size_t)size);
    out_buf[new_size] = '\0';              /* null-terminate for printf */

    return new_size;
}

/* =======================================================================
 * Layer 7 — Application Layer
 * This is the entry point; it accepts the raw user message.
 * ======================================================================= */
void application_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_APP, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("\n  Layer 7 — Application  : %s\n", new_msg);

    /* Pass encapsulated message to the next lower layer */
    presentation_layer(new_msg, new_size);
}

/* =======================================================================
 * Layer 6 — Presentation Layer
 * Handles data formatting, encryption, and compression.
 * ======================================================================= */
void presentation_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_PRES, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("  Layer 6 — Presentation : %s\n", new_msg);

    session_layer(new_msg, new_size);
}

/* =======================================================================
 * Layer 5 — Session Layer
 * Manages sessions (connections) between applications.
 * ======================================================================= */
void session_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_SESS, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("  Layer 5 — Session      : %s\n", new_msg);

    transport_layer(new_msg, new_size);
}

/* =======================================================================
 * Layer 4 — Transport Layer
 * Provides end-to-end data delivery (TCP/UDP segmentation).
 * ======================================================================= */
void transport_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_TRANS, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("  Layer 4 — Transport    : %s\n", new_msg);

    network_layer(new_msg, new_size);
}

/* =======================================================================
 * Layer 3 — Network Layer
 * Handles logical addressing and routing (IP).
 * ======================================================================= */
void network_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_NET, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("  Layer 3 — Network      : %s\n", new_msg);

    datalink_layer(new_msg, new_size);
}

/* =======================================================================
 * Layer 2 — Data Link Layer
 * Handles framing and error detection (MAC / Ethernet).
 * ======================================================================= */
void datalink_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_DATA, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("  Layer 2 — Data Link    : %s\n", new_msg);

    physical_layer(new_msg, new_size);
}

/* =======================================================================
 * Layer 1 — Physical Layer
 * Transmits raw bit streams over the physical medium.
 * This is the lowest layer; it prints the final frame and terminates.
 * ======================================================================= */
void physical_layer(const char *msg, int size)
{
    char new_msg[MAX_BUF];
    int  new_size = encapsulate(HDR_PHY, msg, size, new_msg, MAX_BUF);
    if (new_size < 0) return;

    printf("  Layer 1 — Physical     : %s\n", new_msg);
    printf("\n  [Physical Layer] Final frame ready for transmission on the wire.\n");
    printf("  Total encapsulated size: %d bytes.\n", new_size);
}

/* -----------------------------------------------------------------------
 * main()
 * ----------------------------------------------------------------------- */
int main(void)
{
    char user_msg[512];

    printf("================================================\n");
    printf("  CS302 Lab 2 — OSI Seven-Layer Protocol Model \n");
    printf("================================================\n");
    printf("\nEnter the application message: ");
    fflush(stdout);

    if (!fgets(user_msg, sizeof(user_msg), stdin)) {
        fprintf(stderr, "Error reading input.\n");
        return 1;
    }

    /* Strip the trailing newline character if present */
    int len = (int)strlen(user_msg);
    if (len > 0 && user_msg[len - 1] == '\n') {
        user_msg[len - 1] = '\0';
        len--;
    }

    if (len == 0) {
        fprintf(stderr, "Error: empty message. Please enter at least one character.\n");
        return 1;
    }

    printf("\nOriginal message : \"%s\"  (%d bytes)\n", user_msg, len);
    printf("\n--- Encapsulation  (Application → Physical) ---");

    /* Kick off encapsulation at the top (Application) layer */
    application_layer(user_msg, len);

    printf("\n================================================\n");
    return 0;
}
