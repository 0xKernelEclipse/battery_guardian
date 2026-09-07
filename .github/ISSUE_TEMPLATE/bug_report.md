---
name: Bug report
about: Create a report to help improve Battery Guardian
title: '[BUG] '
labels: bug
assignees: ''
---

### Environment & Versions
- **Battery Guardian Version**: (e.g. v1.0.0-rc1)
- **Flipper Firmware Version / Release**: (e.g. Official 0.101.2, Momentum, Unleashed)
- **Target API Version**: (e.g. API 87.1, Target 7)
- **Execution Mode**: [ ] Physical Hardware  [ ] Host Simulation

### Hardware State (if physical hardware)
- **Hardware Revision**: (e.g. F7B5C4)
- **Battery Age / Condition**: (e.g. Original, Swapped, Swollen)
- **Power Connection**: [ ] Battery Only  [ ] USB Connected  [ ] Charging

### Bug Description
A clear and concise description of what the bug is.

### Steps to Reproduce
1. Launch Battery Guardian
2. Navigate to '...'
3. Trigger '...'
4. See error

### Expected Behavior
A clear and concise description of what you expected to happen.

### Observed Behavior
What actually happened (e.g. unexpected lockout, incorrect metric display, unexpected log message).

### Diagnostics & Logs
If available, provide the values from the **Diagnostics & Safety** view:
- **Gauge**: (OK / FAIL)
- **SD**: (OK / FAIL)
- **Pol**: (e.g. BALANCED)
- **Ctrl**: (e.g. Passive)
- **Samples**: Read: ___  Bad: ___
- **Sessions**: Completed: ___  Locks: ___

Relevant console / serial log lines (`furi_log` output):
```text
[Paste logs here]
```
