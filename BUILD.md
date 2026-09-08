# Build

## Host tests

The test runner needs Python and a C compiler. It checks for Zig, Clang, or GCC on `PATH`; set `CC` to choose a compiler.

```bash
python tests/run_tests.py
```

These tests use mocks. They do not require a Flipper.

## FAP build

Install the Flipper build tool and SDK:

```bash
python -m pip install --upgrade ufbt
ufbt update
```

Build the application:

```bash
ufbt clean
ufbt
```

The FAP is written to `dist/battery_guardian.fap`. With a Flipper connected, `ufbt launch` builds, copies, and launches it.
