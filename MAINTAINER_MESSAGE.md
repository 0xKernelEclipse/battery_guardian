# Draft Review Message for Flipper Community & Maintainers

**Subject:** [App Proposal / Review] Battery Guardian — Battery Telemetry Observation, Capacity Learning & Safety FAP

---

Hi Flipper Team and Community Reviewers,

I have prepared an external application for the Flipper Zero called **Battery Guardian** (`v1.0.0-rc1`) and would appreciate your technical review on the architectural approach before proceeding toward an official catalog pull request.

### What Battery Guardian Does
Battery Guardian is an external FAP (`Tools` category) designed to provide deeper battery intelligence than standard instantaneous voltage lookups:
- Integrates Coulombic energy ($\int I \, dt$) over qualifying discharge sessions ($\Delta\text{SOC} \ge 15\%$) to estimate true battery capacity.
- Applies statistical median filtering over session history to reject outlier spikes.
- Computes multi-factor confidence ratings (`High`, `Medium`, `Low`, `Insufficient`) before presenting health estimates.
- Calculates long-term degradation trajectories using linear regression.
- Persists telemetry into an append-only binary journal with per-record CRC32 verification and an enforced 512 KB storage quota in `/ext/apps_data/battery_guardian/`.
- Evaluates multi-state charge policies (`Balanced` 80%, `Lifespan` 60%, `Full` 100%) with hysteresis and thermal lockout guards.

### Safety & Hardware Boundary
Because battery manipulation carries physical safety implications, I want to be completely upfront:
- **Physical hardware validation has NOT yet been performed** (I do not yet have access to a physical bench test setup with oscilloscope/electronic load).
- Consequently, the production Charger HAL is compiled in a **strictly passive, fail-closed configuration**.
- The app does NOT advertise charge control capabilities and issues **zero register writes** to the BQ25896 PMIC or hardware charging registers.
- All policy decisions and safety states are validated in software simulation and property-based fuzzing (100,000 transitions with zero invariant violations).

### What I Am Asking For
I am not asking for special endorsement, sponsorships, or free hardware. Rather, I would love your feedback on:
1. Is an external FAP the right boundary for this feature set, or are there platform constraints we should be aware of?
2. Are the power HAL APIs (`furi_hal_power_*`) stable for external app consumption across future releases?
3. Review of the catalog manifest and media assets (`manifest.yml`, `icon.png`, `screenshots/`).

The repository includes a comprehensive 118-test host test suite and full documentation:
- Repository: https://github.com/0xKernelEclipse/battery_guardian (Release Candidate `v1.0.0-rc1`)
- Reviewer Guide: `REVIEW.md`
- Hardware Bench Manual: `HARDWARE_VALIDATION.md` (Protocols B-01 through B-06)

Thank you for your time and feedback!

Best regards,  
0xKernelEclipse
