# Security Policy & Vulnerability Disclosure

## 1. Scope & Bug Bounty Clarification

Battery Guardian is an open-source community project developed for the Flipper Zero. **We do NOT operate a paid or monetary bug bounty program.** We deeply appreciate responsible disclosures from security researchers and embedded firmware developers who report potential hazards to help protect the community.

---

## 2. Supported Versions

| Version | Supported | Status |
|---|:---:|---|
| `v1.0.0-rc1` | :white_check_mark: | Active release candidate (Security patches supported) |
| `< 1.0.0` | :x: | Obsolete development milestones |

---

## 3. What Constitutes a Security Issue

A security issue in Battery Guardian involves flaws that could compromise device integrity, bypass safety invariants, or corrupt persistent storage:
- **Memory Safety Violations:** Buffer overflows, out-of-bounds array accesses, heap corruption, use-after-free, or unaligned memory access in the journal parser or telemetry ingestion.
- **Safety State Machine Bypasses:** Any state sequence allowing unmanaged charging while cell temperature exceeds 45°C or pack voltage exceeds 4.5V.
- **Fail-Safe Invariant Failures:** Failure to unsuppress charging upon USB disconnect or fuel gauge loss.
- **Filesystem / Storage Corruption:** Malicious or malformed journal payloads causing unbounded disk writes or file truncation on the microSD card.
- **Denial of Service:** Re-entrant deadlocks, infinite loops in background worker threads, or FreeRTOS task starvation.

*Note:* Standard display formatting quirks, spelling mistakes, or theoretical features are general issues, not security vulnerabilities; please report them via the public Issue Tracker.

---

## 4. How to Report a Vulnerability

If you discover a security vulnerability:

1. **Do NOT disclose the issue publicly** in an open GitHub issue, pull request, or public chat.
2. Please use GitHub's **Private Vulnerability Reporting** feature:
   - Navigate to the **Security** tab of the repository on GitHub.
   - Click **Report a vulnerability** to open an encrypted advisory draft.
3. If Private Vulnerability Reporting is unavailable, open a public issue titled `[SECURITY DISCLOSURE REQUEST]` requesting a private coordination channel **without** including exploit payloads, reproduction code, or technical vulnerability details in the issue text.

---

## 5. What Information to Include

Please provide:
- Affected component and commit hash / tag.
- Description of the root cause (e.g. parser integer overflow, mutex deadlock).
- Minimal reproducible test case (e.g. malformed binary journal file or test script).
- Severity and impact assessment.
- Remediation proposal (if available).

### What NOT to Disclose Publicly
- Do not publish functional exploit code or scripts designed to trigger hardware lockups or electrical faults before a patch is released and coordinated.

---

## 6. Response & Patch Coordination Timeline

- **Initial Response:** Within 48 hours.
- **Technical Triage & Reproduction:** Within 7 days.
- **Fix & Public Release:** Security advisories and patches are coordinated mutually prior to public disclosure.
