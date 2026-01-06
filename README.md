# Legion Laptop Toolkit CLI (LLTC)

A lightweight, command-line-only tool to control a couple of features that are only available in Lenovo Vantage or Legion Zone with **no background service**.

> ℹ️ **Note**: This project is **actively maintained** and was inspired by the now-archived [LenovoLegionToolkit (LLT)](https://github.com/BartoszCichecki/LenovoLegionToolkit).
> Special thanks to [@BartoszCichecki](https://github.com/BartoszCichecki) and the LLT community for their foundational work on Lenovo laptop control interfaces.

---

## 🛠 Features

- Get current battery charging status (`ischarging`)
- Get current battery mode: `Conservation`, `Normal`, or `RapidCharge`
- Set battery mode via name or number:
  - `1` / `Conservation`
  - `2` / `Normal`
  - `3` / `RapidCharge`
- Get or set OverDrive status (`on`/`off` or `1`/`0`)
- Instantly turn off the display (`monitoroff`)

---

## 🖥️ Usage

```bash
# Show help
lltc

# Turn off display
lltc monitoroff        # or: lltc mo

# Get charging status
lltc get ischarging    # or: lltc get ic

# Get current battery mode
lltc get batterymode   # or: lltc get bm

# Set battery mode
lltc set batterymode Conservation   # or: lltc set bm 1
lltc set batterymode Normal         # or: lltc set bm 2
lltc set batterymode RapidCharge    # or: lltc set bm 3

# Get OverDrive status
lltc get overdrive     # or: lltc get od

# Set OverDrive
lltc set overdrive on  # or: lltc set od 1
lltc set overdrive off # or: lltc set od 0
```

---

## ⚠️ Requirements & Compatibility

- **Windows 10 or 11 (64-bit)**
- **Lenovo Legion (Gen 6+), IdeaPad Gaming, or LOQ** laptop
- **Lenovo Energy Management Driver** installed (typically included with Lenovo Vantage)

If the `\\.\EnergyDrv` device is not present, all battery-related commands will fail. Ensure Lenovo system software is properly installed.

---

## 📦 Building from Source

This project is developed with **VS Code + GCC (MinGW-w64)** and requires no IDE.

### Prerequisites
- [MinGW-w64](https://www.mingw-w64.org/)
- `g++` in your `PATH`

### Build Steps
1. Open terminal in project root
2. Run:
   ```bash
   g++ -O2 -s -static -Wall -Wextra -o lltc.exe lltc.cpp -lkernel32 -luser32
   ```

> The resulting `lltc.exe` is **standalone** — no DLLs or redistributables required.


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
- [X] Implement OverDrive control
- [ ] Implement power mode switching (Quiet/Balance/Performance)

---

## 💬 Final Note

Contributions or issues are welcome!

Happy coding!