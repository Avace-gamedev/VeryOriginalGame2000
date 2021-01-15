- **time**: utilities to measure time on Windows
- **bitarray**:
  memory efficient representation of a boolean array, each boolean is storder in a single bit. This is definitely not important for this project, but it was fun to write!
- **xorshift64plus**:
  implementation of the pseudo-random algorithm used to randomize bullet angles in the game. The state of this one is defined by 8 bytes (+ 8 additionnal bytes to store initial seed, should probably be removed..) so it is easy to share at each frame to synchronize the client and server visuals
- **utils**:
  really self explanatory
