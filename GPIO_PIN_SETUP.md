# Raspberry Pi 4 GPIO Pin Setup & Hardware Wiring Guide
### Target: QNX Neutrino RTOS 8.0 | EV Battery Management System (BMS)
### Architecture: Broadcom BCM2711 (ARM Cortex-A72 / AArch64)

---

## 1. Overview of Connected Peripherals

This BMS setup uses **only two physical sensor peripherals**:
1. **INA219 High-Side Power & Current Monitor** via Hardware **I2C1** (Address: `0x40`)
2. **DHT11 Temperature & Relative Humidity Sensor** via **BCM GPIO 4** (Physical Pin 7)

> **Confirmed Hardware Configuration:**
> - **NO BME280 / BMP280** is used.
> - **NO Buzzer** is used.
> - **NO LEDs** are used.
> - All telemetry originates strictly from genuine hardware readings (INA219 + DHT11). If disconnected, the system reports `N/A` and opens the high-voltage contactor.

---

## 2. Complete Raspberry Pi 4 (40-Pin Header) Pinout Map

| Physical Pin # | Pin Name / Function | BCM GPIO | BMS Setup Status | Connected Device & Wire |
| :---: | :---: | :---: | :---: | :--- |
| **Pin 01** | **3.3V Power** | — | **CONNECTED** | **VCC (INA219 & DHT11 VCC)** |
| Pin 02 | 5.0V Power | — | Unused | *Do not connect (3.3V logic only)* |
| **Pin 03** | **GPIO 2 (I2C1 SDA)** | **GPIO 2** | **CONNECTED** | **INA219 SDA (Data Line)** |
| Pin 04 | 5.0V Power | — | Unused | *Do not connect* |
| **Pin 05** | **GPIO 3 (I2C1 SCL)** | **GPIO 3** | **CONNECTED** | **INA219 SCL (Clock Line)** |
| **Pin 06** | **Ground (GND)** | — | **CONNECTED** | **GND (Common Ground)** |
| **Pin 07** | **GPIO 4 (GPCLK0)** | **GPIO 4** | **CONNECTED** | **DHT11 DATA (Single-Wire Bus)** |
| Pin 08 | GPIO 14 (UART TX) | GPIO 14 | Unused | QNX Serial Console (Optional) |
| Pin 09 | Ground (GND) | — | Unused | Available ground |
| Pin 10 | GPIO 15 (UART RX) | GPIO 15 | Unused | QNX Serial Console (Optional) |
| Pin 11 | GPIO 17 | GPIO 17 | **UNUSED** | *No LED* |
| Pin 12 | GPIO 18 (PCM CLK) | GPIO 18 | Unused | Unused |
| Pin 13 | GPIO 27 | GPIO 27 | **UNUSED** | *No LED* |
| Pin 14 | Ground (GND) | — | Unused | Available ground |
| Pin 15 | GPIO 22 | GPIO 22 | **UNUSED** | *No LED* |
| Pin 16 | GPIO 23 | GPIO 23 | **UNUSED** | *No LED* |
| Pin 17 | 3.3V Power | — | Unused | Secondary 3.3V rail |
| Pin 18 | GPIO 24 | GPIO 24 | **UNUSED** | *No Buzzer* |
| Pin 19 | GPIO 10 (SPI MOSI) | GPIO 10 | Unused | Unused |
| Pin 20 | Ground (GND) | — | Unused | Available ground |
| Pin 21 | GPIO 9 (SPI MISO) | GPIO 9 | Unused | Unused |
| Pin 22 | GPIO 25 | GPIO 25 | Unused | Unused |
| Pin 23 | GPIO 11 (SPI SCLK) | GPIO 11 | Unused | Unused |
| Pin 24 | GPIO 8 (SPI CE0) | GPIO 8 | Unused | Unused |
| Pin 25 | Ground (GND) | — | Unused | Available ground |
| Pin 26 | GPIO 7 (SPI CE1) | GPIO 7 | Unused | Unused |
| Pin 27 | ID_SD (I2C ID EEPROM)| — | Unused | Reserved for HAT EEPROM |
| Pin 28 | ID_SC (I2C ID EEPROM)| — | Unused | Reserved for HAT EEPROM |
| Pin 29 | GPIO 5 | GPIO 5 | Unused | Unused |
| Pin 30 | Ground (GND) | — | Unused | Available ground |
| Pin 31 | GPIO 6 | GPIO 6 | Unused | Unused |
| Pin 32 | GPIO 12 (PWM0) | GPIO 12 | Unused | Unused |
| Pin 33 | GPIO 13 (PWM1) | GPIO 13 | Unused | Unused |
| Pin 34 | Ground (GND) | — | Unused | Available ground |
| Pin 35 | GPIO 19 (PCM FS) | GPIO 19 | Unused | Unused |
| Pin 36 | GPIO 16 | GPIO 16 | Unused | Unused |
| Pin 37 | GPIO 26 | GPIO 26 | Unused | Unused |
| Pin 38 | GPIO 20 (PCM DIN) | GPIO 20 | Unused | Unused |
| Pin 39 | Ground (GND) | — | Unused | Available ground |
| Pin 40 | GPIO 21 (PCM DOUT) | GPIO 21 | Unused | Unused |

---

## 3. Peripheral Wiring Schematics

### 3.1 INA219 (I2C Sensor) Wiring
The INA219 sensor communicates over the primary I2C bus (`/dev/i2c1` at 7-bit slave address `0x40`).

```
Raspberry Pi 4 Header                   INA219 Breakout Board
---------------------                   ---------------------
Pin 1  (3.3V DC Power)  ───────────────► VCC
Pin 6  (GND Common)     ───────────────► GND
Pin 3  (GPIO 2 / SDA)   ───────────────► SDA
Pin 5  (GPIO 3 / SCL)   ───────────────► SCL

High-Voltage Battery Pack Connections:
--------------------------------------
Battery Pack Positive (+) Terminal ────► Vin+ (Terminal Block)
Propulsion Load / Controller Positive ─► Vin- (Terminal Block)
Pack Ground / Chassis Ground ──────────► Shared Common GND
```

- **Shunt Resistor Value**: 0.1 Ω (standard onboard SMD shunt).
- **Measurement Capacity**: Up to 26 V bus voltage and ±3.2 A current measurement.

---

### 3.2 DHT11 (Temperature & Humidity Sensor) Wiring
The DHT11 sensor uses a custom single-wire proprietary protocol on **BCM GPIO 4**.

```
Raspberry Pi 4 Header                   DHT11 Sensor
---------------------                   ------------
Pin 1  (3.3V DC Power)  ───────────────► VCC (+)
Pin 6  (GND Common)     ───────────────► GND (-)
Pin 7  (BCM GPIO 4)     ───────────────► DATA (Out)
```

> **Pull-Up Resistor Note:**
> - If you are using a standard **3-pin DHT11 module** (mounted on a small PCB with a built-in resistor and power LED), connect `DATA` directly to **Pin 7**.
> - If using a bare **4-pin DHT11 sensor**, place a **4.7 kΩ to 10 kΩ resistor** between Pin 1 (3.3V) and Pin 7 (DATA).

---

## 4. QNX Neutrino 8.0 BCM2711 Driver Architecture

Under QNX Neutrino RTOS on Raspberry Pi 4, the software interacts with the hardware through two isolated layers:

### 4.1 I2C Driver (`/dev/i2c1`)
The QNX I2C resource manager handles low-level I2C arbitration on BCM GPIO 2 & 3.
- Device node: `/dev/i2c1`
- Start command (if not launched by QNX build file / IFS):
  ```sh
  i2c-bcm2835 -p 0xFE804000 -i 117
  ```
- Driver interaction: Performed via `devctl()` with `DCMD_I2C_SEND` and `DCMD_I2C_RECV`.

### 4.2 BCM2711 Physical MMIO Mapping (GPIO 4)
User-space bit-banging of DHT11 on GPIO 4 requires direct physical register access:
- **BCM2711 Peripheral Base**: `0xFE000000`
- **GPIO Base**: `0xFE200000` (`0xFE000000 + 0x200000`)
- **Mapping Call**:
  ```c
  mmap_device_memory(NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, 0xFE200000ULL);
  ```
- **Relevant Registers**:
  - `GPFSEL0` (offset `0x00`): Sets GPIO 4 as Input (`000`) or Output (`001`).
  - `GPSET0`  (offset `0x1C`): Sets GPIO 4 HIGH.
  - `GPCLR0`  (offset `0x28`): Clears GPIO 4 LOW.
  - `GPLEV0`  (offset `0x34`): Reads current logic level of GPIO 4.
  - `GPIO_PUP_PDN_CNTRL_REG0` (offset `0xE4`): Internal Pull-Up/Pull-Down selection for GPIO 0–15.

---

## 5. Hardware Verification on Target

To verify the wiring and ensure all pins are functioning properly before starting the main BMS processes, run the standalone verification tool:

```sh
# On Raspberry Pi 4 as root:
cd /home/root/ev_bms_final
./build/test_gpio
```

### Expected Output:
```text
========================================================================================
 RASPBERRY PI 4 GPIO & SENSOR HARDWARE VERIFICATION (QNX NEUTRINO 8.0)
========================================================================================

[1/3] Checking Physical GPIO MMIO Access (BCM2711 Base: 0xFE200000)...
      [PASS] Physical GPIO MMIO successfully mapped into virtual address space.

[2/3] Checking DHT11 Single-Bus Pin (Physical Pin 7 | BCM GPIO 4)...
      Current logic level on Pin 7 (GPIO 4): 1 (HIGH / IDLE (PULL-UP OK))
      [PASS] GPIO 4 is idling HIGH. Ready for DHT11 communication.

[3/3] Checking I2C1 Bus Device (/dev/i2c1 | Address 0x40)...
      [PASS] INA219 detected at I2C address 0x40! Config Register: 0x399F

========================================================================================
 RASPBERRY PI 4 (40-PIN HEADER) PIN CONFIGURATION SUMMARY
========================================================================================
   Pin  Function           Status        Connected Peripheral
   ---  --------           ------        --------------------
   01   3.3V Power         CONNECTED     VCC (INA219 & DHT11)
   03   GPIO 2 (I2C1 SDA)  CONNECTED     INA219 SDA
   05   GPIO 3 (I2C1 SCL)  CONNECTED     INA219 SCL
   06   Ground (GND)       CONNECTED     GND (Common Ground)
   07   GPIO 4 (GPCLK0)    CONNECTED     DHT11 DATA
   11   GPIO 17            UNUSED        (No LED)
   13   GPIO 27            UNUSED        (No LED)
   15   GPIO 22            UNUSED        (No LED)
   16   GPIO 23            UNUSED        (No LED)
   18   GPIO 24            UNUSED        (No LED)
   --   I2C 0x76 / 0x77    UNUSED        (No BME280)
========================================================================================
```
