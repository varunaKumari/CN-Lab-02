# Task 1 — File Transfer Using Acknowledgements

## Overview
Client and Server programs that transfer a file over TCP in 5000-byte chunks,
using an ACK (acknowledgement) mechanism between each chunk.

## Files
| File | Description |
|---|---|
| `Client.c` | Reads a file, connects to Server1, sends file in chunks, waits for ACK |
| `Server1.c` | Accepts connection, receives chunks, writes file, sends ACK |
| `Client.exe` | Compiled Windows executable |
| `Server1.exe` | Compiled Windows executable |
| `received_file.txt` | Output file written by the server after a transfer |
| `test_task1.ps1` | PowerShell test script |

## Compilation

Open a Command Prompt in this folder:

```bat
gcc -Wall -Wextra -o Server1.exe Server1.c -lws2_32
gcc -Wall -Wextra -o Client.exe  Client.c  -lws2_32
```

## Execution

### Terminal 1 — Start the server first:
```bat
Server1.exe
```

### Terminal 2 — Run the client:
```bat
Client.exe ..\test_files\alice29.txt
```

## Protocol

```
CLIENT                           SERVER
  |                                |
  |--- [4-byte size = 5000] ------>|
  |--- [5000 bytes of data] ------>|  Server writes chunk to file
  |<--- "ACK" (3 bytes) ----------|
  |                                |
  |--- [4-byte size = 5000] ------>|
  |--- [5000 bytes of data] ------>|
  |<--- "ACK" -------------------- |
  |          ...                   |
  |--- [4-byte size = N (final)] ->|  (N can be < 5000)
  |--- [N bytes of data] --------->|
  |<--- "ACK" --------------------|
  |                                |
  |--- [4-byte size = 0] --------->|  EOF signal
  |<--- "ACK" --------------------|  Server closes file
```

## File Verification

After transfer, compare files with:
```bat
fc /b ..\test_files\alice29.txt received_file.txt
```
Expected output: `FC: no differences encountered`

Or use MD5 hash:
```powershell
Get-FileHash ..\test_files\alice29.txt    -Algorithm MD5
Get-FileHash received_file.txt             -Algorithm MD5
```
Both hashes must match.
