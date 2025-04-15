# CS214 Project IV: Rock, Paper, Scissors

## Authors
| **Name** | **NetID** |
|----------|-----------|
| [Vishal Nagamalla](https://github.com/Vishal-Nagamalla) | vn218 |
| [Yuhan Li](https://github.com/HiT-T) | yl2355 |

---

## Description

This project implements a **Rock-Paper-Scissors** multiplayer server, `rpsd`, which accepts TCP connections from clients. Each client sends a **Play** request (containing the player’s name), and the server pairs players into matches. Once two players are paired, the server instructs each to make a move (Rock, Paper, or Scissors), determines the winner, and reports results back to the clients.

### Key Features

- **TCP-based communication**: The server listens on a user-specified port (passed as a command-line argument) and manages clients over **TCP sockets**.
- **Rock-Paper-Scissors Protocol (RPSP)**: Uses ASCII text messages delimited by `|` and `||`, with messages such as:
  - `P|PlayerName||` (client → server)  
  - `W|1||`, `B|OpponentName||`, `R|Result|OpponentMove||` (server → client)  
  - `M|ROCK||`, `C`, `Q`, etc.
- **Matchmaking**: Waits until at least two clients are available; then pairs them for a match.
- **Result Handling**: Compares moves to decide **win** (W), **lose** (L), or **draw** (D), and sends a **Result** message to each player. A disconnect mid-match is a **forfeit** (F).
- **Rematches**: Players can choose to **Continue** (C) or **Quit** (Q) after each round. If both continue, the server begins a new round with the same players.
- **Concurrent Games** *(optional / full-credit)*: For full concurrency, `rpsd` supports multiple matches in parallel using threads or processes.

---

## Design Choices

- **Message Parsing**:  
  - Implemented a simple **state machine** or **buffer reader** that accumulates TCP data and identifies complete messages by searching for the terminating `||`.
  - Ensured strict adherence to the protocol format (`<LETTER>|<args>||` or single-letter messages like `C`/`Q`).

- **Matchmaking Logic**:  
  - Maintains a **queue of waiting players** (those who have sent `Play` but not yet matched).  
  - When a second player arrives, pairs them with the waiting player and sends both `Begin` messages.

- **Game Flow**:  
  1. **Play/Wait** handshake (client sends `P|Name||`, server replies `W|1||`).  
  2. **Begin**: When matched, server sends `B|OpponentName||`.  
  3. **Moves**: Each client sends `M|ROCK||` / `M|PAPER||` / `M|SCISSORS||`.  
  4. **Result**: Server decides winner and sends `R|<W/L/D/F>|<OpponentMove>||`.  
  5. **Continue/Quit**: Clients reply `C` or `Q`. If both continue, new round begins; otherwise, server closes connections.

- **Concurrency** *(if implemented)*:
  - Uses **threads** (via `pthread_create()`) to handle each player or each match in parallel, synchronizing shared data (e.g., the waiting queue) with **mutexes**.

- **Logging**:  
  - Writes basic log info to `stdout` (accepted connections, match outcomes) for troubleshooting and verification.

---

## Testing Strategy

1. **Manual Testing (Telnet/Netcat)**  
   - Connected via `telnet localhost <port>` or `nc localhost <port>` and typed messages (`P|Alice||`, `M|ROCK||`, etc.) to ensure correctness.
   - Observed server responses, verifying message formats and correctness of RPS outcomes.

2. **Automated Test Client**  
   - Wrote a small C or Python script that automatically sends a sequence of RPSP messages and checks the server’s replies (e.g., to handle edge cases like early quit, malformed messages, etc.).

3. **Concurrent Connections**  
   - For concurrency, opened multiple client connections in quick succession to confirm that each pair could proceed through the match process independently.

4. **Edge Cases**  
   - **Forfeit**: Simulated a client disconnect (e.g., by killing the client process) mid-match and confirmed that the server sends `R|F|||` to the other player.  
   - **Rematch**: Ensured that if both players send `C`, the server resets for a new round with the same players.  
   - **Invalid Input**: Tested partially malformed messages, though robust error handling was optional.

---

## Compilation & Usage

To compile the server:

```bash
make