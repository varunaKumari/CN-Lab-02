# Task 2 — Seven-Layer OSI Protocol Model

## Overview
A C program that simulates the OSI protocol stack by passing a message
downward through all seven layers, with each layer prepending its header.

## Files
| File | Description |
|---|---|
| `OSI_Model.c` | Source code implementing all 7 OSI layers |
| `OSI_Model.exe` | Compiled Windows executable |

## Compilation

```bat
gcc -Wall -Wextra -o OSI_Model.exe OSI_Model.c
```

## Execution

```bat
OSI_Model.exe
```

Then type your application message and press Enter.

## Layer Headers

| Layer | Number | Header Used | Length |
|---|---|---|---|
| Application  | 7 | `APP\|`   | 4 chars |
| Presentation | 6 | `PRES\|`  | 5 chars |
| Session      | 5 | `SESS\|`  | 5 chars |
| Transport    | 4 | `TRANS\|` | 6 chars |
| Network      | 3 | `NET\|`   | 4 chars |
| Data Link    | 2 | `DATA\|`  | 5 chars |
| Physical     | 1 | `PHY\|`   | 4 chars |

All headers are ≤ 64 characters as required by the assignment.

## Sample Output

```
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
```
