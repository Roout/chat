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
Fix errors and add multithreading safety
[-] strand for callbacks
[-] queue to avoid race when writing/reading in the same direction/

As result: 
clients can write to 'chatroom' (now it's just a server) and read all text shown in the chat!