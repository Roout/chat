**Main points:**  
1. 
2. to formalize the data exchange even further, the server may implement an API
    - define  specific content format
    - parse it.
3. scheduling system to prioritize incoming requests from clients to accommodate them
4. to prevent abuse and maximize availability, the server software may limit the availability to clients (agains DoS attacks)
5. encryption should be applied if sensitive information is to be communicated between the client and the server. 

TCP protocol will be used.  

Done:
[+] read data from client
[+] pass read data to server  
[+] broadcast recieved data to all clients
[+] timely remove all connections that were closed

TODO:
Fix errors and add multithreading safety:

[Good answer](https://stackoverflow.com/a/40588070/11468611)
1. Concurrent execution of functions that access the same socket.
2. Concurrent execution of delegates that enclose the same socket.
3. Interleaved execution of delegates that write to the same socket.
[About async_write](https://www.boost.org/doc/libs/1_73_0/doc/html/boost_asio/reference/async_write/overload7.html)

[+] strand for callbacks (solve the issue #1)
    Reasons (same for the client and session):
        - prevent data race around ostream;
        - prevent data race around the buffer,
         i.e. one OnWrite function hasn't return yet and another one started.
         However It still won't solve fully problem with buffers if OnWrite can't send 
         all message at once!

[-] different buffers to avoid race when writing/reading in the same direction/
    I need several buffers ( maybe look through scatter/gether IO idiom) when writing for the case
    when one handle hasn't yet sent all message but the situation force you to send another one.
    It's important to have  


TODO #2:
[-] remove server pointer from the session
[-] reorganize work with buffers 
[-] 

As result: 
clients can write to 'chatroom' (now it's just a server) and read all text shown in the chat!