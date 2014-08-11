This application is used to generated incast pattern traffic.

It consists of two parts: the client application running on the receiver and the server application running on the senders

The client application can generate N requests to senders for the data of D bytes. The server application responds with requested data.

Considering there are M senders, N may be larger than N. In this scenario, we try to load balance requests to senders. 