# CS214 Project IV: Rock, Paper, Scissors (Concurrent Games)

## Authors
| **Name** | **NetID** |
|----------|-----------|
| [Vishal Nagamalla](https://github.com/Vishal-Nagamalla) | vn218 | 
| [Yuhan Li](https://github.com/HiT-T) | yl1234 |

---

## Description

This project implements a **Rock-Paper-Scissors** multiplayer server, `rpsd`, which accepts TCP connections from clients. Each client sends a **Play** request (containing the player’s name), and the server pairs players into matches. Once two players are paired, the server instructs each to make a move (Rock, Paper, or Scissors), determines the winner, and reports results back to the clients.

### Key Features

- **TCP-based communication**: The server listens on a user-specified port (passed as a command-line argument) and manages clients over **TCP sockets**.
- **Rock-Paper-Scissors Protocol (RPSP)**: Uses ASCII text messages delimited by `|` and `||`, with messages such as:
  - `P|PlayerName||` (client → server)  
  - `W|1||`, `B|OpponentName||`, `R|Result|OpponentMove||` (server → client)  
  - `M|ROCK||`, `C||`, `Q||`, etc.
- **Matchmaking**: Waits until at least two clients are available; then pairs them for a match.
- **Result Handling**: Compares moves to decide **win** (W), **lose** (L), or **draw** (D), and sends a **Result** message to each player. A disconnect mid-match is a **forfeit** (F).
- **Rematches**: Players can choose to **Continue** (C) or **Quit** (Q) after each round. If both continue, the server begins a new round with the same players.
- **Concurrent Games**: For full credit, `rpsd` supports multiple matches in parallel using **POSIX threads**, synchronized with mutexes.

---

## Design Choices

- **Message Parsing**:  
  - Accumulates incoming TCP data one byte at a time, identifying complete messages by detecting the `||` suffix.
  - Ensures strict adherence to the RPSP protocol format.

- **Matchmaking Logic**:  
  - Maintains a thread-safe **waiting queue**.
  - When two players are ready, dequeues them and launches a **match thread**.

- **Game Flow**:  
  1. `P|Name||` → client initiates play request  
  2. Server replies `W|1||`  
  3. When matched, server sends `B|OpponentName||`  
  4. Players send `M|ROCK||`, etc.  
  5. Server replies `R|W/L/D/F|OpponentMove||`  
  6. Players send `C||` to continue or `Q||` to quit

- **Concurrency**:  
  - Uses `pthread_create()` to spawn a match-handling thread per pair of players.
  - Global queues and player lists are synchronized with `pthread_mutex`.

- **Logging**:  
  - Prints connection events, match pairing, and game termination to `stdout` for transparency and debugging.

---

## Testing Strategy

1. **Manual Testing (Telnet/Netcat)**  
   - Connected manually using `telnet` or `nc` to verify correct behavior and protocol compliance.

2. **Automated Test Client**  
   - Implemented a C-based test client (`tests/test.c`) that simulates a full RPS match, including rematch and quit behavior.

3. **Concurrent Connections**  
   - Launched multiple clients in rapid succession to test concurrent game handling. Verified isolation of matches and correctness of results.

4. **Edge Cases**  
   - **Forfeit**: Simulated client disconnection mid-match and verified the opponent received `R|F|||`.  
   - **Repeat Names**: Ensured that a second client attempting to use the same name is rejected with `R|L|Logged in||`.  
   - **Rematches**: Confirmed that if one client sends `Q||` and the other `C||`, the latter is properly requeued.

---

## Compilation & Usage

To compile the server and client:

```bash
make
```

To run the server:

```bash
./rpsd <port>
```

To run test clients:

```bash
./tests/test 127.0.0.1 <port> Alice
./tests/test 127.0.0.1 <port> Bob
```