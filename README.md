A minimalist Conway's Game of Life lockscreen for Hyprlock.

It has Hyprlock run a CPP script at a certain interval and pipes its console output to the screen.

# Installation

Note that if Hyprlock is already set up, this will overwrite any existing configuration.

```
git clone https://www.github.com/Mythilllian/Conway-Hyprlock.git
cd Conway-Hyprlock
./setup.sh
```

# Configuration

After setup, `conway.cpp`, `conway` (its binary), and `config.json` are stored in `$USER_HOME/.config/hypr/conway_lockscreen`, and `hyprlock.conf` is stored in `$USER_HOME/.config/hypr`. `$USER_HOME` is either the user's profile or the root profile if `setup.sh` was run with `sudo`.

These files can all be modified there, though `conway.cpp` would need to be recompiled if modified.

To modify the update interval, see the `label:text` attribute in `hyprlock.conf`.

`config.json` provides access to several parts of the program. `density` measures the percent change a tile is spawned at a certain position when the game is initialized. `entropy` measures the chance out of 1000 that a tile is randomly swapaped. It is used to ensure that the simulation never stabilizes or goes exinct. Generally, increasing its value introduces more randomness. `columns` and `rows` are used to size the simulation. Adjust it as necessary to fit your screen. `exclude` and `exclude_enabled` are used to cull a part of the simulation from being visible on the screen. In the `hyprland.conf` provided, it gives enough room for a clock and the password box. `hide_lonely_entropy_tiles` is used so that the randomly flipped entropy tiles aren't flashing constantly, and ensures that in order for a new entropy tile to be made visible, it must have neighbors that are not also new entropy tiles.
