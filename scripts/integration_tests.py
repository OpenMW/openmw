#!/usr/bin/env python3

import argparse, datetime, os, subprocess, sys, shutil
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
example_suite_content = example_suite_dir / "game_template" / "data" / "template.omwgame"
if not example_suite_content.is_file():
    sys.exit(
        f"{example_suite_content} not found, use 'git clone https://gitlab.com/OpenMW/example-suite/' to get it"
    )

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


def runTest(name):
    print(f"Start {name}")
    shutil.rmtree(config_dir, ignore_errors=True)
    config_dir.mkdir()
    shutil.copyfile(example_suite_dir / "settings.cfg", config_dir / "settings.cfg")
    test_dir = tests_dir / name
    with open(config_dir / "openmw.cfg", "w", encoding="utf-8") as omw_cfg:
        omw_cfg.writelines(
            (
                f'data="{example_suite_dir}{os.sep}game_template{os.sep}data"\n',
                f'data="{testing_util_dir}"\n',
                f'data-local="{test_dir}"\n',
                f'user-data="{userdata_dir}"\n',
                "content=template.omwgame\n",
            )
        )
        if (test_dir / "openmw.cfg").exists():
            omw_cfg.write(open(test_dir / "openmw.cfg").read())
        elif (test_dir / "test.omwscripts").exists():
            omw_cfg.write("content=test.omwscripts\n")
    with open(config_dir / "settings.cfg", "a", encoding="utf-8") as settings_cfg:
        settings_cfg.write(
            "[Video]\n"
            "resolution x = 640\n"
            "resolution y = 480\n"
            "framerate limit = 60\n"
            "[Game]\n"
            "smooth animation transitions = true\n"
        )
    stdout_lines = list()
    exit_ok = True
    test_success = True
    with subprocess.Popen(
        [openmw_binary, "--replace=config", "--config", config_dir, "--skip-menu", "--no-grab", "--no-sound"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        env={
            "OPENMW_OSG_STATS_FILE": str(work_dir / f"{name}.{time_str}.osg_stats.log"),
            "OPENMW_OSG_STATS_LIST": "times",
            **os.environ,
        },
    ) as process:
        quit_requested = False
        for line in process.stdout:
            if args.verbose:
                sys.stdout.write(line)
            else:
                stdout_lines.append(line)
            words = line.split(" ")
            if len(words) > 1 and words[1] == "E]":
                print(line, end="")
            elif "Quit requested by a Lua script" in line:
                quit_requested = True
            elif "TEST_START" in line:
                w = line.split("TEST_START")[1].split("\t")
                print(f"TEST {w[2].strip()}\t\t", end="")
            elif "TEST_OK" in line:
                print(f"OK")
            elif "TEST_FAILED" in line:
                w = line.split("TEST_FAILED")[1].split("\t")
                print(f"FAILED {w[3]}\t\t")
                test_success = False
        process.wait(5)
        if not quit_requested:
            print("ERROR: Unexpected termination")
            exit_ok = False
        if process.returncode != 0:
            print(f"ERROR: openmw exited with code {process.returncode}")
            exit_ok = False
    if os.path.exists(config_dir / "openmw.log"):
        shutil.copyfile(config_dir / "openmw.log", work_dir / f"{name}.{time_str}.log")
    if not exit_ok and not args.verbose:
        sys.stdout.writelines(stdout_lines)
    if test_success and exit_ok:
        print(f"{name} succeeded")
    else:
        print(f"{name} failed")
    return test_success and exit_ok


status = 0
for entry in tests_dir.glob("test_*"):
    if entry.is_dir():
        if not runTest(entry.name):
            status = -1
shutil.rmtree(config_dir, ignore_errors=True)
shutil.rmtree(userdata_dir, ignore_errors=True)
exit(status)
