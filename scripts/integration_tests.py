#!/usr/bin/env python3

import argparse
import datetime
import os
import shutil
import subprocess
import sys
import time

from pathlib import Path

parser = argparse.ArgumentParser(description="OpenMW integration tests.")
parser.add_argument(
    "example_suite",
    type=str,
    help="path to openmw example suite (use 'git clone https://gitlab.com/OpenMW/example-suite/' to get it)",
)
parser.add_argument("--omw", type=str, default="openmw", help="path to openmw binary")
parser.add_argument(
    "--workdir", type=str, default="integration_tests_output", help="directory for temporary files and logs"
)
parser.add_argument("--verbose", action='store_true', help="print all openmw output")
args = parser.parse_args()

example_suite_dir = Path(args.example_suite).resolve()

content_paths = (
    example_suite_dir / "game_template" / "data" / "template.omwgame",
    example_suite_dir / "example_animated_creature" / "data" / "landracer.omwaddon",
    example_suite_dir / "the_hub" / "data" / "the_hub.omwaddon",
)
for path in content_paths:
    if not path.is_file():
        sys.exit(f"{path} is not found, use 'git clone https://gitlab.com/OpenMW/example-suite/' to get it")

openmw_binary = Path(args.omw).resolve()
if not openmw_binary.is_file():
    sys.exit(f"{openmw_binary} not found")

work_dir = Path(args.workdir).resolve()
work_dir.mkdir(parents=True, exist_ok=True)
config_dir = work_dir / "config"
userdata_dir = work_dir / "userdata"
tests_dir = Path(__file__).resolve().parent / "data" / "integration_tests"
testing_util_dir = tests_dir / "testing_util"
time_str = datetime.datetime.now().strftime("%Y-%m-%d-%H.%M.%S")


def run_test(test_name):
    start = time.time()
    print(f'[----------] Running tests from {test_name}')
    shutil.rmtree(config_dir, ignore_errors=True)
    config_dir.mkdir()
    shutil.copyfile(example_suite_dir / "settings.cfg", config_dir / "settings.cfg")
    test_dir = tests_dir / test_name
    with open(config_dir / "openmw.cfg", "w", encoding="utf-8") as omw_cfg:
        for path in content_paths:
            omw_cfg.write(f'data="{path.parent}"\n')
        omw_cfg.writelines(
            (
                f'data="{testing_util_dir}"\n',
                f'data-local="{test_dir}"\n',
                f'user-data="{userdata_dir}"\n',
            )
        )
        for path in content_paths:
            omw_cfg.write(f'content={path.name}\n')
        with open(test_dir / "openmw.cfg") as stream:
            omw_cfg.write(stream.read())
    with open(config_dir / "settings.cfg", "a", encoding="utf-8") as settings_cfg:
        settings_cfg.write(
            "[Video]\n"
            "resolution x = 640\n"
            "resolution y = 480\n"
            "framerate limit = 60\n"
            "[Game]\n"
            "smooth animation transitions = true\n"
            "[Lua]\n"
            f"memory limit = {1024 * 1024 * 256}\n"
        )
    stdout_lines = list()
    test_success = True
    fatal_errors = list()
    with subprocess.Popen(
        [openmw_binary, "--replace=config", "--config", config_dir, "--skip-menu", "--no-grab", "--no-sound"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        env={
            "OPENMW_OSG_STATS_FILE": str(work_dir / f"{test_name}.{time_str}.osg_stats.log"),
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
            elif "TEST_START" in line:
                test_start = time.time()
                number, name = line.split("TEST_START")[1].strip().split("\t", maxsplit=1)
                running_test_number = int(number)
                running_test_name = name
                count += 1
                print(f"[ RUN      ] {running_test_name}")
            elif "TEST_OK" in line:
                duration = (time.time() - test_start) * 1000
                number, name = line.split("TEST_OK")[1].strip().split("\t", maxsplit=1)
                assert running_test_number == int(number)
                print(f"[       OK ] {running_test_name} ({duration:.3f} ms)")
            elif "TEST_FAILED" in line:
                duration = (time.time() - test_start) * 1000
                number, name, error = line.split("TEST_FAILED")[1].strip().split("\t", maxsplit=2)
                assert running_test_number == int(number)
                print(error)
                print(f"[  FAILED  ] {running_test_name} ({duration:.3f} ms)")
                failed_tests.append(running_test_name)
        process.wait(5)
        if not quit_requested:
            fatal_errors.append("unexpected termination")
        if process.returncode != 0:
            fatal_errors.append(f"openmw exited with code {process.returncode}")
    if os.path.exists(config_dir / "openmw.log"):
        shutil.copyfile(config_dir / "openmw.log", work_dir / f"{test_name}.{time_str}.log")
    if fatal_errors and not args.verbose:
        sys.stdout.writelines(stdout_lines)
    total_duration = (time.time() - start) * 1000
    print(f'\n[----------] {count} tests from {test_name} ({total_duration:.3f} ms total)')
    print(f"[  PASSED  ] {count - len(failed_tests)} tests.")
    if fatal_errors:
        print(f"[  FAILED  ] fatal error: {'; '.join(fatal_errors)}")
    if failed_tests:
        print(f"[  FAILED  ] {len(failed_tests)} tests, listed below:")
        for failed_test in failed_tests:
            print(f"[  FAILED  ] {failed_test}")
    return len(failed_tests) == 0 and not fatal_errors


status = 0
for entry in tests_dir.glob("test_*"):
    if entry.is_dir():
        if not run_test(entry.name):
            status = -1
if status == 0:
    shutil.rmtree(config_dir, ignore_errors=True)
    shutil.rmtree(userdata_dir, ignore_errors=True)
exit(status)
