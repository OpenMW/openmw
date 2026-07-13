#!/usr/bin/env python3

import argparse
import datetime
import os
import shutil
import subprocess
import sys
import time

from pathlib import Path

def parse_args():
    parser = argparse.ArgumentParser(description="OpenMW integration tests.")
    parser.add_argument(
        "suites",
        nargs='*',
        type=str,
        default=None,
        help="suites to run: a directory with suites or subdirectories containing them",
    )
    parser.add_argument("--omw", type=str, default="openmw", help="path to openmw binary")
    parser.add_argument(
        "--workdir", type=str, default="integration_tests_output", help="directory for temporary files and logs"
    )
    parser.add_argument("--verbose", action='store_true', help="print all openmw output")
    parser.add_argument(
        "--test_filter", type=str, default=None,
        help="test cases to run (e.g. 'player.*running')",
    )
    parser.add_argument(
        "--list_tests", action='store_true',
        help="print test names instead of running them",
    )
    parser.add_argument(
        "--config", type=str, default=None,
        help="extra directory with custom settings.cfg",
    )
    parser.add_argument(
        "--data", action='append', default=[],
        help="extra data directories with content files",
    )
    parser.add_argument(
        "--ini", type=str, default=None,
        help="INI file to import fallback values from via openmw-iniimporter (found next to --omw)",
    )
    return parser.parse_args()


def lua_string_literal(value):
    escaped = value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return f'"{escaped}"'


def discover_suites(suite_roots):
    all_suites = dict()
    for root in suite_roots:
        if not root.is_dir():
            sys.exit(f"{root} is not a directory")
        if (root / "openmw.cfg").is_file():
            all_suites[root.name] = root
        else:
            for entry in sorted(root.glob("test_*")):
                if entry.is_dir() and (entry / "openmw.cfg").is_file():
                    all_suites[entry.name] = entry
    if not all_suites:
        sys.exit(f"No suites found in: {', '.join(str(v) for v in suite_roots)}")
    return all_suites


def write_test_config_module(test_config_dir, test_filter, list_tests):
    shutil.rmtree(test_config_dir, ignore_errors=True)
    test_config_dir.mkdir(parents=True)
    fields = []
    if test_filter:
        fields.append(f"filter = {lua_string_literal(test_filter)}")
    if list_tests:
        fields.append("list = true")
    with open(test_config_dir / "test_config.lua", "w", encoding="utf-8") as stream:
        stream.write(f"return {{ {', '.join(fields)} }}\n")


def parse_test_name(status, line):
    return line.split(status)[1].strip().split("\t", maxsplit=1)


def write_generated_config(
    config_dir, userdata_dir, data_local_dir, test_config_dir, config_source_dir, args, iniimporter_binary, ini_path
):
    shutil.rmtree(config_dir, ignore_errors=True)
    config_dir.mkdir(parents=True)
    base_cfg_path = config_dir / ("openmw.base.cfg" if args.ini else "openmw.cfg")
    with open(base_cfg_path, "w", encoding="utf-8") as omw_cfg:
        omw_cfg.write(f'user-data="{userdata_dir}"\n')
        omw_cfg.write(f'data-local="{data_local_dir}"\n')
        omw_cfg.write(f'data="{test_config_dir}"\n')
    if args.ini:
        subprocess.run(
            [
                iniimporter_binary,
                "--ini", ini_path,
                "--cfg", base_cfg_path,
                "--output", config_dir / "openmw.cfg",
            ],
            check=True,
        )
    with open(config_dir / "settings.cfg", "w", encoding="utf-8") as settings_cfg:
        if config_source_dir is not None:
            source_settings = config_source_dir / "settings.cfg"
            if source_settings.is_file():
                settings_cfg.write(source_settings.read_text(encoding="utf-8"))
        settings_cfg.write(
            "[Video]\n"
            "resolution x = 640\n"
            "resolution y = 480\n"
            "framerate limit = 60\n"
            "[Game]\n"
            "smooth animation transitions = true\n"
            "[Lua]\n"
            f"memory limit = {1024 * 1024 * 256}\n"
            "lua profiler = true\n"
        )


def run_test(suite_name, suite_dir, config_dir, log_dir, time_str, data_dirs, args, openmw_binary):
    start = time.time()
    if not args.list_tests:
        print(f'[----------] Running tests from {suite_name}')
    command = [openmw_binary, "--replace=config", "--config", suite_dir, "--config", config_dir, "--no-grab"]
    for path in data_dirs:
        command += ["--data", path]
    stdout_lines = list()
    fatal_errors = list()
    with subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        env={
            "OPENMW_OSG_STATS_FILE": str(log_dir / f"{suite_name}.{time_str}.osg_stats.log"),
            "OPENMW_OSG_STATS_LIST": "times",
            **os.environ,
        },
    ) as process:
        quit_requested = False
        running_test_number = None
        running_test_name = None
        count = 0
        failed_tests = list()
        test_start = None
        for line in process.stdout:
            if args.verbose:
                sys.stdout.write(line)
            else:
                stdout_lines.append(line)
            if "Quit requested by a Lua script" in line:
                quit_requested = True
            elif "TEST_FOUND" in line:
                _, name = parse_test_name("TEST_FOUND", line)
                count += 1
                print(name)
            elif "TEST_START" in line:
                test_start = time.time()
                number, name = parse_test_name("TEST_START", line)
                running_test_number = int(number)
                running_test_name = name
                count += 1
                print(f"[ RUN      ] {running_test_name}")
            elif "TEST_OK" in line:
                duration = (time.time() - test_start) * 1000
                number, name = parse_test_name("TEST_OK", line)
                assert running_test_number == int(number)
                print(f"[       OK ] {running_test_name} ({duration:.3f} ms)")
            elif "TEST_FAILED" in line:
                duration = (time.time() - test_start) * 1000
                number, name, error = line.split("TEST_FAILED")[1].strip().split("\t", maxsplit=2)
                assert running_test_number == int(number)
                print(error)
                print(f"[  FAILED  ] {running_test_name} ({duration:.3f} ms)")
                failed_tests.append(running_test_name)
            elif "TEST_ERROR" in line:
                message = line.split("TEST_ERROR")[1].strip()
                fatal_errors.append(f"test framework error: {message}")
        process.wait(5)
        if not quit_requested:
            fatal_errors.append("unexpected termination")
        if process.returncode != 0:
            fatal_errors.append(f"openmw exited with code {process.returncode}")
    if os.path.exists(config_dir / "openmw.log"):
        shutil.copyfile(config_dir / "openmw.log", log_dir / f"{suite_name}.{time_str}.log")
    if fatal_errors and not args.verbose:
        sys.stdout.writelines(stdout_lines)
    if not args.list_tests:
        total_duration = (time.time() - start) * 1000
        print(f'\n[----------] {count} tests from {suite_name} ({total_duration:.3f} ms total)')
        print(f"[  PASSED  ] {count - len(failed_tests)} tests.")
    if fatal_errors:
        print(f"[  FAILED  ] fatal error: {'; '.join(fatal_errors)}")
    if failed_tests:
        print(f"[  FAILED  ] {len(failed_tests)} tests, listed below:")
        for failed_test in failed_tests:
            print(f"[  FAILED  ] {failed_test}")
    return len(failed_tests) == 0 and not fatal_errors, count


def main():
    args = parse_args()

    openmw_binary = Path(args.omw).resolve()
    if not openmw_binary.is_file():
        sys.exit(f"{openmw_binary} not found")

    iniimporter_binary = None
    ini_path = None
    if args.ini:
        ini_path = Path(args.ini).resolve()
        if not ini_path.is_file():
            sys.exit(f"{ini_path} not found")
        iniimporter_binary = openmw_binary.parent / f"openmw-iniimporter{openmw_binary.suffix}"
        if not iniimporter_binary.is_file():
            sys.exit(f"{iniimporter_binary} not found")

    config_source_dir = Path(args.config).resolve() if args.config is not None else None
    data_dirs = [Path(v).resolve() for v in args.data]

    work_dir = Path(args.workdir).resolve()
    work_dir.mkdir(parents=True, exist_ok=True)
    test_config_dir = work_dir / "test_config"
    time_str = datetime.datetime.now().strftime("%Y-%m-%d-%H.%M.%S")

    suite_roots = [Path(v).resolve() for v in args.suites]
    all_suites = discover_suites(suite_roots)

    write_test_config_module(test_config_dir=test_config_dir, test_filter=args.test_filter, list_tests=args.list_tests)

    status = 0
    total_count = 0
    for name in sorted(all_suites):
        suite_work_dir = work_dir / name
        write_generated_config(
            config_dir=suite_work_dir,
            userdata_dir=suite_work_dir / "userdata",
            data_local_dir=suite_work_dir / "data-local",
            test_config_dir=test_config_dir,
            config_source_dir=config_source_dir,
            args=args,
            iniimporter_binary=iniimporter_binary,
            ini_path=ini_path,
        )
        result, count = run_test(
            suite_name=name,
            suite_dir=all_suites[name],
            config_dir=suite_work_dir,
            log_dir=work_dir,
            time_str=time_str,
            data_dirs=data_dirs,
            args=args,
            openmw_binary=openmw_binary,
        )
        total_count += count
        if not result:
            status = -1
    if args.test_filter and total_count == 0:
        print(f"[  WARNING ] no tests matched --test_filter {args.test_filter!r}")
        status = -1
    exit(status)


if __name__ == "__main__":
    main()
