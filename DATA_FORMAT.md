# Battery Guardian Journal Binary Data Format

Battery Guardian persists time-series telemetry, session summaries, semantic events, and capacity estimates to an append-only binary journal file stored at `/ext/battery_guardian.log`.

---

## 1. File Layout Overview

A valid journal file begins with exactly one **Global Header**, followed by zero or more **Record Envelopes**:

```
+-------------------------------------------------------------+
|                     GLOBAL FILE HEADER                      |
|                   (JournalGlobalHeader, 9 bytes)            |
+-------------------------------------------------------------+
|                     RECORD 0 ENVELOPE                       |
|  - Record Header (JournalRecordHeader, 16 bytes)            |
|  - Payload Data (Variable length N)                         |
|  - CRC32 Checksum (uint32_t, 4 bytes, of Payload Data)      |
+-------------------------------------------------------------+
|                     RECORD 1 ENVELOPE                       |
|  ...                                                        |
+-------------------------------------------------------------+
```

---

## 2. Global File Header

Written once when a new journal is initialized:

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;             // 0x42415447 ("BATG")
    uint8_t format_version;     // Format version (currently 1)
    uint32_t device_session_id; // Monotonic boot/session identifier
} JournalGlobalHeader;
```

- **Magic (`0x42415447`)**: Identifies the file as a Battery Guardian journal.
- **Format Version (`1`)**: Future versions with incompatible record envelope schemas will increment this field. Readers encountering `format_version > JOURNAL_FORMAT_VERSION` safely abort without modifying or truncating the file.

---

## 3. Record Envelope Structure

Each record is prepended with a 16-byte header and appended with a 4-byte CRC32:

```c
typedef struct __attribute__((packed)) {
    uint8_t type;           // JournalRecordType (1..5)
    uint8_t version;        // Payload schema version
    uint16_t length;        // Byte length of payload data
    uint32_t sequence;      // Monotonically increasing record ID
    uint64_t timestamp;     // 64-bit monotonic timestamp in ms
} JournalRecordHeader;
```

Following `JournalRecordHeader`, the raw payload of `length` bytes is written, followed immediately by:

```c
uint32_t payload_crc32; // CRC-32 computed over payload bytes (seed: 0xFFFFFFFF)
```

---

## 4. Record Types & Payloads

### Type 1: Telemetry Sample (`RecordTypeSample`)
- **Version**: 1
- **Payload Struct**: `BatteryTelemetry` (~52 bytes)
  - `timestamp_ms` (`uint64_t`): Monotonic millisecond timestamp.
  - `voltage_v` (`float`): Fuel gauge voltage.
  - `current_a` (`float`): Positive = charging, negative = draining.
  - `temperature_c` (`float`): Battery cell temperature.
  - `soc_pct` (`uint8_t`): State of charge percentage (0–100%).
  - `remaining_capacity_mah` (`uint32_t`): Fuel gauge reported remaining capacity.
  - `full_capacity_mah` (`uint32_t`): Fuel gauge reported full capacity.
  - `design_capacity_mah` (`uint32_t`): Nominal design capacity (2100 mAh).
  - `gauge_health_pct` (`uint8_t`): Fuel gauge SOH percentage.
  - `flags` (`uint32_t`): Bitmask of verified sensor validity flags.

### Type 2: Session Summary (`RecordTypeSession`)
- **Version**: 1
- **Payload Struct**: `BatterySession` (~60 bytes)
  - `session_id` (`uint32_t`): Incremental session index.
  - `type` (`uint8_t`): `Charging (1)`, `Discharging (2)`, `Idle (3)`.
  - `start_timestamp` / `end_timestamp` (`uint64_t`).
  - `start_soc` / `end_soc` (`uint8_t`).
  - `accumulated_energy_mah` (`float`): Integrated Coulombic charge.
  - `quality_flags` (`uint32_t`): Quality indicators for candidate eligibility.

### Type 3: Semantic Event (`RecordTypeEvent`)
- **Version**: 1
- **Payload Struct**: `BatteryEventPayload`
  - `event_type` (`uint8_t`): Enumerated event code.
  - `severity` (`uint8_t`): `Info (1)`, `Warning (2)`, `Error (3)`.
  - `message` (`char[32]`): Human-readable event description.

### Type 4: Capacity Estimate (`RecordTypeEstimate`)
- **Version**: 1
- **Payload Struct**: `EstimateRecordPayload`
  - `robust_estimate_mah` (`float`): Filtered capacity estimate.
  - `accepted_candidates` (`uint32_t`): Total accepted sessions.
  - `rejected_outliers` (`uint32_t`): Total rejected outliers.
  - `confidence_level` (`uint8_t`): `High (3)`, `Medium (2)`, `Low (1)`, `Insufficient (0)`.

---

## 5. Self-Healing Recovery Algorithm

On startup, `journal_recover_ex()` verifies the entire file sequentially:
1. Validates the global header magic and format version.
2. For each record, validates `type`, payload `length` bounds ($\le 1024$ bytes), and compares the computed CRC32 against the stored CRC32.
3. If an incomplete write, truncated record, or CRC mismatch is encountered:
   - Scanning immediately halts.
   - The file is truncated at `valid_offset` via `storage_file_truncate()` (O(1) filesystem operation).
   - All preceding valid records are completely preserved.
   - New records append seamlessly starting from the verified byte offset.
