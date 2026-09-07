# Security Policy

## Supported Versions

| Version | Supported          | Notes |
| ------- | ------------------ | ----- |
| 1.0.x-rc | :white_check_mark: | Active release candidate |
| < 1.0.0 | :x:                | Obsolete prototype phases |

## Reporting a Vulnerability

Battery Guardian monitors and evaluates battery health and charging policies. Because battery manipulation on embedded hardware carries thermal and fire safety hazards, we take software security and safety invariants with extreme seriousness.

If you discover a security vulnerability, buffer overflow, state machine flaw, or fail-safe bypass:

1. **Do NOT disclose the issue publicly** in an open GitHub issue or public forum.
2. Please use GitHub's **Private Vulnerability Reporting** feature on the repository:
   - Navigate to the **Security** tab of the repository on GitHub.
   - Click **Report a vulnerability** to open an advisory draft.
3. If Private Vulnerability Reporting is unavailable, open an issue titled `[SECURITY DISCLOSURE REQUEST]` requesting a private communication channel without posting exploit code or vulnerability details in the issue body.

### What to Include in Your Report
- A detailed description of the vulnerability (e.g. out-of-bounds access, state machine deadlock, parser flaw).
- Step-by-step reproduction instructions or a minimal test case.
- An assessment of potential impact (e.g. memory corruption, denial of service, unintended hardware register write).
- Proposed mitigation or patch if you have developed one.

### Response Timeline
- Initial acknowledgment: within 48 hours.
- Status update and validation: within 7 days.
- Patch release and disclosure coordination: mutually agreed timeline following fix verification.
