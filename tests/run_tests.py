import subprocess
import sys
import os
import shutil
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent

def find_compiler():
    # 1. Check CC environment variable
    env_cc = os.environ.get("CC")
    if env_cc:
        return env_cc, False
    
    # 2. Check local tools directory (preferring complete zig013 distribution)
    repo_root = BASE_DIR.parent
    for p in repo_root.glob("tools/zig013/**/zig.exe"):
        return str(p), True
    for p in repo_root.glob("tools/**/zig.exe"):
        return str(p), True
    for p in repo_root.glob("tools/**/zig"):
        return str(p), True

    # 3. Check for zig on PATH
    zig_on_path = shutil.which("zig")
    if zig_on_path:
        return zig_on_path, True

    # 4. Check for standard C compilers on PATH
    for name in ["clang", "gcc"]:
        p = shutil.which(name)
        if p:
            return p, False
            
    return None, False

def main():
    compiler, is_zig = find_compiler()
    if not compiler:
        print("Error: Suitable C compiler (zig, clang, gcc) not found.")
        print("Ensure 'zig', 'clang', or 'gcc' is in your PATH or CC environment variable.")
        sys.exit(1)

    print(f"Using compiler: {compiler}")
    
    build_dir = BASE_DIR / "build"
    build_dir.mkdir(exist_ok=True)
    
    exe_name = "run_tests.exe" if os.name == "nt" else "run_tests"
    exe_out = str(build_dir / exe_name)
    
    cmd = [compiler]
    if is_zig:
        cmd.append("cc")
        
    cmd.extend([
        "-I", str(BASE_DIR / "tests" / "mocks"),
        "-I", str(BASE_DIR / "core"),
        "-I", str(BASE_DIR / "storage"),
        "-I", str(BASE_DIR / "phase2"),
        "-DBG_HOST_TEST",
        # Core pipeline
        str(BASE_DIR / "core" / "telemetry.c"),
        str(BASE_DIR / "core" / "telemetry_adapter.c"),
        str(BASE_DIR / "core" / "session.c"),
        str(BASE_DIR / "core" / "event.c"),
        str(BASE_DIR / "core" / "diagnostics.c"),
        str(BASE_DIR / "storage" / "journal.c"),
        # Phase 2A intelligence engines
        str(BASE_DIR / "phase2" / "estimator.c"),
        str(BASE_DIR / "phase2" / "confidence.c"),
        str(BASE_DIR / "phase2" / "degradation.c"),
        str(BASE_DIR / "phase2" / "battery_health.c"),
        # Phase 2B charge control
        str(BASE_DIR / "core" / "charger_hal.c"),
        str(BASE_DIR / "phase2" / "charge_policy.c"),
        # Phase 1 tests
        str(BASE_DIR / "tests" / "telemetry" / "test_validation.c"),
        str(BASE_DIR / "tests" / "telemetry" / "test_sampling.c"),
        str(BASE_DIR / "tests" / "telemetry" / "test_buffer.c"),
        str(BASE_DIR / "tests" / "journal" / "test_records.c"),
        str(BASE_DIR / "tests" / "journal" / "test_crc.c"),
        str(BASE_DIR / "tests" / "journal" / "test_recovery.c"),
        str(BASE_DIR / "tests" / "session" / "test_state_machine.c"),
        str(BASE_DIR / "tests" / "session" / "test_metrics.c"),
        str(BASE_DIR / "tests" / "event" / "test_generation.c"),
        str(BASE_DIR / "tests" / "integration" / "test_pipeline.c"),
        # Phase 2A tests
        str(BASE_DIR / "tests" / "phase2" / "test_phase2.c"),
        # Simulation harness
        str(BASE_DIR / "tests" / "simulation" / "trace.c"),
        str(BASE_DIR / "tests" / "simulation" / "simulator.c"),
        str(BASE_DIR / "tests" / "simulation" / "test_simulation.c"),
        str(BASE_DIR / "tests" / "simulation" / "test_charge_policy.c"),
        # Phase 3 Hardening and Regression tests
        str(BASE_DIR / "tests" / "hardening" / "test_journal_hardening.c"),
        str(BASE_DIR / "tests" / "hardening" / "test_hostile_telemetry.c"),
        str(BASE_DIR / "tests" / "regression" / "test_regressions.c"),
        # Phase 4 Platform Adapter & Hardware Abstraction
        str(BASE_DIR / "tests" / "platform" / "test_platform_adapter.c"),
        # Phase 5 Security Audit & Adversarial Persistence
        str(BASE_DIR / "tests" / "hardening" / "test_security_audit.c"),
        # Entry point
        str(BASE_DIR / "tests" / "test_main.c"),
        "-o", exe_out
    ])
    
    print("Compiling test suite...")
    res = subprocess.run(cmd)
    if res.returncode != 0:
        print("Compilation failed!")
        sys.exit(res.returncode)

    print("\nRunning test suite...")
    res = subprocess.run([exe_out])
    sys.exit(res.returncode)

if __name__ == "__main__":
    main()
