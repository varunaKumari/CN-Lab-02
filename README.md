# CS302 — Computer Networks | Lab 2

> **Title:** File Transfer with Acknowledgements & OSI Protocol Model Simulation  
> **Course:** CS302 Computer Networks  
> **Language:** C (Winsock2 API, Windows)  
> **Compiler:** GCC 6.3.0 (MinGW-W64)

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Project Structure](#2-project-structure)
3. [Prerequisites](#3-prerequisites)
4. [How to Compile](#4-how-to-compile)
5. [Task 1 — File Transfer Using Acknowledgements](#5-task-1--file-transfer-using-acknowledgements)
   - [Theory](#51-theory)
   - [Protocol Design](#52-protocol-design)
   - [How the Code Works](#53-how-the-code-works)
   - [How to Run](#54-how-to-run)
   - [Sample Output](#55-sample-output)
   - [File Verification](#56-file-verification)
6. [Task 2 — OSI Seven-Layer Protocol Model](#6-task-2--osi-seven-layer-protocol-model)
   - [Theory](#61-theory)
   - [How the Code Works](#62-how-the-code-works)
   - [How to Run](#63-how-to-run)
   - [Sample Output](#64-sample-output)
7. [Test File — alice29.txt](#7-test-file--alice29txt)
8. [Actual Test Results](#8-actual-test-results)
9. [Error Handling](#9-error-handling)
10. [Key Concepts Demonstrated](#10-key-concepts-demonstrated)

---

## 1. Project Overview

This lab contains **two independent programs** that demonstrate fundamental computer networking concepts:

| Task | Program | Marks |
|------|---------|-------|
| Task 1 | File Transfer with ACK mechanism (Client + Server) | 5 |
| Task 2 | OSI Seven-Layer Protocol Model Simulation | 5 |
| **Total** | | **10** |

---

## 2. Project Structure

```
lab-02/
│
├── Task1_File_Transfer/        ← Task 1 source code and executables
│   ├── Client.c                   Client program (sends the file)
│   ├── Server1.c                  Server program (receives the file)
│   ├── Client.exe                 Compiled executable (Windows)
│   ├── Server1.exe                Compiled executable (Windows)
│   ├── received_file.txt          Output file written by Server after transfer
│   └── README.md                  Task-specific quick-start guide
│
├── Task2_OSI_Model/            ← Task 2 source code and executable
│   ├── OSI_Model.c                OSI layer simulation program
│   ├── OSI_Model.exe              Compiled executable (Windows)
│   └── README.md                  Task-specific quick-start guide
│
├── test_files/                 ← Test input files
│   └── alice29.txt                25,572-byte Alice in Wonderland text file
│
├── LAB2_REPORT.md              ← Detailed lab report with actual results
└── README.md                   ← This file
```

---

## 3. Prerequisites

| Requirement | Detail |
|-------------|--------|
| Operating System | Windows 10 / 11 |
| Compiler | GCC (MinGW) — already installed at `C:\MinGW\bin\gcc.exe` |
| Library | Winsock2 (`ws2_32.lib`) — built into Windows, linked with `-lws2_32` |
| Terminal | Windows Command Prompt (`cmd.exe`) or PowerShell |

> **Note:** The `.exe` files are already compiled and included. You only need to recompile if you modify the source code.

---

## 4. How to Compile

Open a **Command Prompt** and run:

### Task 1 (Client and Server):

```bat
cd C:\Users\sd407\OneDrive\Desktop\lab-02\Task1_File_Transfer

gcc -Wall -Wextra -o Server1.exe Server1.c -lws2_32
gcc -Wall -Wextra -o Client.exe  Client.c  -lws2_32
```

### Task 2 (OSI Model):

```bat
cd C:\Users\sd407\OneDrive\Desktop\lab-02\Task2_OSI_Model

gcc -Wall -Wextra -o OSI_Model.exe OSI_Model.c
```

**Expected result:** No errors, no warnings. All three programs compile cleanly.

---

## 5. Task 1 — File Transfer Using Acknowledgements

### 5.1 Theory

TCP (Transmission Control Protocol) is a reliable, connection-oriented protocol that guarantees delivery of data but does **not** guarantee that one `send()` call equals one `recv()` call. Data may be split into multiple packets by the network stack.

This task implements a **Stop-and-Wait ARQ** (Automatic Repeat Request) protocol on top of TCP:

```
Client                              Server
  │  ──── Chunk #1 (5000 B) ──────► │  receives chunk, writes to file
  │  ◄──── ACK ──────────────────── │  sends acknowledgement
  │  ──── Chunk #2 (5000 B) ──────► │
  │  ◄──── ACK ──────────────────── │
  │          ...                     │
  │  ──── Last chunk (≤ 5000 B) ──► │
  │  ◄──── ACK ──────────────────── │
  │  ──── EOF signal (size = 0) ──► │  signals end of file
  │  ◄──── ACK ──────────────────── │  server closes output file
```

The **client never sends the next chunk until it receives the ACK for the current one.** This guarantees in-order, reliable delivery at the application level.

---

### 5.2 Protocol Design

A lightweight binary header precedes every data transfer:

```
 ┌─────────────────────────────────────┐
 │  4 bytes — uint32_t in network      │  ← chunk length (big-endian)
 │            byte order (htonl)       │
 ├─────────────────────────────────────┤
 │  chunk_length bytes of file data    │  ← actual payload
 └─────────────────────────────────────┘

 Server reply: "ACK" (exactly 3 bytes)
```

| Header value | Meaning |
|---|---|
| `> 0` | Number of data bytes that follow in this chunk |
| `= 0` | End-of-file (EOF) signal — no data follows |

**Why a binary header instead of a fixed 5000-byte message?**  
The last chunk of the file is usually smaller than 5000 bytes. Without a header, the server would not know how many bytes to expect. The 4-byte length header solves this cleanly.

**Why `htonl()` / `ntohl()`?**  
Different CPU architectures store integers in different byte orders (little-endian vs. big-endian). `htonl()` (host-to-network-long) converts to network byte order before sending; `ntohl()` converts back on the receiving end. This makes the protocol portable.

---

### 5.3 How the Code Works

#### `send_all()` and `recv_all()` — The Most Important Functions

TCP is a **stream** protocol. A single `send(1000 bytes)` might be received as `recv(600 bytes)` + `recv(400 bytes)`. Without handling this, a file transfer program will silently lose data.

Both programs use helper functions that loop until every byte has been sent or received:

```c
/* Keeps sending until all n bytes are delivered */
static int send_all(SOCKET sock, const char *buf, int n) {
    int sent = 0;
    while (sent < n) {
        int s = send(sock, buf + sent, n - sent, 0);
        if (s == SOCKET_ERROR) return -1;  // error
        sent += s;
    }
    return 0;  // success
}

/* Keeps receiving until all n bytes have arrived */
static int recv_all(SOCKET sock, char *buf, int n) {
    int received = 0;
    while (received < n) {
        int r = recv(sock, buf + received, n - received, 0);
        if (r <= 0) return -1;  // error or disconnected
        received += r;
    }
    return 0;  // success
}
```

#### Client Logic (`Client.c`)

```
1.  WSAStartup()          — initialise Winsock on Windows
2.  fopen(file, "rb")     — open input file in binary read mode
3.  socket()              — create TCP socket
4.  connect()             — connect to 127.0.0.1:8080
5.  LOOP:
      bytes_read = fread(buffer, 1, 5000, file)
      send_all(header = htonl(bytes_read))     ← always send header first
      if bytes_read == 0:
          recv_all(ack, 3)                     ← wait for final ACK
          BREAK  (EOF signal was sent)
      send_all(buffer, bytes_read)             ← send data chunk
      recv_all(ack, 3)                         ← wait for "ACK"
6.  closesocket(), fclose(), WSACleanup()
```

#### Server Logic (`Server1.c`)

```
1.  WSAStartup()          — initialise Winsock
2.  socket()              — create TCP socket
3.  bind(port 8080)       — bind to port
4.  listen()              — start listening
5.  accept()              — wait for client connection
6.  fopen(output, "wb")   — open output file in binary write mode
7.  LOOP:
      recv_all(header, 4)              ← read 4-byte length header
      chunk_len = ntohl(header)
      if chunk_len == 0:
          send_all("ACK", 3)           ← final ACK
          BREAK  (EOF received)
      recv_all(buffer, chunk_len)      ← read exactly chunk_len bytes
      fwrite(buffer, chunk_len, file)  ← write to output file
      send_all("ACK", 3)              ← send acknowledgement
8.  fclose(), closesocket(), WSACleanup()
```

---

### 5.4 How to Run

> **Important:** Start the **Server first**, then the Client. The server must be listening on port 8080 before the client tries to connect.

#### Step 1 — Open two Command Prompt windows

Press `Win + R`, type `cmd`, press Enter. Repeat to get a second window.

#### Step 2 — Window 1: Start the Server

```bat
cd C:\Users\sd407\OneDrive\Desktop\lab-02\Task1_File_Transfer
Server1.exe
```

You should see:
```
[SERVER] Listening on port 8080 ...
[SERVER] Waiting for client to connect...
```

**Keep this window open.**

#### Step 3 — Window 2: Run the Client

```bat
cd C:\Users\sd407\OneDrive\Desktop\lab-02\Task1_File_Transfer
Client.exe ..\test_files\alice29.txt
```

#### Step 4 — Verify the transferred file

After the transfer completes, run this in Window 2:

```bat
fc /b ..\test_files\alice29.txt received_file.txt
```

Expected output:
```
FC: no differences encountered
```

---

### 5.5 Sample Output

**Server Window:**
```
[SERVER] Listening on port 8080 ...
[SERVER] Waiting for client to connect...

[SERVER] Client connected from 127.0.0.1.
[SERVER] Writing received data to 'received_file.txt'.

[SERVER] Received chunk #1  (5000 bytes)  |  total so far: 5000 bytes
[SERVER] ACK sent for chunk #1.

[SERVER] Received chunk #2  (5000 bytes)  |  total so far: 10000 bytes
[SERVER] ACK sent for chunk #2.

[SERVER] Received chunk #3  (5000 bytes)  |  total so far: 15000 bytes
[SERVER] ACK sent for chunk #3.

[SERVER] Received chunk #4  (5000 bytes)  |  total so far: 20000 bytes
[SERVER] ACK sent for chunk #4.

[SERVER] Received chunk #5  (5000 bytes)  |  total so far: 25000 bytes
[SERVER] ACK sent for chunk #5.

[SERVER] Received chunk #6  (572 bytes)   |  total so far: 25572 bytes
[SERVER] ACK sent for chunk #6.

[SERVER] Received EOF signal from client.
[SERVER] Final ACK sent.
[SERVER] -------------------------------------------
[SERVER] Transfer complete. Chunks received : 6
[SERVER] Total bytes written to file        : 25572
[SERVER] Output file                        : received_file.txt
[SERVER] -------------------------------------------
```

**Client Window:**
```
[CLIENT] File '..\test_files\alice29.txt' opened. Size: 25572 bytes.

[CLIENT] Connecting to 127.0.0.1:8080 ...
[CLIENT] Connected to server.

[CLIENT] Sent chunk #1  (5000 bytes)  |  total so far: 5000 / 25572 bytes
[CLIENT] Waiting for ACK...
[CLIENT] ACK received: "ACK"

[CLIENT] Sent chunk #2  (5000 bytes)  |  total so far: 10000 / 25572 bytes
[CLIENT] Waiting for ACK...
[CLIENT] ACK received: "ACK"

[CLIENT] Sent chunk #3  (5000 bytes)  |  total so far: 15000 / 25572 bytes
[CLIENT] Waiting for ACK...
[CLIENT] ACK received: "ACK"

[CLIENT] Sent chunk #4  (5000 bytes)  |  total so far: 20000 / 25572 bytes
[CLIENT] Waiting for ACK...
[CLIENT] ACK received: "ACK"

[CLIENT] Sent chunk #5  (5000 bytes)  |  total so far: 25000 / 25572 bytes
[CLIENT] Waiting for ACK...
[CLIENT] ACK received: "ACK"

[CLIENT] Sent chunk #6  (572 bytes)   |  total so far: 25572 / 25572 bytes
[CLIENT] Waiting for ACK...
[CLIENT] ACK received: "ACK"

[CLIENT] EOF reached. Sent EOF signal (header = 0) to server.
[CLIENT] Server acknowledged EOF: "ACK"

[CLIENT] -------------------------------------------
[CLIENT] File transfer complete.
[CLIENT] Chunks sent      : 6
[CLIENT] Total bytes sent : 25572
[CLIENT] -------------------------------------------
```

---

### 5.6 File Verification

#### Method 1 — Binary file compare (Windows built-in):
```bat
fc /b ..\test_files\alice29.txt received_file.txt
```
✅ Result: `FC: no differences encountered`

#### Method 2 — MD5 hash comparison (PowerShell):
```powershell
Get-FileHash ..\test_files\alice29.txt    -Algorithm MD5
Get-FileHash received_file.txt             -Algorithm MD5
```
✅ Result: Both hashes = `5AEA23480D590D8E0DE3E7F357FDACDA` (identical)

---

## 6. Task 2 — OSI Seven-Layer Protocol Model

### 6.1 Theory

The **OSI (Open Systems Interconnection)** model is a conceptual framework that standardises the functions of a networking system into **seven distinct layers**. When a message is sent from one application to another over a network, it travels downward through all seven layers on the sender side, and upward through all seven layers on the receiver side.

**Encapsulation:** At each layer, a **protocol header** is added in front of the data received from the layer above. This process of wrapping data with headers is called encapsulation.

```
Sender Side                              Receiver Side
─────────────────────────────────────────────────────
7. Application   │  User Data          ▲  Remove APP header
6. Presentation  │  PRES + Data        │  Remove PRES header
5. Session       │  SESS + ...         │  Remove SESS header
4. Transport     │  TRANS + ...        │  Remove TRANS header
3. Network       │  NET + ...          │  Remove NET header
2. Data Link     │  DATA + ...         │  Remove DATA header
1. Physical      ▼  PHY + ...  ──────►    Remove PHY header
                     (transmitted over wire)
```

This program simulates the **sender side** — the downward encapsulation path.

---

### 6.2 How the Code Works

There are **seven functions**, one per OSI layer. Each function:

1. Receives `(message, size)` from the layer above.
2. Prepends its own header string to the beginning of the message using `memcpy()`.
3. Calculates the new size: `new_size = strlen(header) + size`.
4. Null-terminates the buffer (for safe use with `printf`).
5. Prints the encapsulated message.
6. Calls the next lower-layer function with the new message and size.

The `encapsulate()` helper function handles steps 2–4 and also **guards against buffer overflow** by checking that the message fits within the allocated 2048-byte buffer.

#### Layer Headers Used:

| Layer | Number | Header | Length | Meaning |
|-------|--------|--------|--------|---------|
| Application | 7 | `APP\|` | 4 chars | Application layer identifier |
| Presentation | 6 | `PRES\|` | 5 chars | Presentation layer identifier |
| Session | 5 | `SESS\|` | 5 chars | Session layer identifier |
| Transport | 4 | `TRANS\|` | 6 chars | Transport layer identifier |
| Network | 3 | `NET\|` | 4 chars | Network layer identifier |
| Data Link | 2 | `DATA\|` | 5 chars | Data Link layer identifier |
| Physical | 1 | `PHY\|` | 4 chars | Physical layer identifier |

> All headers are well within the **64-character maximum** required by the assignment.

#### Function Call Chain:

```c
main()
  └─► application_layer()      // Layer 7 — adds "APP|"
        └─► presentation_layer()   // Layer 6 — adds "PRES|"
              └─► session_layer()      // Layer 5 — adds "SESS|"
                    └─► transport_layer()   // Layer 4 — adds "TRANS|"
                          └─► network_layer()   // Layer 3 — adds "NET|"
                                └─► datalink_layer()  // Layer 2 — adds "DATA|"
                                      └─► physical_layer()  // Layer 1 — adds "PHY|"
                                            // prints final frame, terminates
```

---

### 6.3 How to Run

Open a **single Command Prompt window**:

```bat
cd C:\Users\sd407\OneDrive\Desktop\lab-02\Task2_OSI_Model
OSI_Model.exe
```

The program will prompt:
```
Enter the application message:
```

Type any message and press **Enter**.

---

### 6.4 Sample Output

**Input:** `Hello World`

```
================================================
  CS302 Lab 2 — OSI Seven-Layer Protocol Model
================================================

Enter the application message: Hello World

Original message : "Hello World"  (11 bytes)

--- Encapsulation  (Application → Physical) ---

  Layer 7 — Application  : APP|Hello World
  Layer 6 — Presentation : PRES|APP|Hello World
  Layer 5 — Session      : SESS|PRES|APP|Hello World
  Layer 4 — Transport    : TRANS|SESS|PRES|APP|Hello World
  Layer 3 — Network      : NET|TRANS|SESS|PRES|APP|Hello World
  Layer 2 — Data Link    : DATA|NET|TRANS|SESS|PRES|APP|Hello World
  Layer 1 — Physical     : PHY|DATA|NET|TRANS|SESS|PRES|APP|Hello World

  [Physical Layer] Final frame ready for transmission on the wire.
  Total encapsulated size: 44 bytes.

================================================
```

**What to observe:**
- Each layer adds its header **in front** of everything that came before it.
- The message grows with each layer (11 → 15 → 20 → 25 → 31 → 35 → 40 → 44 bytes).
- The Physical layer carries the **complete encapsulated frame** ready for transmission.

---

## 7. Test File — alice29.txt

`test_files/alice29.txt` is an excerpt from *Alice's Adventures in Wonderland* by Lewis Carroll — the same file used in the **Calgary Compression Corpus**, a standard benchmark for data compression and networking research.

| Property | Value |
|---|---|
| File size | 25,572 bytes |
| Chunk size | 5,000 bytes |
| Number of full chunks | 5 |
| Final (partial) chunk | 572 bytes |
| **Total chunks transferred** | **6** |

This file was chosen because it is:
- Large enough to require **multiple chunks** (tests the loop and ACK mechanism).
- Ends with a **partial chunk** (tests the final-chunk handling).
- Plain text (easy to visually verify correctness).

---

## 8. Actual Test Results

| Test | Expected | Result |
|---|---|---|
| Compilation (Server1.c) | 0 errors, 0 warnings | ✅ Passed |
| Compilation (Client.c) | 0 errors, 0 warnings | ✅ Passed |
| Compilation (OSI_Model.c) | 0 errors, 0 warnings | ✅ Passed |
| Client connects to Server | Connection established | ✅ Passed |
| Chunk count (25572 ÷ 5000) | 6 chunks (5×5000 + 1×572) | ✅ Passed |
| ACK received for every chunk | 6 ACKs + 1 final ACK | ✅ Passed |
| Server writes correct byte count | 25,572 bytes | ✅ Passed |
| `fc /b` file comparison | No differences | ✅ Passed |
| MD5 hash match | Hashes identical | ✅ Passed |
| OSI layers called in order (7→1) | APP→PRES→SESS→TRANS→NET→DATA→PHY | ✅ Passed |
| All 7 headers present in output | Yes | ✅ Passed |
| All headers ≤ 64 characters | Longest is "TRANS\|" = 6 chars | ✅ Passed |

---

## 9. Error Handling

Both Task 1 programs handle the following error conditions:

| Error Condition | How It Is Handled |
|---|---|
| `WSAStartup` fails | Prints error code, exits with code 1 |
| `socket()` fails | Prints WSA error code, cleans up, exits |
| `bind()` fails | Prints WSA error code, cleans up, exits |
| `connect()` fails | Prints WSA error code, cleans up, exits |
| File cannot be opened | Prints error message, cleans up, exits |
| Partial `send()` / `recv()` | Handled by `send_all()` / `recv_all()` loops |
| Client disconnects mid-transfer | `recv_all` returns -1, server prints error and stops |
| Chunk size > 5000 (invalid) | Server rejects chunk, prints error and stops |
| File write error | Server prints error and stops |
| Empty message in OSI program | Program prints error and exits gracefully |
| Buffer overflow in OSI program | `encapsulate()` checks size before writing, exits gracefully |

---

## 10. Key Concepts Demonstrated

### Task 1
| Concept | Where Applied |
|---|---|
| **TCP socket programming** | `socket()`, `bind()`, `listen()`, `accept()`, `connect()` |
| **Stop-and-Wait ARQ** | Client waits for ACK before sending the next chunk |
| **Binary protocol framing** | 4-byte length header sent before every chunk |
| **Network byte order** | `htonl()` on send, `ntohl()` on receive |
| **Partial read/write handling** | `send_all()` and `recv_all()` helper functions |
| **Binary file I/O** | `fopen("rb")` / `fopen("wb")`, `fread()`, `fwrite()` |
| **EOF detection** | `fread()` returns 0 → client sends header = 0 |
| **Winsock lifecycle** | `WSAStartup()` before use, `WSACleanup()` on exit |

### Task 2
| Concept | Where Applied |
|---|---|
| **OSI model encapsulation** | Each layer wraps the message with its own header |
| **Protocol layering** | Function chain simulates the PDU (Protocol Data Unit) model |
| **Buffer safety** | `encapsulate()` checks for overflow before writing |
| **Data as bytes** | `memcpy()` used instead of `strcat()` for portability |
| **Recursive abstraction** | Each layer is independent and knows only its immediate neighbour |
