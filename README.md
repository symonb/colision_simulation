# colision_simulation

Inspirated from game of life:
https://youtu.be/scvuli-zcRc
https://youtu.be/0Kx4Y9TVMGg

Particles have:
- flavour (type of molecule)
- mass (some randomness added)
- radius (depend on mass)
- force relations between molecules
- position (2D or 3D)
- velocity (2D or 3D)
- force vector (2D or 3D)

Force function is a bit complicated but for infinitelly distance it is 0 at  can be defined in ball.cpp at the top of the file.

To speed up whole simulation multithreading was used (TBB library). Since problem is O(n^2) or O(0.5*n^2) at best code is really slow for more than 1k particles. 
