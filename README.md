# 🕶️ Oblivion — CS2 External Cheat & Overlay

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows">
  <img src="https://img.shields.io/badge/Language-C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Graphics-DirectX%2011-107C10?style=for-the-badge&logo=directx&logoColor=white" alt="DirectX 11">
  <img src="https://img.shields.io/badge/UI-ImGui-FF6F00?style=for-the-badge" alt="ImGui">
  <img src="https://img.shields.io/badge/Status-Active-brightgreen?style=for-the-badge" alt="Active">
</p>

<p align="center">
  <a href="https://discord.gg/DQXRjR6FhW">
    <img src="https://img.shields.io/badge/Discord-Join%20to%20get%20the%20cheat-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord">
  </a>
</p>

> 🚀 **Get the latest build, updates, and support in our Discord:**
> **👉 [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)**
> Join now to download the cheat and unlock all features!

---

## 📖 Table of Contents

- [About](#-about)
- [Features](#-features)
- [Screenshots](#-screenshots)
- [Getting Started](#-getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
  - [Building from Source](#building-from-source)
- [Usage](#-usage)
- [Configuration](#-configuration)
- [Project Structure](#-project-structure)
- [Safe Mode & Streamproof](#-safe-mode--streamproof)
- [Troubleshooting](#-troubleshooting)
- [FAQ](#-faq)
- [Disclaimer](#-disclaimer)
- [Join the Community](#-join-the-community)

---

## 🧠 About

**Oblivion** is a modern external cheat and overlay for **Counter-Strike 2**. It uses a lightweight DirectComposition + DirectX 11 overlay with a fully custom ImGui menu, advanced memory reading through indirect NT syscalls, handle hijacking, and a **cached entity snapshot system** with per-player slot reuse for smooth, high-performance ESP. <!-- UPDATED: removed "double-buffered" claim; the code uses a single-buffer build with a persistent slot cache -->

The project is designed for **external use only** — no injection required. It reads game memory from outside the process and renders everything on a transparent, topmost overlay.

> 💬 **Download & support:** [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)

---

## ✨ Features

### 🎯 Aimbot
| Feature | Description |
|---------|-------------|
| **Enable Toggle** | Master switch for the aimbot |
| **Mode** | Toggle / Hold / Always |
| **Keybind** | Configurable activation key with click-capture UI (default: `M1` in Hold mode) |
| **FOV Slider** | 0–180° aiming field of view |
| **Show FOV** | Circular/elliptical on-screen FOV indicator derived from the view matrix, with full RGBA color picker |
| **Visible Check** | Only target players the local player can actually see (`m_entitySpottedState`) |
| **Hitbox Selection** | Independently toggleable: Head, Neck, Chest, Stomach, Arms, Legs, Feet |
| **Hitbox Priority** | Closest / Head Priority / Body Priority |
| **Weapon Groups** | Per-weapon aim profiles (33 weapon presets, up to 32 custom groups) |
| **Smooth Slider** | 0–100 aim smoothing with min-jerk velocity curve and burst-mode velocity floor |
| **Auto-Calibration** | Learns your in-game sensitivity on the fly by comparing sent mouse deltas to observed view-angle changes |

Aim output uses indirect syscalls (`NtUserSendInput` via a generated syscall stub with spoofed return addresses) so mouse input isn't generated from the cheat module itself.

### 🎯 Humanization
A dedicated **Humanization** card sits next to the Aimbot card and can shape the aim path to look less mechanical. All sub-features are gated behind a single master **Enable** toggle:

<!-- UPDATED: added Reaction Delay row; corrected overshoot/undershoot/curve descriptions to match code behavior -->

| Feature | Description |
|---------|-------------|
| **Randomize Smooth** | Re-rolls the smoothing value every tick between a configurable Min/Max range |
| **Microjitter** | Adds per-tick pseudo-noise pixel offset (`ValueNoise` on top of a random seed) to the outgoing mouse delta |
| **Overshoot** | Adds a random percentage of the current aim magnitude (Min–Max %) as a forward offset, so the crosshair slightly leads the target |
| **Undershoot** | Subtracts a random percentage of the current aim magnitude (Min–Max %) so the crosshair slightly lags the target |
| **Aim Curve** | Rotates the aim delta by a per-burst angle (up to ~28°) — the direction is rolled once per burst — producing a curved, arc-like motion instead of a straight line |
| **Reaction Delay** | Adds a random per-target delay (Min–Max ms) before the aimbot engages a newly acquired target |

### 🎯 Visuals / ESP
| Feature | Description |
|---------|-------------|
| **Box ESP** | Full box with corner, rounded, fill, and outline styles (outline pass drawn underneath the main stroke) |
| **Skeleton ESP** | Bone-based skeleton with per-limb group toggles (Head, Torso, Left/Right Arm, Left/Right Leg, Feet) and optional joint dots |
| **Healthbar** | Dynamic health bars with custom colors |
| **Nametag** | Player names with 4-direction shadow outline |
| **Distance** | Distance indicator in meters |
| **Head Circle** | Circle around the head, scaled by distance |
| **Snapline** | Lines from screen bottom to player |
| **View Direction** | 45° FOV fan showing where players look |
| **Weapon** | Current weapon name (resolved from item definition index) |
| **Flags** | Flashed, Scoped, Defusing, Planting, Bomb, Armor/Helmet, Reloading, Money, Ping, Ammo |
| **Offscreen Indicators** | Directional arrows for players outside your view, with adjustable radius and size sliders |

Every element has its **own color picker** and separate settings for **Enemy** and **Team**. The ESP card scrolls independently and uses animated expanders for the Box / Skeleton / Flags / Offscreen sub-sections.

### 🎨 Customization
- Full color picker for every ESP element (both Enemy and Team variants)
- Per-flag toggle and per-flag color
- Box style: corner, rounded, fill, outline
- Skeleton joint groups (Head / Torso / Left Arm / Right Arm / Left Leg / Right Leg / Feet) with optional joint dots
- Offscreen marker distance and size sliders
- Animated UI scaling, menu fade-in, and multi-layer starfield background
- Boot animation with a glowing title, blur-in and fade-out stages
- Click sounds and hover animations throughout

### 🛡️ Safety
- **Safe Mode** — blocks all memory writes, disables write-based features
- Configurable warning when disabling Safe Mode ("Do not remind me again" checkbox)
- No injection — external overlay only
- Handle hijacking from trusted processes (Steam, explorer, dwm, etc.) instead of direct `OpenProcess`
- Indirect NT syscalls with spoofed return addresses for memory reads/writes
- Batched `ReadRaw` reads with a per-player slot cache to minimize syscall churn
- **Streamproof mode** — uses `WDA_EXCLUDEFROMCAPTURE` (with `WDA_MONITOR` fallback) so the overlay is invisible in OBS / captures
- String obfuscation via compile-time Xorstr

### 🖥️ Interface
- Custom ImGui menu with an animated multi-layer starfield background (5 depth layers, twinkle + halo + sparkle)
- Draggable, resizable menu with smooth intro animation
- **Flow-line pulse** header that travels across the title bar and syncs with the title text glow
- Boot animation with glowing "Oblivion" title
- Config browser with search, save/load/delete/create/refresh/folder-open
- Click sounds and hover animations throughout
- Keybind: `INSERT` to toggle menu

### 🧩 Menu Layout
<!-- UPDATED: System and Configs tab order corrected to match GUI.cpp (tab 3 = System, tab 4 = Configs) -->

- **Aimbot** → Aimbot (functional), Humanization (functional), Triggerbot *(placeholder)*, Recoil *(placeholder)*
- **Visuals** → Enemy, Team (functional), Local *(placeholder)*, World *(placeholder)*
- **Settings** *(placeholder)*
- **System** → Safemode, VSync, Allow Tearing, Fullscreen Mode, Streamproof, Unload
- **Configs** → Full config browser with actions panel

> ℹ️ Tabs marked *placeholder* are visible in the UI but have no backend logic yet.

---

## 📸 Screenshots

> *Screenshots coming soon. Join the Discord to see live previews!*

<p align="center">
  <img src="https://via.placeholder.com/800x450/1a1a24/ffffff?text=Oblivion+Menu" alt="Menu Preview" width="700">
</p>

---

## 🚀 Getting Started

### Prerequisites

- **Windows 10 / 11** (64-bit)
- **Counter-Strike 2** installed via Steam
- **Visual Studio 2022** with C++20 (for building)
- **DirectX 11** runtime
- Administrator privileges (for memory access and handle hijacking)

### Installation

1. **Join our Discord** → [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)
2. Go to the **#downloads** channel.
3. Download the latest `Oblivion.zip`.
4. Extract the archive to a folder of your choice.
5. Run `Oblivion.exe` as **Administrator**.
6. Launch **Counter-Strike 2**.
7. Press `INSERT` to open the menu.

> ⚠️ **Important:** Always run the cheat before launching CS2 for best results.
> If CS2 is already running, the cheat will attach automatically (and will auto-launch CS2 via `steam://rungameid/730` if it isn't found, waiting up to 5 minutes).

### Building from Source

1. Clone the repository.
2. Open the solution in **Visual Studio 2022**.
3. Set configuration to **Release | x64**.
4. Make sure the following dependencies are available:
   - DirectX 11 SDK (`d3d11.lib`, `dxgi.lib`, `dcomp.lib`)
   - DirectComposition
   - ImGui (included in `External/ImGUI`)
   - Xorstr (included in `External/Xorstr`)
   - Windows SDK 10/11
   - `dbghelp.lib`, `ntdll.lib`, `winmm.lib`, `shell32.lib`, `ole32.lib`, `avrt.lib`, `dwmapi.lib`
5. Build the project.
6. The output binary will be in `bin/Release/`.

> 🛠️ **Need help building?** Ask in our Discord — we provide pre-built binaries and support.

---

## 🎮 Usage

| Key | Action |
|-----|--------|
| `INSERT` | Toggle menu (interactive / non-interactive) |
| Mouse | Navigate menu / drag by the header |

**Menu tabs** (left → right): <!-- UPDATED: System now listed before Configs to match actual tab indices -->

1. **Aimbot** — Aimbot, Humanization, Triggerbot *(placeholder)*, Recoil *(placeholder)*
2. **Visuals** — Enemy, Team, Local *(placeholder)*, World *(placeholder)*
3. **Settings** — *(placeholder)*
4. **System** — Safemode, VSync, Allow Tearing, Fullscreen Mode, Streamproof, Unload
5. **Configs** — Create, save, load, delete, refresh, and browse `.ini` configs

> ⚠️ There is **no separate END hotkey** in the current build — the cheat is unloaded via the **System → Unload** button (with confirmation popup).

---

## ⚙️ Configuration

Configurations are stored as `.ini` files in:
