# DESYNC 

![C++](https://img.shields.io/badge/C++-17-blue.svg) ![SDL3](https://img.shields.io/badge/SDL3-Latest-red.svg) ![Size](https://img.shields.io/badge/Size-1.63_MB-brightgreen.svg)

**DESYNC** is a micro-sized (only 1.63 MB!) pseudo-3D action/survival game built entirely from scratch using C++ and SDL3. Players must survive incoming waves of enemies while keeping their system's "Sync" level under control. As the system synchronization breaks down, the game's audio and visual atmosphere dynamically glitch and degrade.

## 📥 Download and Play

To jump right into the action without compiling the source code:
1. Go to the **[Releases](https://github.com/burakenesportakal/DESYNC/releases/tag/Game)** tab on the right side of this repository.
2. Download the `DESYNC.rar` file from the latest release.
3. Extract the archive and double-click `SelfDistruction.exe` to start playing! *(No installation required).*

---

## 🚀 Features

* **Custom Pseudo-3D Engine:** A 3D illusion created entirely from 2D rendering using trigonometry, raycasting logic, and depth-sorting (z-index).
* **Dynamic Audio Glitch System:** As the "Sync" level reaches critical points, the background audio realistically stutters and corrupts in real-time.
* **Game Feel & "Juice":** Includes hit-stop, screen shake, additive blending for damage flashes, and procedural sword animations.
* **Resolution Independence:** Utilizes SDL3's Logical Presentation to flawlessly scale the game to 1080p full-screen (Letterbox) on any monitor without distortion.
* **Object Pooling:** A highly optimized, memory-friendly enemy spawning system that avoids constant memory allocation.

---

## 🎮 How to Play

**Your Goal:** Do not let your physical health (HP) drop to zero, AND prevent the System Synchronization (SYNC) from reaching 100%!

* `W, A, S, D` - Move
* `Mouse` - Look around (FPS Camera)
* `Left Click` - Swing Sword (Attack)
* `E` - Pick up Medkit
* `P` - Pause Game
* `ESC` - Quit / Emergency Exit

**Enemy Types:**
1. **Invader:** Approaches you directly and deals physical damage (HP).
2. **Syncer:** Rapidly increases the system's `Sync` level just by existing in the world. They must be your primary targets!

---

## 🛠️ Build Instructions (For Developers)

If you wish to compile the source code yourself:
1. Open the project in Visual Studio (x64).
2. Ensure the **SDL3** library is correctly linked to your `Include` and `Lib` directories.
3. The project uses `stb_image.h` for texture loading (already included in the repository).
4. Build the solution in `Release` mode.

*Developed by Burak*
