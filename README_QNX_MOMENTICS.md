# EV Battery Management System (BMS)
### Target: QNX Neutrino 8.0 on Raspberry Pi 4 (Broadcom BCM2711 / AArch64) | Fail-Safe Embedded Architecture

---

## 1. Overview & Genuine Hardware Monitored Parameters

This software provides real-time battery management telemetry and operating system workload metrics across the **HTML Web Dashboard**, **QNX OS Terminal CLI**, and **REST API**:

| Category | Parameter | Hardware Driver & Sensor Source | Failure State Display |
| :--- | :--- | :--- | :--- |
| **Safety Status** | **Status** | `okay`, `running`, `fault` evaluated from live hardware readings | `fault` (Loss of Telemetry) |
| **Electrical** | **Pack Voltage** | INA219 High-Side Shunt Monitor on I2C1 (`0x40`) | `N/A [INA219 OFFLINE]` |
| | **Pack Current** | INA219 Shunt Voltage across 0.1 Ω precision shunt resistor | `N/A [INA219 OFFLINE]` |
| | **Pack Power** | $P = V_{pack} \times I_{pack}$ | `N/A` |
| | **3S Cell Voltage** | Series average ($V_{pack} / 3.0$) based on single-pack hardware monitor | `N/A` |
| | **State of Charge (SOC)**| Evaluated from piecewise open-circuit voltage (OCV) curve | `N/A` |
| | **State of Health (SOH)**| 100.0% nominal design baseline capacity retention | `N/A` |
| | **Contactor State** | Hardware fail-safe logic: CLOSED only when sensors are OK | `OPEN (SAFE CUTOFF)` |
| | **Charger Interlock** | Truthful hardware state: interlock open (always NOT CONNECTED) | `NOT CONNECTED` |
| **Environmental** | **Temperature** | DHT11 Single-Bus sensor on BCM GPIO 4 | `N/A [DHT11 OFFLINE]` |
| | **Humidity** | DHT11 Single-Bus sensor on BCM GPIO 4 (% RH) | `N/A` |
| **QNX Workload** | **PID & PPID** | Child and parent Process IDs via `getpid()` / `getppid()` | Live System Table |
| | **TID & Priority** | POSIX Thread IDs and `SCHED_FIFO` priorities | Live System Table |
| | **Memory** | Physical RAM utilization via `sysconf(_SC_PHYS_PAGES)` | Live System RAM MB / % |
| | **Disks** | Storage filesystem utilization via QNX `statvfs()` | Live QNX `/` mount info |

> **Safety Notice:** If any sensor disconnects, fails I2C/CRC communication, or reports stale data, the BMS immediately trips to **FAULT**, de-energizes the contactor to **OPEN** (fail-safe cutoff), and renders `N/A` on the dashboard and CLI. No dummy, random, or simulated telemetry is ever substituted.

---

## 2. Hardware Wiring (Raspberry Pi 4 Pinout)

Connect the sensors to the 40-pin GPIO header on Raspberry Pi 4:

```
Raspberry Pi 4 Header       Peripheral Sensor
---------------------       -----------------
Pin 1  (3.3V Power)   ----> VCC (INA219 & DHT11 VCC)
Pin 6  (GND Ground)   ----> GND (Common Grounds)
Pin 3  (GPIO 2 / SDA) ----> SDA (INA219 SDA)
Pin 5  (GPIO 3 / SCL) ----> SCL (INA219 SCL)
Pin 7  (GPIO 4)       ----> DATA (DHT11 Data Pin)
```

*(Note: No external LEDs, buzzer, or BME280 are required or utilized.)*

---

## 3. Compiling with QNX SDP 8.0

### From Command Line:
Initialize the QNX SDP environment and invoke `make`:
```sh
# On Windows Host with QNX SDP 8.0:
call C:\Users\ajith\qnx800\qnxsdp-env.bat
cd c:\Users\ajith\ide-8.0.3-workspace\tata_punch_bms_final_111\ev_bms_final
make clean
make
```

All 4 binaries compile using `qcc -Vgcc_ntoaarch64le` into the `build/` directory:
- `build/bms_sensor` : Hardware acquisition daemon (I2C1 INA219 & BCM2711 GPIO 4 DHT11)
- `build/bms_core`   : BMS safety core with POSIX SCHED_FIFO tasks & fail-safe contactor logic
- `build/bms_web`    : Embedded micro HTTP daemon serving `/api/state` and `dashboard.html`
- `build/bms_cli`    : Native QNX OS interactive ANSI terminal monitor

---

## 4. Importing into QNX Momentics IDE 8.0

1. Launch **QNX Momentics IDE** (`C:\QNX\qnxmomenticside_3\qde.exe`).
2. Choose your workspace directory: `c:\Users\ajith\ide-8.0.3-workspace`.
3. Go to **File -> Import... -> General -> Existing Projects into Workspace**.
4. Browse to: `c:\Users\ajith\ide-8.0.3-workspace\tata_punch_bms_final_111\ev_bms_final`.
5. Select `ev_bms_final` and click **Finish**.
6. The project will automatically load with the QNX C Nature and Makefile builder configured for `QNX_AArch64_RPi4`.
7. Right-click the project in Project Explorer and choose **Build Project** (`Ctrl + B`).

---

## 5. Deployment and Execution on Raspberry Pi 4

1. **Transfer Project to Raspberry Pi 4:**
   ```sh
   scp -r ev_bms_final root@<raspberry_pi_ip>:/home/root/
   ```

2. **Run All BMS Daemons:**
   On the Raspberry Pi 4 target terminal as root:
   ```sh
   cd /home/root/ev_bms_final
   sh run.sh
   ```

3. **Launch the Live Terminal Monitor:**
   In another QNX target shell (or run `sh run.sh cli`):
   ```sh
   ./build/bms_cli
   ```

4. **Access the Web Dashboard:**
   Open any web browser on the network and navigate to:
   ```text
   http://<raspberry_pi_ip>:8080/dashboard.html
   ```
