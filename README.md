# Legion Laptop Toolkit CLI (LLTC)

A lightweight, command-line-only tool to control a couple of features that are only available in Lenovo Vantage or Legion Zone with **no background service**.

> ℹ️ **Note**: This project is **actively maintained** and was inspired by the now-archived [LenovoLegionToolkit (LLT)](https://github.com/BartoszCichecki/LenovoLegionToolkit).
> Special thanks to [@BartoszCichecki](https://github.com/BartoszCichecki) and the LLT community for their foundational work on Lenovo laptop control interfaces.

---

## 🛠 Features

- Get or set battery charging mode
- Get or set keyboard white backlight
- Get or set screen OverDrive status
- Get detailed battery information (with dynamic monitoring mode)
- Instantly turn off the display

---

## 🖥️ Usage

```bash
# Show help
lltc

# Get current battery mode
lltc get batterymode   # or: lltc get bm

# Set battery mode
lltc set batterymode Conservation   # or: lltc set bm 1
lltc set batterymode Normal         # or: lltc set bm 2
lltc set batterymode RapidCharge    # or: lltc set bm 3

# Get current keyboard backlight state
lltc get keyboardbacklight   # or: lltc get kb

# Set keyboard backlight
lltc set keyboardbacklight Off     # or: lltc set kb 0
lltc set keyboardbacklight Low     # or: lltc set kb 1
lltc set keyboardbacklight High    # or: lltc set kb 2

# Get OverDrive status
lltc get overdrive     # or: lltc get od

# Set OverDrive
lltc set overdrive on  # or: lltc set od 1
lltc set overdrive off # or: lltc set od 0

# Get detailed battery information
lltc get batteryinformation             # or: lltc get bi
lltc get batteryinformation -dmon       # monitoring mode (refresh rate 1000ms by default)
lltc get batteryinformation -dmon 500   # or: lltc get bi -dmon 500

# Turn off display
lltc monitoroff        # or: lltc mo
```

---

## ⚠️ Requirements & Compatibility

- **Windows 10 or 11 (64-bit)**
- **Lenovo Legion** laptop
- **Lenovo Energy Management Driver** installed

If the `\\.\EnergyDrv` device is not present, battery-related commands will fail. Ensure Lenovo system software is properly installed.

---

## 📦 Building from Source

This project is developed with **VS Code + GCC (MinGW-w64)** and requires no IDE.

### Prerequisites
- [MinGW-w64](https://www.mingw-w64.org/)
- `g++` in your `PATH`

### Option 1: Build via Terminal
Open a terminal in the project root and run:

```bash
g++ -O2 -s -static -Wall -Wextra -o lltc.exe lltc.cpp -lole32 -loleaut32 -lwbemuuid -luuid -lversion -lsetupapi
```

### Option 2: Build via VS Code (for development)
1. Open the project folder in VS Code.
2. Create `.vscode/tasks.json` with the following content:

```json
{
    "tasks": [
        {
            "type": "cppbuild",
            "label": "Build lltc.exe (Release)",
            "command": "g++",
            "args": [
                "-fdiagnostics-color=always",
                "-O2",
                "-s",
                "-static",
                "-Wall",
                "-Wextra",
                "-o",
                "${workspaceFolder}/lltc.exe",
                "${workspaceFolder}/lltc.cpp",
                "-lole32",
                "-loleaut32",
                "-lwbemuuid",
                "-luuid",
                "-lversion",
                "-lsetupapi"
            ],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "problemMatcher": ["$gcc"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "detail": "Standalone release build with MinGW-w64"
        }
    ],
    "version": "2.0.0"
}
```

3. Press `Ctrl+Shift+B` to build.


---

## 📜 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

<details>
<summary>✨ Inspired by LenovoLegionToolkit</summary>

This project was made possible thanks to the pioneering reverse-engineering work done by the community around [LenovoLegionToolkit](https://github.com/BartoszCichecki/LenovoLegionToolkit).  
While LLTC shares a similar goal — lightweight control of Legion laptop features — it does not incorporate any code from LenovoLegionToolkit (which is licensed under GPLv3), so this tool remains under the permissive MIT License.
</details>

---

## 🧩 Next Tasks
- [ ] **Implement power mode switching (Quiet/Balance/Performance)**
- [ ] Implement port backlight control
- [ ] Implement Always-on-USB control
- [ ] Implement Instant-Boot control
- [ ] Implement Flip-to-Start control

---

## 💬 Final Note

Contributions or issues are welcome!

Happy coding!