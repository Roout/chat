**Main points:**  
1. to formalize the data exchange, the server may implement an API
    - define specific content format
    - parse it.
2. scheduling system to prioritize incoming requests from clients to accommodate them
3. to prevent abuse and maximize availability, the server software may limit the availability to clients (agains DoS attacks)
4. encryption should be applied if sensitive information is to be communicated between the client and the server. 

TCP protocol will be used.  

Done:
[x] read data from client
[x] pass read data to server  
[x] broadcast recieved data to all clients
[x] timely remove all connections that were closed

TODO #1
[ ] Add safe client disconnection
[ ] Add safe server disconnection
[ ] Fix errors and add multithreading safety:

[Good answer](https://stackoverflow.com/a/40588070/11468611)
1. Concurrent execution of functions that access the same socket.
2. Concurrent execution of delegates that enclose the same socket.
3. Interleaved execution of delegates that write to the same socket.
[About async_write](https://www.boost.org/doc/libs/1_73_0/doc/html/boost_asio/reference/async_write/overload7.html)

[x] strand for callbacks (solve the issue #1)
    Reasons (same for the client and session):
        - prevent data race around ostream;
        - prevent data race around the buffer,
         i.e. one OnWrite function hasn't return yet and another one started.
         However It still won't solve fully problem with buffers if OnWrite can't send 
         all message at once!

[ ] different buffers to avoid race when writing/reading in the same direction/
    I need several buffers ( maybe look through scatter/gether IO idiom) when writing for the case
    when one handle hasn't yet sent all message but the situation force you to send another one.

TODO #2
[ ] remove server pointer from the session
[x] reorganize work with buffers 

TODO #3
[ ] make copy of the project and apply cooroutings!

Expected result: 
[ ] simple protocol & parser for text messages
    [ ] define message structure
    [ ] define parser
[ ] one chatroom with several users
    [ ] it will broadcast if somebody joined
    [ ] it will broadcast if somebody leave
    [ ] timeouts with timers for the server to avoid server's abuse
    [ ] force to choose username
    [ ] add login/logout
    [ ] fix sequrity issues (ssl?) 
    [ ] encryption?
[ ] support multithreading for server as there will be several clients who can write/read to/from 'chatroom'
[ ] implement some simple GUI for the clients
[ ] one client written on LUA