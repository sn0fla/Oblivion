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
- [Safe Mode](#-safe-mode)
- [Troubleshooting](#-troubleshooting)
- [FAQ](#-faq)
- [Disclaimer](#-disclaimer)
- [Join the Community](#-join-the-community)

---

## 🧠 About

**Oblivion** is a modern external cheat and overlay for **Counter-Strike 2**. It uses a lightweight DirectComposition + DirectX 11 overlay with a fully custom ImGui menu, advanced memory reading through NT syscalls, and a double-buffered entity snapshot system for smooth, high-performance ESP.

The project is designed for **external use only** — no injection required. It reads game memory from outside the process and renders everything on a transparent topmost overlay.

> 💬 **Download & support:** [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)

---

## ✨ Features

### 🎯 Visuals / ESP
| Feature | Description |
|---------|-------------|
| **Box ESP** | Full box with corner, rounded, fill, and outline styles |
| **Skeleton ESP** | Bone-based skeleton rendering |
| **Healthbar** | Dynamic health bars with custom colors |
| **Nametag** | Player names with outline |
| **Distance** | Distance indicator in meters |
| **Head Circle** | Circle around the head |
| **Snapline** | Lines from screen bottom to player |
| **View Direction** | 45° FOV fan showing where players look |
| **Weapon** | Current weapon name |
| **Flags** | Flashed, Scoped, Defusing, Planting, Bomb, Armor, Reloading, Money, Ping, Ammo |

### 🏃 Movement
| Feature | Description |
|---------|-------------|
| **Bunnyhop** | Auto-jump when on ground |

### 🎨 Customization
- Full color picker for every ESP element
- Separate settings for **Enemy** and **Team**
- Toggle individual flags
- Box style: corner, rounded, fill, outline
- UI scaling and animation

### 🛡️ Safety
- **Safe Mode** — disables all memory writes
- Configurable warning when disabling Safe Mode
- No injection — external overlay only

### 🖥️ Interface
- Custom ImGui menu with animated starfield background
- Draggable, resizable menu
- Config browser with save/load/delete
- Click sounds and smooth animations
- Keybind: `INSERT` to toggle menu

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
- Administrator privileges (for memory access)

### Installation

1. **Join our Discord** → [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)
2. Go to the **#downloads** channel.
3. Download the latest `Oblivion.zip`.
4. Extract the archive to a folder of your choice.
5. Run `Oblivion.exe` as **Administrator**.
6. Launch **Counter-Strike 2**.
7. Press `INSERT` to open the menu.

> ⚠️ **Important:** Always run the cheat before launching CS2 for best results.  
> If CS2 is already running, the cheat will attach automatically.

### Building from Source

1. Clone the repository.
2. Open the solution in **Visual Studio 2022**.
3. Set configuration to **Release | x64**.
4. Make sure the following dependencies are available:
   - DirectX 11 SDK
   - ImGui (included in `External/ImGUI`)
   - Windows SDK 10/11
5. Build the project.
6. The output binary will be in `bin/Release/`.

> 🛠️ **Need help building?** Ask in our Discord — we provide pre-built binaries and support.

---

## 🎮 Usage

| Key | Action |
|-----|--------|
| `INSERT` | Toggle menu |
| `END` | Unload cheat (optional) |
| Mouse | Navigate menu |

**Menu tabs:**
- **Aimbot** — (UI placeholder, backend in progress)
- **Visuals** — Enemy / Team / Local / World ESP settings
- **Settings** — General settings
- **Configs** — Save, load, delete configurations
- **System** — Safe Mode toggle, Unload button

---

## ⚙️ Configuration

Configurations are stored as `.ini` files in:

```
C:\Oblivion\Configurations\
```

You can:
- Create new configs
- Save current settings
- Load existing configs
- Delete unwanted configs
- Open the config folder directly from the UI

The default config file is `config.ini` and is loaded automatically on startup.

---

## 📁 Project Structure

```
Oblivion/
├── Core/
│   ├── Memory/           # NT syscall memory reading, handle hijacking
│   └── Overlay/          # DComp + D3D11 + ImGui overlay
├── SDK/
│   ├── Entities/         # Entity snapshot system
│   ├── Offsets/          # CS2 offsets
│   └── Schemas/          # Schema dumps
├── Features/
│   └── Visuals/
│       └── Player/
│           └── ESP/      # Box, Skeleton, Healthbar, etc.
├── UI/
│   ├── Framework/        # Custom ImGui widgets
│   ├── Header/           # Menu header
│   ├── Cards/            # ESP cards
│   └── Stars/            # Animated background
├── Utilities/
│   ├── Config/           # Config system
│   └── Maths/            # Vector math, WorldToScreen
└── External/
    └── ImGUI/            # ImGui library
```

---

## 🛡️ Safe Mode

**Safe Mode** is enabled by default. When active:
- All memory writes are blocked.
- ESP and overlay still work (read-only).
- Bunnyhop and any write-based features are disabled.

To disable Safe Mode:
1. Open the menu → **System** tab.
2. Toggle **Safemode** off.
3. Read the warning and confirm.

> ⚠️ Disabling Safe Mode allows the cheat to modify game memory. Use at your own risk.

---

## 🧰 Troubleshooting

| Problem | Solution |
|---------|----------|
| Menu doesn't open | Run as Administrator, check `INSERT` key, run CS2 in fullscreen windowed |
| ESP not showing | Make sure CS2 is running and you're in a match, run CS2 in fullscreen windowed |
| Game crashes | Disable Safe Mode only if needed; update offsets |
| Overlay flickers | Disable fullscreen optimizations in CS2 properties |
| Config not saving | Check folder permissions for `C:\Oblivion\Configurations` |

> Still stuck? **Join the Discord** for instant support.

---

## ❓ FAQ

**Q: Is this detected?**  
A: We cannot guarantee safety, but currently it's undetected. Always use at your own risk. Safe Mode reduces risk.

**Q: Does it work on Faceit / ESEA?**  
A: No. This is for official CS2 servers only.

**Q: Do I need to inject?**  
A: No. It's fully external.

**Q: How do I get the cheat?**  
A: Join our Discord → [https://discord.gg/DQXRjR6FhW](https://discord.gg/DQXRjR6FhW)

**Q: Is it free?**  
A: Yes, Oblivion is completely free and open-source.

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
