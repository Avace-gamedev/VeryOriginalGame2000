The server !

You'll find README files in the sub-directories.

## Run the server

```
cd /path/to/root/folder
mkdir build
cd build
cmake ..
cmake --build .
```

Then you should see a `server.exe` file in `build`.

THE SERVER USES WINSOCK2 TO OPEN SOCKETS, YOU'LL NEED TO ADAPT IT FOR UNIX-LIKE SYSTEMS.
