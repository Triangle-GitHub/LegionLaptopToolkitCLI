# Legion Laptop Toolkit CLI (LLTC)

A lightweight, command-line-only tool to control a couple of features that are only available in Lenovo Vantage or Legion Zone with **no background service**.

> ℹ️ **Note**: This project is **actively maintained** and was inspired by the now-archived [LenovoLegionToolkit (LLT)](https://github.com/BartoszCichecki/LenovoLegionToolkit).
> Special thanks to [@BartoszCichecki](https://github.com/BartoszCichecki) and the LLT community for their foundational work on Lenovo laptop control interfaces.

---

## 🛠 Features

- Get or set power mode (as well as power button color) 🔑
- Get or set Hybrid GPU working mode 🔑
- Get or set battery charging mode
- Get or set keyboard white backlight
- Get or set screen OverDrive status 🔑
- Get or set Always on USB status
- Get detailed battery information (with dynamic monitoring mode)
- Instantly turn off the display

> 🔑: Admin privileges required.

---

## 🖥️ Usage

```bash
# Show help
lltc

# Get current power mode
lltc get powermode                      # or: lltc get pm

# Set power mode
lltc set powermode Quiet                # or: lltc set pm 1
lltc set powermode Balance              # or: lltc set pm 2
lltc set powermode Performance          # or: lltc set pm 3
#lltc set powermode GodMode (coming soon...)

# Get current gpu working mode
lltc get gpumode                        # or: lltc get gm

# Set gpu working mode
lltc set gpumode Hybrid                 # or: lltc set gm 1
lltc set gpumode HybridIGPU             # or: lltc set gm 2
lltc set gpumode HybridAuto             # or: lltc set gm 3
lltc set gpumode dGPU                   # or: lltc set gm 4

# Get current battery mode
lltc get batterymode                    # or: lltc get bm

# Set battery mode
lltc set batterymode Conservation       # or: lltc set bm 1
lltc set batterymode Normal             # or: lltc set bm 2
lltc set batterymode RapidCharge        # or: lltc set bm 3

# Get current keyboard backlight state
lltc get keyboardbacklight              # or: lltc get kb

# Set keyboard backlight
lltc set keyboardbacklight Off          # or: lltc set kb 0
lltc set keyboardbacklight Low          # or: lltc set kb 1
lltc set keyboardbacklight High         # or: lltc set kb 2

# Get OverDrive status
lltc get overdrive                      # or: lltc get od

# Set OverDrive
lltc set overdrive on                   # or: lltc set od 1
lltc set overdrive off                  # or: lltc set od 0

# Get Always on USB status
lltc get alwaysonusb                    # or: lltc get ao

# Set Always on USB status
lltc set alwaysonusb Off                # or: lltc set ao 0
lltc set alwaysonusb OnWhenSleeping     # or: lltc set ao 1
lltc set alwaysonusb OnAlways           # or: lltc set ao 2

# Get detailed battery information
lltc get batteryinformation             # or: lltc get bi
lltc get batteryinformation -dmon       # monitoring mode (refresh rate 1s by default)
lltc get batteryinformation -dmon 3     # or: lltc get bi -dmon 3

# Turn off display
lltc monitoroff                         # or: lltc mo
```

---

## ⚠️ Requirements & Compatibility

- **Windows 10 or 11 (64-bit)**
- **Lenovo Legion** laptop
- **Lenovo Energy Management Driver** (`\\.\EnergyDrv`) installed

---

## 📦 Building from Source

This project is developed with **VS Code + GCC (MinGW-w64)** and requires no IDE. **Windows-only.**

### Prerequisites
- [MinGW-w64](https://www.mingw-w64.org/)
- `g++` (version **15.2.0**) in your `PATH`

### Option 1: Build via Terminal
Open a terminal in the project root and run:

```bash
g++ -std=c++26 -O2 -Wall -o lltc.exe lltc.cpp -static -s -lole32 -loleaut32 -lwbemuuid -luuid -lsetupapi -lpowrprof -lversion -lstdc++exp
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
                "-std=c++26",
                "-O2",
                "-Wall",
                "-o", "${workspaceFolder}/lltc.exe",
                "${workspaceFolder}/lltc.cpp",
                "-static",
                "-s",
                "-lole32",
                "-loleaut32",
                "-lwbemuuid",
                "-luuid",
                "-lsetupapi",
                "-lpowrprof",
                "-lversion",
                "-lstdc++exp"
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
- [ ] Implement GodMode
- [ ] Implement port backlight control
- [ ] Implement Always-on-USB control
- [ ] Implement Instant-Boot control
- [ ] Implement Flip-to-Start control
- [ ] Improve code quality

---

## 💬 Final Note

Contributions or issues are welcome!

Happy coding!