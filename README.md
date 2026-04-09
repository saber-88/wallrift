# Wallrift

A Wayland wallpaper engine with smooth **parallax / panning effects**.

---
### 🧭 Preview
https://github.com/user-attachments/assets/0987d409-daf4-4777-9a38-25e060707f63

## ✨ Features

- 🖼️ Dynamic wallpaper rendering using OpenGL (GLES2)
- 🎯 Smooth cursor-based parallax effect
- ⚡ Real-time control via CLI


---

## 📦 Architecture

Wallrift consists of two components:

### 1. `wallrift-daemon`
- Runs in background
- Handles rendering (Wayland + EGL + OpenGL)
- Listens for commands via Unix socket

### 2. `wallrift` (CLI)
- Sends commands to daemon
- Used to change wallpaper / adjust settings

---

## Installation

```bash
yay -S wallrift-git
```
---
## 🚀 Usage

### Start daemon

```bash
wallrift-daemon
```

### Set wallpaper

```bash
wallrift img /path/to/image.jpg
```

### Adjust parallax speed

```bash
wallrift speed 0.05
```

### Combined usage

```bash
wallrift img ~/wallpapers/bg.jpg speed 0.03
```

---

## ⚙️ Commands

| Command | Description |
|--------|------------|
| `img <path>` | Set wallpaper image |
| `speed <value>` | Set parallax speed (0.0 – 1.0) |

---

## 🧠 How It Works

- Image is loaded using `stb_image`
- Uploaded as GPU texture
- Fragment shader applies parallax offset
- Cursor movement is interpolated for smooth motion

---

## 🎮 Parallax Behavior

- Cursor position controls horizontal movement
- Smooth interpolation avoids abrupt jumps
- Works best with images slightly larger than screen resolution

---

## 🛠️ Build

### Dependencies

- Wayland
- EGL
- OpenGL ES 2.0
- `stb_image`

### Compile

```bash
make
```

---

## 📁 Socket

Wallrift communicates via a Unix domain socket:

```
/tmp/wallrift.sock
```

---


## 🧪 Tips

- Use wallpapers slightly larger than your screen for best parallax effect


---

## 📌 Roadmap

- [ ] Memory optimization
- [ ] Transitions
- [ ] Multi-monitor support
- [ ] Animated wallpapers
- [ ] Config file support

---

## 👨‍💻 Author

https://github.com/saber-88
---
