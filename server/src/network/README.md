**!!! WARNING !!!**
This code uses winsock api to handle sockets, it will probably need adjustments to work on unix systems.

## NetworkFrame

Messages exchanged over the network are encapsulated in a NetworkFrame. It is basically a buffer with some helpful methods. The struct also contains a `sender` field that is used for incoming frames only. It stores the ID that the UDP server assigned to the address that sent the frame.

By convention, the buffer contains:

- byte 0 : opcode
- bytes 1-2: payload size
- bytes ...: payload

The buffer in NetworkFrame is of type BUFFER which is a union between a stack char array of size BUFFER_SIZE and a char pointer.
Because most of our payloads (especially during the actual game) will be small, we can avoid `malloc` and `free` calls by storing them in the stack array.
If the actual message is too long, a new buffer will be `malloc`-ed and stored in the char pointer.
If you know the size of your payload at initialization, you can pass it to the constructor. It will allocate a buffer on the heap if necessary.

## Servers

Two classes are defined here:

- UDPServer: used during the actual gameplay, this is the most basic server you could think of.

  - **Init**:
    When someone sends an init message (a frame with opcode init containing the string 'hithere'), they get a server init answer (a frame with opcode init, the string 'hithere' and the ID they've been assigned).
    The server then stores the peer's address in a slot.
    The number of available slots is capped by the constant MAX_PEERS.

  - **Update**:
    The `update` method takes a timeout that will be given to `select`, and it stores any incoming frame in a NetworkFrame queue.
    The frames are then accessible through `pop()`.
    At each update call it will check if the active peers' last communication is recent enough (less than SERVER_TIMEOUT).
    If not, the peer is discarded from active peers, and its ID is added to `lost_communications`.
    This vector is accessible through `lostConnections()`.
    The call to `lostConnections` returns the vector of lost IDs since last time it was called.

- TCPServer: used for sending big files before starting the game, eg. tilemaps, tilesets, weapon list, ...
  - **Init**:
    You initialize the server by giving it a port on which it will listen and a vector of NetworkFrames that will be sent to the incoming client.
  - **Update**:
    The server doesn't really check anything. It listens for a connect (only one), when it got one it starts sending the files in order, in packets of TCP_PACKET_SIZE bytes.
    When it is done, it closes itself.
