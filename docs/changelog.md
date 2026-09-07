# Changelog

## 1.0
- Initial release candidate of Battery Guardian (v1.0.0-rc1).
- Real-time physical SI telemetry observation (V, A, C, SOC).
- Coulombic capacity estimation over qualifying discharge sessions.
- Median-based statistical outlier filtering over 16-session history.
- Multi-factor confidence rating (High, Medium, Low, Insufficient).
- OLS linear degradation trend analysis.
- Crash-resilient binary journal with per-record CRC32 framing.
- Multi-state charge policy safety state machine with hysteresis guards.
- Passive fail-closed charger abstraction layer (zero PMIC register writes).
- Comprehensive 5-view user interface (Dashboard, Health, History, Sessions, Diagnostics).
