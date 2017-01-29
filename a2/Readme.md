# Robust Chat
______________________________________________________________________________________________
### SUMMARY
------------------------
Built a simple server client chat application with the following specifications:
1. Clients can message each other using client names, in the following format;
<clientname>:<message>
2. Clients can broadcast the same message to all peers.
3. +online displays a list of peers currently online.
4. Maximum 5 clients can connect at a time. Additional clients will not be allowed to connect in the same session, even if one of them later disconnects.
5. On connection, each client is assigned a 5 digit random ID and a 10 character long random name.
_______________________________________________________________________________________________
### IMPLEMENTATION DETAILS
-------------------------
*CLIENT SIDE:
Using poll system call (similar to select system call) to wait for:
a. Client to type a message, or
b. Server to write a message on the socket
If the client types a message, it is sent to the server, where the string is handled appropriately.
If the server sends a message, it is displayed on the screen, along with the sender name.
*SERVER SIDE:
Well formatted incoming messages from clients are sent to the message queue. Not well formatted input is returned with an appropriate error message. If the input string is a command, it is handled appropriately.
Messages from clients are added to a message queue. This message queue is checked continuously for any incoming messages.
These messages are then delivered to their respective clients.
_________________________________________________________________________________________________
### TEST CASES

-----------------------------------------------------------------
| ID | TEST CASE |
|---------------|--------------------------------------------------|
| 1 | Client name not mentioned |
| 2 | Client name does not exist |
| 3 | Delimiter (colon) not present |
| 4 | Sending messages to yourself |
| 5 | Message sent to a client which has disconnected |
| 6 | More than 5 clients trying to connect |
| 7 | Server has crashed |
| 8 | Trying to connect when the server is not running |
| 9 | Port being used by some other process |
__________________________________________________________________________________________________
### TESTING REPORT

-----------------------------------------------------------------------------------------------------------------------------------------
| ID | Input | Expected Output | Actual Output | Result |
|--------|---------------------------------|---------------------|--------------------------|-------------------------------------------------|
| 1 | message | Please include client name or command | Please include client name or command | Passed |
| 2 | client6:message | Client does not exist | Client does not exist | Passed |
| 3 | client0:message (in client0)| Cannot send messages to yourself | Cannot send message to yourself | Passed |
| 4 | More than 5 Clients | Limit exceeded | Limit exceeded | Passed |
| 5 | Server crashed | Server disconnected | Server disconnected | Passed |
| 6 | using a busy Port | Port busy | Port busy | Passed |
| 7 | +command | Please include client name or command | Please include a client name or command | Passed |
