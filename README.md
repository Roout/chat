The client only has to understand the response based on the well-known application protocol, i.e. the content and the formatting of the data for the requested service

TODO:
1. define communications protocol.
2. to formalize the data exchange even further, the server may implement an API
    - define  specific content format
    - parse it.
3. scheduling system to prioritize incoming requests from clients to accommodate them
4. to prevent abuse and maximize availability, the server software may limit the availability to clients (agains DoS attacks)
5. encryption should be applied if sensitive information is to be communicated between the client and the server. 

TCP protocol will be used.