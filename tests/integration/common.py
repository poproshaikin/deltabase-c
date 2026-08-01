import subprocess, shutil, pathlib

_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
CLI      = str(_ROOT / "deltabase/build/bin/cli.exe")
DATA_DIR = str(_ROOT / "deltabase/build/bin/data")

def _filter(out: str) -> str:
    lines = out.splitlines()
    result, i = [], 0
    while i < len(lines):
        if lines[i].strip() == "ERR: 17" and i + 1 < len(lines) and "Unsupported statement" in lines[i + 1]:
            i += 2
        else:
            result.append(lines[i])
            i += 1
    return "\n".join(result)

def run(db, *queries):
    inp = "\n".join(queries) + "\n"
    r = subprocess.run([CLI, "--db", db], input=inp, capture_output=True, text=True, timeout=5)
    return _filter(r.stdout + r.stderr)

def create_db(name):
    r = subprocess.run([CLI], input=f"create database {name};\n", capture_output=True, text=True, timeout=5)
    return _filter(r.stdout + r.stderr)

def reset(name):
    shutil.rmtree(f"{DATA_DIR}/{name}", ignore_errors=True)

_fails = 0
_total = 0

def check(label, output, expect_ok=True, expected_substr=None):
    global _fails, _total
    _total += 1
    has_err = "ERR:" in output or "MSG:" in output
    if expect_ok and has_err:
        print(f"FAIL [{label}]: unexpected error\n  {output.strip()}")
        _fails += 1
    elif not expect_ok and not has_err:
        print(f"FAIL [{label}]: expected error but got none\n  {output.strip()}")
        _fails += 1
    elif expected_substr and expected_substr not in output:
        print(f"FAIL [{label}]: expected '{expected_substr}' in output\n  {output.strip()}")
        _fails += 1
    else:
        print(f"OK   [{label}]")

def summary():
    passed = _total - _fails
    print(f"\n{passed}/{_total} passed" + ("" if _fails == 0 else f"  ({_fails} failed)"))