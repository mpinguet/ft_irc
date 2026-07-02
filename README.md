*This project has been created as part of the 42 curriculum by levincen, mpinguet, cmontaig*

## Description

**ft_irc** is a custom implementation of an Internet Relay Chat (IRC) server written in C++98. The goal of the project is to understand network programming, socket communication and concurrent client management while respecting the IRC protocol.

The server allows multiple clients to connect simultaneously, authenticate with a password, join channels, exchange private and channel messages, and perform common IRC operations. It is designed to be compatible with standard IRC clients.

This project focuses on:
- TCP socket programming
- Non-blocking I/O
- Event multiplexing (poll)
- IRC protocol implementation
- Client and channel management
- Object-oriented programming in C++98

---

## Features

- Password-protected server
- Multiple simultaneous client connections
- Nickname and username registration
- Channel creation and management
- Private messaging
- Channel messaging
- Operator privileges
- Basic IRC commands, including:
  - PASS
  - NICK
  - USER
  - JOIN
  - PART
  - PRIVMSG
  - KICK
  - INVITE
  - TOPIC
  - MODE

---

## Instructions

### Requirements

- C++ compiler supporting the C++98 standard
- Make
- Unix-like operating system (Linux/macOS)

### Compilation

```bash
make
```

This generates the executable:

```bash
./ircserv
```

### Running the server

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 mypassword
```

### Connecting with an IRC client

Example using HexChat:

1. Create a new network.
2. Connect to:
   - Host: `localhost`
   - Port: `6667`
3. Enter the server password.
4. Join a channel:

```
/join #general
```

---

## Project Structure

```
.
├── include/
├── src/
├── Makefile
└── README.md
```

---

## Resources

### IRC Documentation

- RFC 1459 – Internet Relay Chat Protocol
- RFC 2810 – IRC Architecture
- RFC 2811 – Channel Management
- RFC 2812 – Client Protocol
- RFC 2813 – Server Protocol

### Network Programming

- Linux `poll(2)` manual page
- Linux socket programming documentation
- cppreference.com (C++98 features)

### AI Usage

Artificial Intelligence tools were used to:
- clarify parts of the IRC protocol;
- explain socket programming concepts;
- understand the behavior of `poll()` and non-blocking sockets;
- improve documentation and README formatting.

All implementation, architecture, debugging, and testing were completed by the project authors.

---

## Testing

The server has been tested using:
- HexChat

Tests include:
- Multiple client connections
- Authentication
- Channel creation
- Messaging
- Operator commands
- Client disconnections
- Invalid command handling

