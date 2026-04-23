# Wallrift

A lightweight Wayland wallpaper engine featuring smooth, GPU-accelerated **parallax / panning effects**.

---

## 🧭 Preview
https://github.com/user-attachments/assets/0987d409-daf4-4777-9a38-25e060707f63

---

## ✨ Features

- 🖼️ Real-time wallpaper rendering powered by OpenGL (GLES2)
- 🎯 Smooth, cursor-driven parallax effect
- ⚡ Fast and minimal CLI-based control
- 🧩 Clean daemon + client architecture

---

## 📦 Architecture

Wallrift is composed of two main components:

### 1. `wallrift-daemon`
- Runs as a background service  
- Handles rendering via Wayland, EGL, and OpenGL  
- Listens for commands over a Unix domain socket  

### 2. `wallrift` (CLI)
- Sends commands to the daemon  
- Used to set wallpapers and adjust parameters  

---

## 📥 Installation

### AUR (Recommended)

```bash
yay -S wallrift
```

### Manual Build

```bash
git clone https://github.com/saber-88/wallrift
cd wallrift
mkdir build && cd build
cmake ..
make
```

> ⚠️ **Note**  
> Shader files must be available at:  
> `/usr/share/wallrift/shaders/`  
>  
> Copy the `shaders/` directory manually after building.

---

## 🚀 Usage

### Start the daemon

```bash
wallrift-daemon
```

### Set a wallpaper

```bash
wallrift img /path/to/image.jpg
```

### Adjust parallax speed

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

- Images are loaded using `stb_image`  
- Uploaded to the GPU as textures  
- Cursor movement is interpolated to produce smooth parallax motion  

---

## 🎮 Parallax Behavior

- Horizontal movement is driven by cursor position  
- Smooth interpolation eliminates abrupt transitions  
- Best suited for wallpapers wider than screen resolution  
- Lower speed values provide more fluid motion  

---

## 🛠️ Build Dependencies

- Wayland  
- EGL  
- OpenGL ES 2.0  
- stb_image  

---

## 📁 Communication

Wallrift uses a Unix domain socket for IPC:

```
/tmp/wallrift.sock
```

---

## 🧪 Tips

- Use wide (ultrawide-style) wallpapers for best results  
- Wallpaper collection: https://github.com/saber-88/walls  
- Recommended companion tool: https://github.com/saber-88/swifty  

---

## 📌 Roadmap

- [ ] Memory optimization  
- [ ] Smooth transitions  
- [x] Multi-monitor support  
- [ ] Animated wallpapers  
- [ ] Config file support  

---

## 👨‍💻 Author

https://github.com/saber-88

---
