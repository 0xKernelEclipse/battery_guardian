# Battery Guardian — Hardware Access & Physical Validation Plan

**Milestone:** Post-v1.0.0-rc1 Hardware Validation  
**Current Status:** Hardware Not Available / Physical Bench Tests Not Performed  

This document outlines legitimate, transparent strategies for obtaining access to physical Flipper Zero hardware and bench instrumentation to execute protocols **B-01 through B-06** defined in [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md).

---

## 1. Principles of Hardware Engagement

- **No Entitlement:** We do not expect or request free hardware from Flipper Devices or community members as an entitlement.
- **Evidence-First:** Hardware access is sought strictly to execute structured, documented electrical bench protocols with calibrated instruments.
- **Safety First:** Bench testing begins with passive telemetry observation before any controlled actuation trials are attempted.

---

## 2. Potential Pathways for Hardware Validation

### Pathway A: Personal Flipper Zero Hardware (Primary Path)
- **Description:** Developer acquires standard retail Flipper Zero hardware.
- **Action:**
  - Flash official release firmware.
  - Install `dist/battery_guardian.fap` via qFlipper.
  - Execute passive observation protocols (`B-01`, `B-04`, `B-06`) during normal personal daily usage.
  - Conduct bench instrumentation protocols (`B-05`, `B-03`, `B-02`) with bench power supplies in an electronics lab setting.

### Pathway B: Community Hardware Tester Collaboration
- **Description:** Partner with verified community developers or electronics enthusiasts in the Flipper community who possess bench multimeters, oscilloscopes, and battery testing fixtures.
- **Action:**
  - Publish `HARDWARE_VALIDATION.md` and `HARDWARE_EVIDENCE_TEMPLATE.md` publicly.
  - Invite independent community testers to run Protocol `B-01` (telemetry accuracy) and `B-04` (USB bounce) and submit pull requests with raw measurement logs.
  - Review and archive submitted bench evidence.

### Pathway C: Authorized Developer / Maintainer Review Trial
- **Description:** If Flipper core maintainers express interest in evaluating the battery intelligence algorithms during catalog review, maintainers may run passive observation or laboratory bench tests on their internal test fleets.
- **Action:**
  - Provide maintainers with self-contained test artifacts and instructions.
  - Integrate maintainer feedback and hardware trace logs into `hardware/` archives.

---

## 3. Physical Validation Gates Required for Active Control

Under no circumstances will active charge control (`CHARGER_CAP_CHARGE_CONTROL`) be enabled in production builds until:
1. Protocols B-01, B-04, B-05, B-06 pass on real hardware.
2. Protocol B-03 (thermal cutoff) proves software shuts down charging when cell exceeds 45°C.
3. Protocol B-02 (charge suppression transient) captures oscilloscope waveforms proving clean current drop without voltage ringing.
4. All bench evidence is logged with instrument models and serial numbers in [HARDWARE_EVIDENCE_TEMPLATE.md](HARDWARE_EVIDENCE_TEMPLATE.md).
