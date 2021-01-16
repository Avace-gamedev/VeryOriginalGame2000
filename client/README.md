The client !

It is still very simple, it can connect to and communicate with the server, it starts by downloading the tilemap, tileset and weapon list before showing the game. Here are the available features:

Basics

- Character moves and shoots, health is displayed, names too, entities are spawned if the server says so.
- Client prediction: the client is always ahead of the server concerning player inputs, so it plays the inputs that have not been ACKed by the server yet in advance (if control frames are not received by the server, the client will teleport to a different location because the prediction will not match the server data)
- World interpolation: show a previous, interpolated state of the world to counter lag spikes (to an extent) and to show smooth dynamics
- Line of sight: LineShader implements a LOS algorithm in a GLSL shader.
  My previous version using the CPU and clever algorithm [like this one](https://www.redblobgames.com/articles/visibility/) was kind of slow, I started by computing polygons representing my tilemap to minimize the number of vertices then applied the algorithm at each frame, but with a around 200 vertices on my map my computer was running at 10 FPS. With the GPU implementation, even if it is highly inefficient, it works like a charm.

## Run the client

Install [Haxe](https://haxe.org/) and [HaxeFlixel](https://haxeflixel.com/). Setup them correctly.

Run `lime test neko`, if you add `-debug` you'll see ghosts following every entity, the ghosts are the last position received from the server for those entities.
