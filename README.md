

**Real-time Battery Management System with cell-fault handling for EV and BESS prototypes, built on QNX Neutrino 8.0.**

> Team Voltedge · Hyderabad · State Level Hackathon · Problem #9 (Automotive)
> Document: `SV-BMS-QNX-TPR-001` · Rev 1.0

---

## Overview

Sentinel-BMS is a real-time battery management prototype that detects and contains cell- and pack-level faults for electric-vehicle (EV) and battery-energy-storage-system (BESS) use cases. It runs on **QNX Neutrino 8.0** on a **Raspberry Pi 4/5** and separates sensing, fault detection, core state management, telemetry, web/CLI access, and recovery into isolated user-space processes.

The safety path is led by a fixed-priority fault-detection thread, making worst-case fault-to-decision time the primary design goal rather than average throughput.

**Core thesis:** deterministic scheduling and process isolation reduce the chance that a software fault or workload spike delays a safety decision — while independent hardware protection remains required for defense in depth.

---

## Key features

- **Deterministic safety loop** — highest-priority `fault_det` thread (priority 60) released by a periodic timer pulse, targeting a **10 ms fault-to-decision deadline**.
- **Stale-data-as-fault** — any sample older than 500 ms, a failed I²C read, or a dead process is treated as unsafe and drives the safe state.
- **Process isolation** — each component runs in its own protected address space; a crashed dashboard or sensor process cannot corrupt the fault detector.
- **Debounced detection** — three consecutive 100 ms critical samples required before latching a threshold violation (noise filtering), while freshness violations trip immediately.
- **Latched fail-safe state** — a fault stays latched until a controlled, policy-driven re-arm; restart never silently clears a fault.
- **Supervised recovery** — a recovery manager restarts failed processes (~1 s target) and revalidates before re-arming.
- **Observability** — QNX System Profiler / tracelog capture of per-thread timing, CPU usage, and decision latency.

---

## Architecture

```
acquire → timestamp → validate → detect → decide → actuate → log → recover
```

Three planes:

- **Data plane** — measurements and status
- **Control plane** — requests, pulses, fault events, safe-state commands
- **Safety plane** — the minimal always-live path: `fault_det`, the state machine, and the actuation interface

The UI and telemetry are observers only — they are never the authority for closing a fault or re-arming the pack.

### Process / thread model

| Process / thread | Priority | Responsibility | Safety class |
|---|---|---|---|
| `fault_det` | 60 | Validate freshness, evaluate limits, latch trip | Critical |
| `telem_acq` | 50 | Acquire and timestamp measurement frames | Important |
| `bms_sensor` | 45 | Sensor service, I²C/GPIO access | Important |
| `ina219` / charger | 40 | Device measurement / charge service | Important |
| `thermal` | 35 | Thermal processing and trends | Important |
| `bms_core` | 30 | State machine, policy, SoC/SoH, shared status | Critical support |
| `bms_web` / CLI | 10 | Dashboard and operator observation | Non-critical |
| `bms_can` | 10–30 (proposed) | Pack/network frames and diagnostics | Non-critical until validated |

---

## Hardware

| Component | Role | Interface |
|---|---|---|
| Raspberry Pi 4/5 | Compute platform | — |
| INA219 | Pack voltage / current monitor | I²C |
| DHT11 | Temperature / humidity (demo sensor) | GPIO4 |
| 3S Li-ion pack | Demonstrator battery | — |

Sampling: INA219 at **100 ms**, DHT11 at **2 s**, monotonic timestamps on every sample.

---

## Prototype thresholds

| Fault | Threshold | Action |
|---|---|---|
| Pack over-voltage | > 12.60 V (3 ticks) | Inhibit charge; open contactor |
| Pack under-voltage | < 9.00 V (3 ticks) | Inhibit discharge |
| Over-current | > 3.00 A (3 ticks) | Open/inhibit power path |
| Over-temperature | > 55 °C (3 ticks) | Stop charge/discharge; alarm |
| Stale / invalid data | age > 500 ms or I²C failure | Immediate safe state |
| Sensor process death | exit / heartbeat timeout | Immediate safe state; restart |

> **Note:** these are demonstration values and must be recalculated from cell chemistry, pack configuration, sensor accuracy, and hazard analysis before any deployment.

---

## Performance targets

| Metric | Target |
|---|---|
| Decision latency | ≤ 10 ms (report max, p99, min) |
| Deadline misses | 0 in acceptance run |
| Sample age | < 500 ms or immediate stale fault |
| Recovery time | ~1 s process restart (not safety latency) |
| Fault coverage | 100% of specified prototype cases |

---

## Verification

A fault-injection test plan (V-01 through V-10) covers boundary voltages, debounce behavior, stale-data handling, `SIGKILL` of the sensor process, web/CLI flooding, I²C bus faults, thermal ramps, contactor feedback mismatch, power-cycle-during-fault, and worst-case load latency.

Each test records injected condition, expected vs. actual state, timestamps, threshold version, and whether a **physical** safe-state transition was observed. Raw traces are preserved, not just screenshots.

---

## Scope & limitations

This is a **proof-of-concept prototype**, not a certified BMS.

- Cell values are estimated from pack ÷ 3 — no individual cell taps.
- The contactor is a software state; **no wired hardware cutoff**.
- CAN is a message-path stub, not a validated vehicle bus.
- DHT11 is a slow demonstration sensor, not production thermal hardware.
- No claim of ISO 26262, IEC 61508, UNECE R100, or UL/NFPA compliance.

### Production path

Multi-cell AFE (e.g. BQ76952-class), independent hardware over/under-voltage, over-current and over-temperature paths, distributed temperature sensing, contactor driver with auxiliary feedback and welded-contact detection, dual watchdog, validated CAN/CAN-FD, and a safety case aligned with applicable regulations (UNECE R100 for REESS).

---

## Tech stack

- **RTOS:** QNX Neutrino 8.0 (microkernel, POSIX user-space processes)
- **IPC:** synchronous message passing (`MsgSend`/`MsgReceive`/`MsgReply`), pulses, timers
- **Hardware:** Raspberry Pi 4/5, INA219, DHT11, GPIO
- **Tooling:** QNX System Profiler / tracelog

---

## References

Key sources behind the design: QNX SDP 8.0 System Architecture docs, Texas Instruments INA219 and BQ76952 datasheets, UNECE Regulation No. 100, and Sandia/DOE battery-safety material. See the full technical report for the complete citation list.

---

## License

See `LICENSE`. Project documentation is confidential to Team Voltedge.
