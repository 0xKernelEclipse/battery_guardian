# Security

The relevant risks in this app are malformed journal data, invalid telemetry, buffer bounds, unexpected policy transitions, and resource exhaustion from persistent logging.

The host tests exercise CRC failures, truncated records, unsupported journal versions, oversized payloads, invalid numeric values, and policy edge cases. They do not replace review or hardware testing.

To report a security problem, open a private GitHub security advisory for the repository. Do not publish an exploit for a hardware-control issue before the fix has been discussed.
