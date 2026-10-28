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

**Oblivion** is a modern external cheat and overlay for **Counter-Strike 2**. It uses a lightweight DirectComposition + DirectX 11 overlay with a fully custom ImGui menu, advanced memory reading through NT syscalls, handle hijacking, and a double-buffered entity snapshot system for smooth, high-performance ESP.

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
| **Smooth Slider** | 0–100 aim smoothing |
| **Auto-Calibration** | Learns your in-game sensitivity on the fly by comparing sent mouse deltas to observed view-angle changes |

Aim output uses indirect syscalls (`NtUserSendInput` via a generated syscall stub with spoofed return addresses) so mouse input isn't generated from the cheat module itself.

### 🎯 Humanization
A dedicated **Humanization** card sits next to the Aimbot card and can shape the aim path to look less mechanical. All sub-features are gated behind a single master **Enable** toggle:

| Feature | Description |
|---------|-------------|
| **Randomize Smooth** | Re-rolls the smoothing value every tick between a configurable Min/Max range |
| **Microjitter** | Adds per-tick random pixel offset (±amount) to the outgoing mouse delta |
| **Overshoot** | Multiplies the aim step by a random 1.0–1.5× factor so the crosshair slightly leads the target |
| **Undershoot** | Multiplies the aim step by a random 0.5–1.0× factor so the crosshair slightly lags the target |
| **Aim Curve** | Rotates the aim delta by a random ±per-burst angle to produce a curved, arc-like motion instead of a straight line |

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
- External read-only snapshot system (`ReadRaw` batch reads, coalesced multi-field reads per entity)
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
- **Aimbot** → Aimbot (functional), Humanization (functional), Triggerbot, Recoil *(UI placeholders)*
- **Visuals** → Enemy, Team (functional), Local, World *(UI placeholders)*
- **Settings** *(placeholder)*
- **Configs** → Full config browser with actions panel
- **System** → Safemode, VSync, Allow Tearing, Fullscreen Mode, Streamproof, Unload

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

**Menu tabs:**
- **Aimbot** — Aimbot, Humanization, Triggerbot *(placeholder)*, Recoil *(placeholder)*
- **Visuals** — Enemy / Team / Local *(placeholder)* / World *(placeholder)* ESP settings
- **Settings** — *(placeholder)*
- **Configs** — Create, save, load, delete, refresh, and browse `.ini` configs
- **System** — Safemode toggle, VSync, Allow Tearing, Fullscreen Mode, Streamproof, Unload

> ⚠️ There is **no separate END hotkey** in the current build — the cheat is unloaded via the **System → Unload** button (with confirmation popup).

---

## ⚙️ Configuration

Configurations are stored as `.ini` files in:

```
C:\Oblivion\Configurations\
```

You can:
- Create new configs
- Save current settings to the selected config (with overwrite confirmation)
- Load an existing config (with confirmation)
- Delete a config (with confirmation)
- Refresh the file list
- Open the config folder directly from the UI
- Search configs by name

The **Aimbot** section also stores:
- Mode (Toggle / Hold / Always)
- Key selection via key-capture button
- FOV, Hitbox mask, Smooth value
- FOV visualize toggle + color
- Visible check
- Humanization master + all sub-feature toggles and range sliders

> ℹ️ Config files store all ESP elements, colors, flags, offscreen settings, display settings (VSync / tearing / fullscreen), safe-mode preference, safe-mode warning suppression, and streamproof state.

---

## 📁 Project Structure

```
Oblivion/
├── Core/
│   ├── Memory/           # NT syscall stubs, handle hijacking, PEB module walk
│   └── Overlay/          # DComp + D3D11 + ImGui overlay, swapchain handling,
│                         # UIAccess elevation for fullscreen mode
├── SDK/
│   ├── Entities/         # Cached controller/pawn scan + double-buffered frame snapshot
│   ├── Offsets/          # CS2 offsets
│   └── Schemas/          # Schema dumps (client_dll, etc.)
├── Features/
│   ├── Combat/
│   │   └── Aimbot/       # Mouse-delta aimbot via indirect NtUserSendInput,
│   │                     # auto-calibration, humanization
│   └── Visuals/
│       └── Player/
│           └── ESP/      # Box, Skeleton, Healthbar, Nametag, Distance,
│                         # HeadCircle, Snapline, ViewDirection, Weapon,
│                         # Flags, Offscreen
├── UI/
│   ├── Framework/        # Shared ImGui helpers, icon font, click sounds,
│   │                     # flow-line pulse drawing
│   ├── Header/           # Animated menu header (flow-line pulse, icon tabs,
│   │                     # animated sub-tab strip)
│   ├── Cards/            # Aimbot / Humanize / ESP / Config / System cards
│   ├── Widgets/          # ESP card layout, dashed sliders, checkbox drawing
│   ├── Animations/       # Boot animation (blur-in, text glow, fade-out)
│   └── Stars/            # Multi-layer animated starfield background
├── Utilities/
│   ├── Config/           # INI config system + config browser state
│   ├── Logging/          # Log rotation, crash handler (minidump + stack walk),
│   │                     # latency sample buckets
│   └── Maths/            # Vector math, WorldToScreen, angle calculations
│       └── Vectors/      # Vector2, Vector3, ViewMatrix
└── External/
    ├── ImGUI/            # ImGui library
    └── Xorstr/           # Compile-time string obfuscation
```

---

## 🛡️ Safe Mode & Streamproof

### Safe Mode

**Safe Mode** is enabled by default. When active:
- All memory writes are blocked (via `g_AllowMemoryWrite`)
- ESP, overlay, and aimbot still function (aimbot uses mouse input, not memory writes)
- Any future write-based features will be disabled

To disable Safe Mode:
1. Open the menu → **System** tab.
2. Toggle **Safemode** off.
3. Read the warning and confirm — or check **"Do not remind me again"** to disable the warning permanently (this preference is stored in the config).

> ⚠️ Disabling Safe Mode allows the cheat to modify game memory. Use at your own risk.

### Streamproof

The **Streamproof** toggle (System tab) sets the overlay window's display affinity to `WDA_EXCLUDEFROMCAPTURE`, which hides it from OBS, Discord, Xbox Game Bar, and most screen capture APIs. If the OS rejects that flag it falls back to `WDA_MONITOR` (window appears as a black rectangle in captures).

### Fullscreen Mode / UIAccess

The **Fullscreen Mode** toggle attempts to elevate the cheat to the `UIAccess` integrity level by duplicating the token of `osk.exe` (On-Screen Keyboard). This allows the overlay to stay topmost over exclusive-fullscreen games without stealing focus. The current process relaunches itself with a `--ui-active` flag once the new token is acquired. If elevation fails, the cheat continues running at its current integrity level.

---

## 🧰 Troubleshooting

| Problem | Solution |
|---------|----------|
| Menu doesn't open | Run as Administrator, check `INSERT` key, run CS2 in fullscreen windowed |
| ESP not showing | Make sure CS2 is running and you're in a match, run CS2 in fullscreen windowed |
| Game crashes | Check `C:\Oblivion\Logs\` for the latest `.log` and `.dmp` — attach to a Discord report |
| Overlay flickers | Disable fullscreen optimizations in CS2 properties, or toggle VSync / Allow Tearing |
| Config not saving | Check folder permissions for `C:\Oblivion\Configurations` |
| Aimbot overshoots / undershoots | Adjust Smooth value, or let auto-calibration re-learn by moving the mouse a few times; tune Humanization ranges |
| Menu doesn't respond to input | Make sure overlay is interactive (it is while the menu is open) |
| Overlay disappears in OBS | Enable **Streamproof** in the System tab |
| Overlay won't stay on top of fullscreen game | Enable **Fullscreen Mode** in the System tab (requires Admin) |
| Cheat closes on its own | Look for `cs2.exe exited` in the log — the cheat monitors the game process and shuts down when it closes |

> Still stuck? **Join the Discord** for instant support.

---

## ❓ FAQ

**Q: Is this detected?**
A: We cannot guarantee safety, but currently it's undetected. Always use at your own risk. Safe Mode reduces risk.

**Q: Does it work on Faceit / ESEA?**
A: No. This is for official CS2 servers only.

**Q: Do I need to inject?**
A: No. It's fully external — a transparent DirectComposition overlay + external memory reads.

**Q: How do I get the cheat?**
A: Join our Discord → [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)

**Q: Is it free?**
A: Yes, Oblivion is completely free.

**Q: Is bunnyhop included?**
A: No. The current build focuses on ESP, aimbot, and humanization.

**Q: Why are some tabs empty?**
A: Triggerbot, Recoil, Local ESP, World ESP, and the Settings tab are UI placeholders reserved for future features.

**Q: What's the difference between Safe Mode and Streamproof?**
A: **Safe Mode** blocks memory writes. **Streamproof** hides the overlay from screen captures. They are independent and can be combined.

**Q: What does Fullscreen Mode do?**
A: It relaunches the cheat with a duplicated `UIAccess` token from `osk.exe` so the overlay can render on top of exclusive-fullscreen games without stealing focus.

**Q: What is Humanization for?**
A: It makes the aimbot's mouse movement look less robotic by randomizing smoothing, adding jitter, overshooting/undershooting, and curving the aim path. Each component has its own toggle and range sliders, plus a master enable.

**Q: How does auto-calibration work?**
A: Every time the aimbot sends a mouse delta, the cheat records the view angles before the send. On the next tick it compares them to the new angles to learn the current degrees-per-pixel ratio. This adapts automatically if you change your in-game sensitivity.

---

## ⚠️ Disclaimer

This project is for **educational purposes only**.
The developers are **not responsible** for any bans, account suspensions, or damages caused by using this software.
Cheating in online games violates the Terms of Service of Valve and Steam.
**Use at your own risk.**

---

## 💬 Join the Community

<p align="center">
  <a href="https://discord.gg/DQXRjR6FhW">
    <img src="https://img.shields.io/badge/Discord-Join%20Now-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord">
  </a>
</p>

**Why join?**
- ✅ Download the latest cheat build
- ✅ Get help with setup and troubleshooting
- ✅ Request features and report bugs
- ✅ Stay updated with new releases
- ✅ Connect with other users

> 🔗 **Discord Invite:** [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)

---

<p align="center">
  <strong>Oblivion</strong> — Elevate your CS2 experience.
</p>

<p align="center">
  Made with ❤️ by the Oblivion Team
</p>
