# Client-server application: simple chat

## About

Async client server chat written on C++ (std::c++17) using boost::asio library.

## Target

1. to formalize the data exchange, the server may implement an API
    - define specific content format
    - parse it.
2. scheduling system to prioritize incoming requests from clients to accommodate them
3. to prevent abuse and maximize availability, the server software may limit the availability to clients (agains DoS attacks)
4. encryption should be applied if sensitive information is to be communicated between the client and the server.

TCP protocol will be used.

## TODO

- [x] read data from client
- [x] pass read data to server
- [x] broadcast received data to all clients
- [x] timely remove all connections that were closed
- [x] Add safe client disconnection
- [x] Add safe server disconnection
- [x] Fix errors and add multithreading safety:
- [x] can I remove server pointer from the session? - Can't
- [x] reorganize work with buffers.
- [ ] make copy of the project and apply cooroutings!

## Expected result

- [x] simple protocol & parser for text messages
  - [x] define message structure
  - [x] define parser
- [ ] one chatroom with several users
  - [ ] it will broadcast if somebody joined
  - [ ] it will broadcast if somebody leave
  - [ ] timeouts with timers for the server to avoid server's abuse
  - [ ] force to choose username
  - [ ] add login/logout
  - [ ] add SSL
- [ ] support multithreading for server as there will be several clients who can write/read to/from 'chatroom'
- [ ] implement some simple GUI for the clients
