# Client-server application: simple chat
#### About:
Async client server chat written on C++ (std::c++17) using boost::asio library.
#### Target:
1. to formalize the data exchange, the server may implement an API
    - define specific content format
    - parse it.
2. scheduling system to prioritize incoming requests from clients to accommodate them
3. to prevent abuse and maximize availability, the server software may limit the availability to clients (agains DoS attacks)
4. encryption should be applied if sensitive information is to be communicated between the client and the server.

TCP protocol will be used.

#Done
- [x] read data from client
- [x] pass read data to server
- [x] broadcast recieved data to all clients
- [x] timely remove all connections that were closed

#TODO #1
- [ ] Add safe client disconnection
- [ ] Add safe server disconnection
Safe disconnection should be achieved by catching exception from io_context so
it need to be wrapped in try-catch block.
- [x] Fix errors and add multithreading safety:

[Good answer](https://stackoverflow.com/a/40588070/11468611)
1. Concurrent execution of functions that access the same socket.
2. Concurrent execution of delegates that enclose the same socket.
3. Interleaved execution of delegates that write to the same socket. 
[About async_write](https://www.boost.org/doc/libs/1_73_0/doc/html/boost_asio/reference/async_write/overload7.html)

- [x] strand for callbacks (solve the issue #1)
    Reasons (same for the client and session)
- prevent data race around ostream;
- prevent data race around the buffer, i.e. one OnWrite function hasn't return yet and another one started. However It still won't solve fully problem with buffers if OnWrite can't send all message at once!
- [x] wrap` Client::Write(text)` function by strand to avoid data race.
- [x] wrap `Session::Write(text)` function by strand to avoid data race.

**Description:**
Let client write some data to server in thread A while in thread B this client recieve message, read it, and send a reply from the `async_read_some` complection handler! => possible 2 `async_write` are executed concurrently.

**Solution:**
For now I solved (at least I think so) this problem using strand: I wrapped the write function in strand so that neither buffer/isWriting variables neither `async_write_some` operation aren't used\executed concurrently.

**Cons:**

- There advantage of the multiply threads almost gone. 
With such design only `async_read` operation can be run parallel with `async_write` operation but their complection handlers can't! It's because there will be data race around `std::ostream` object
if 2 completion handlers will be executed in one time. To sum up, all handlers are forced to go through
one `boost::asio::strand`.

- Also I wish that there Session or Client Write() function can be called from several threads at one point of time
and if the writing is going on it will just push to buffer text to be sent immediately **without queuing and going through strand one-by-one**.

-  I think I need at least to use other strand for reading operation as it can be used concurrently with writing operation. And may be `mutex` around `std::cout`. 

**Pros**

- Very easy to implement.

#TODO #2
- [ ] remove server pointer from the session (will be achieved when I define communication protocol: request(client)-reply(server) and message structure).
- [x] reorganize work with buffers.

#TODO #3
- [ ] make copy of the project and apply cooroutings!

#Expected result
- [ ] simple protocol & parser for text messages
    - [ ] define message structure
    - [ ] define parser
- [ ] one chatroom with several users
    - [ ] it will broadcast if somebody joined
    - [ ] it will broadcast if somebody leave
    - [ ] timeouts with timers for the server to avoid server's abuse
    - [ ] force to choose username
    - [ ] add login/logout
    - [ ] fix sequrity issues (ssl?)
    - [ ] encryption?
- [ ] support multithreading for server as there will be several clients who can write/read to/from 'chatroom'
- [ ] implement some simple GUI for the clients
- [ ] one client written on LUA