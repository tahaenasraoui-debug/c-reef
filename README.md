# 🐠 c-reef

![Language](https://img.shields.io/badge/Language-C99-00599C?style=flat-square&logo=c&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20POSIX-FCC624?style=flat-square&logo=linux&logoColor=black)
![Dependencies](https://img.shields.io/badge/Dependencies-Zero%20(libc%20%2B%20libm)-success?style=flat-square)
![FPS](https://img.shields.io/badge/Framerate-30%20FPS-00C7B7?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)

**`c-reef`** is a zero-dependency, 30 FPS Linux terminal ASCII aquarium written in pure **C99**. It features a flicker-free double-buffered ANSI rendering engine, 12 marine species (10 fish + 2 benthic invertebrates) with custom autonomous physics, swaying sinusoidal kelp forests, rising air bubbles, and a built-in **procedural underwater acoustics synthesizer** (100% natural, instrument-free ocean surf and bubble physics).

---

## ✨ Visual Preview

```text
                                 /|
   ><>  ><>                 ____/ |_____
     ><>                   /  o         \______===>          .
                          |   \____  ___/                   o
          _.-""\           \_______|_/                       .
        .' |   |`>==<                                       (
       < (o| | | )==<                      / \               )
        `'-|___|/                         /   \             {
                                         | o o |             }
  {              _.-""""-._              / / | \ \          (     (\       /)
   }            < (o)(o)   >==<         ( ( ( ) ) )          )     \.-'""'-./
  (              `'-....-'`                                 {     < ( o  o ) >
.__}_~._O_~._.~_._~.___o_~._@_.~_._~.__._~_O_.__~._._~_o_.__.}__~_._/'-.__.-'\_
~._~_o_._.~_@._~_.__.~_._~_O_._~_.__~_._~_._o_~_.__._~_._@_.~_._~_./ /  ||  \ \
```

---

## 🚀 Key Features

* **Zero External Dependencies:** Built strictly with standard C99, POSIX system calls (`ioctl`, `sigaction`, `nanosleep`, `write`), and `<math.h>`. No `ncurses` required.
* **Zero-Flicker Alternate Screen Rendering:** Maintains separate 2D character and ANSI color matrices in memory. Renders inside the terminal's alternate screen buffer (`\x1b[?1049h`) with explicit per-row coordinate addressing (`\x1b[row;1H`) and single-pass `write()` calls—eliminating flicker and line-wrap scrolling.
* **True Sprite Transparency & Edge Clipping:** Internal whitespace (`' '`) inside multi-line sprite bounding boxes never overwrites background kelp, rising bubbles, or overlapping fish. Sprites enter and exit terminal borders seamlessly.
* **Dynamic `SIGWINCH` Resizing:** Queries `ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` to dynamically reallocate buffers and regenerate the seabed in real time when you resize or tile your terminal window.
* **Procedural Halal Ocean Ambience:** Forks a lightweight background PCM synthesizer piped to `aplay` that mathematically generates low-pass filtered brown noise (deep ocean swells) and natural Minnaert bubble resonance—100% free of musical instruments or external audio files.

---

## 🐟 Marine Roster & Autonomous Behaviors

| # | Species | Height | ANSI Color | Behavior & Zone |
| :- | :--- | :---: | :--- | :--- |
| **1** | **Schooling Minnow / Neon Tetra** | 1 line | Electric Blue (`\x1b[94m`) | Spawns and zips rapidly in tight synchronized packs of 3–5. |
| **2** | **Guppy / Fancy Tail** | 2 lines | Neon Magenta (`\x1b[95m`) | Frantic, high-frequency sinusoidal vertical flutter. |
| **3** | **Spiky Round Pufferfish** | 3 lines | Bright Yellow (`\x1b[93m`) | Slow, buoyant drift across mid-water. |
| **4** | **Classic Angelfish** | 4 lines | Coral Gold (`\x1b[33m`) | Elegant, steady mid-water cruiser. |
| **5** | **True Striped Clownfish** | 4 lines | Vivid Orange (`208`) + White (`97m`) | Multi-colored sprite rendering with distinct white vertical bar bands. |
| **6** | **Moray Eel / Ribbon Fish** | 2 lines | Olive Green (`\x1b[32m`) | Undulates per-line with a sinusoidal wave along the lower third of the tank. |
| **7** | **Swordfish / Marlin** | 4 lines | Deep Ocean Blue (`\x1b[34m`) | High-velocity pelagic swimmer with a long piercing bill. |
| **8** | **Flatfish / Flounder** | 3 lines | Mottled Gray (`\x1b[90m`) | Skims right above the sand and boulders along the seabed. |
| **9** | **Sleek Hunter / Pelagic Tuna** | 5 lines | Silver Gray (`\x1b[37m`) | Heavy, fast-moving open-water predator. |
| **10** | **Great Apex Shark** | 6 lines | Dark Slate (`\x1b[90m`) | Massive, rare slow patrol through the deep/middle water column. |
| **A** | **Deep-Water Squid** | 7 lines | Violet / Purple (`\x1b[35m`) | Propels diagonally upward in bursts before coasting gently downward. |
| **B** | **Seabed Crab** | 5 lines | Crimson (`\x1b[91m`) | Scuttles along the bottom sand row while snapping its pincers (2-frame animation). |

---

## 🛠️ Compilation & Usage

### Prerequisites
* A Linux / POSIX operating system with a UTF-8 terminal emulator (Konsole, Alacritty, Kitty, GNOME Terminal, etc.).
* `gcc` (or `clang`) and `make`
* `alsa-utils` (`aplay`) for procedural underwater audio (runs silently if `aplay` is absent).

### Clone & Build
```bash
git clone [https://github.com/tahaenasraoui-debug/c-reef.git](https://github.com/tahaenasraoui-debug/c-reef.git)
cd c-reef
make
```
*(Or compile directly with `gcc -std=c99 -O2 -Wall -Wextra aquarium.c -o c-reef -lm`)*

### Run
```bash
./c-reef
```

### Controls
* **Resize Window:** Freely resize or tile your terminal; `c-reef` adapts automatically via `SIGWINCH`.
* **`Ctrl + C`:** Cleanly terminates the simulation, stops the background audio child process, restores the terminal cursor (`\x1b[?25h`), exits the alternate screen buffer (`\x1b[?1049l`), and resets ANSI styling (`\x1b[0m`).

---

## 📄 License

Distributed under the [MIT License](LICENSE).
