# HTTP_Client_Server

This project is an implementation of the HTTP protocol in the form of a client and server. The client is able to "GET" from any standard web server. The server is able to serve any "GET" request from browsers/clients.

Note: client does not support caching or recursively retrieving embedded objects.  

## What is HTTP Protocol?

HTTP Protocol allows for transfer between a client and a server. Data such as files and images are shared with this protocol online. It is an application layer protocol, and it uses TCP. 


## How To Run Server and Client

1. Open up two different terminals (can be on the same machine or different vms)
2. Launch the server in one terminal using the following command:```./http_server port_number```
3. Launch the client in the other terminal using the following command: 
    - ```./http_client http://hostname[:port]/path/to/file``` 
    - ex: ```./http_client http://127.0.0.1/index.html```

    note: if there is no port number (:port), then code assumes port 80 (standard HTTP port)