# Legion Laptop Toolkit CLI (LLTC)

A lightweight, command-line-only tool to control battery charging modes on compatible Lenovo Legion laptops with **no background service**.

> ℹ️ **Note**: This project is **actively maintained** and was inspired by the now-archived [LenovoLegionToolkit (LLT)](https://github.com/BartoszCichecki/LenovoLegionToolkit).
> Special thanks to [@BartoszCichecki](https://github.com/BartoszCichecki) and the LLT community for pioneering the reverse-engineering of Lenovo’s `EnergyDrv` driver interface.

---

## 🛠 Features

- Get current battery charging status (`ischarging`)
- Get current battery mode: `Conservation`, `Normal`, or `RapidCharge`
- Set battery mode via name or number:
  - `1` / `Conservation`
  - `2` / `Normal`
  - `3` / `RapidCharge`
- Instantly turn off the display (`monitoroff`)

All operations communicate directly with Lenovo’s kernel-mode driver `\\.\EnergyDrv` using standard Windows `DeviceIoControl` calls.

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
lltc set batterymode Normal   # or: lltc set bm 2
lltc set batterymode RapidCharge   # or: lltc set bm 3
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

- [ ] Implement power mode switching (Quiet/Balance/Performance)

---

## 💬 Final Note

This tool exists to empower automation, scripting, and minimal-power management on Lenovo gaming laptops — without GUI overhead or background services.

Contributions or issues are welcome!

Happy coding!