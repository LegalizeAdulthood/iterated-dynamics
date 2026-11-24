# SPDX-License-Identifier: GPL-3.0-only
# Copyright 2026 Richard Thomson.

"""Prepare a CMake instrumentation query and preserve its collected timings."""

import argparse
import csv
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys


def prepare(build_dir):
    query_dir = build_dir.resolve() / ".cmake" / "instrumentation" / "v1" / "query"
    query_dir.mkdir(parents=True, exist_ok=True)
    callback = [sys.executable, str(Path(__file__).resolve()), "collect"]
    command = subprocess.list2cmdline(callback) if os.name == "nt" else shlex.join(callback)
    query = {
        "version": 1,
        "callbacks": [command],
        "options": ["trace", "staticSystemInformation", "dynamicSystemInformation"],
    }
    # Collect explicitly after the workflow, including when a step fails.
    (query_dir / "timings.json").write_text(json.dumps(query, indent=2) + "\n", encoding="utf-8")


def collect(index_file):
    index = json.loads(index_file.read_text(encoding="utf-8"))
    data_dir = Path(index["dataDir"])
    commands = [
        json.loads((data_dir / snippet).read_text(encoding="utf-8"))
        for snippet in index["snippets"]
    ]
    if not commands:
        return

    commands.sort(key=lambda item: (-item["duration"], item["timeStart"], item["role"]))
    output_dir = Path(index["buildDir"]) / "cmake-instrumentation"
    output_dir.mkdir(parents=True, exist_ok=True)
    with (output_dir / "commands.csv").open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output)
        writer.writerow([
            "duration_ms", "role", "target", "source", "outputs", "time_start_ms",
            "result", "working_directory", "command", "before_host_memory_used_kib",
            "after_host_memory_used_kib", "before_cpu_load", "after_cpu_load",
        ])
        for command in commands:
            system = command.get("dynamicSystemInformation", {})
            writer.writerow([
                command["duration"],
                command["role"],
                command.get("target", ""),
                command.get("source", ""),
                json.dumps(command.get("outputs", [])),
                command["timeStart"],
                command.get("result"),
                command["workingDir"],
                command.get("command", ""),
                system.get("beforeHostMemoryUsed"),
                system.get("afterHostMemoryUsed"),
                system.get("beforeCPULoadAverage"),
                system.get("afterCPULoadAverage"),
            ])
    # CMake deletes the snippets after this callback; preserve its trace too.
    shutil.copyfile(data_dir / index["trace"], output_dir / "trace.json")
    print("Saved {} command timings and trace to {}".format(len(commands), output_dir))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("prepare", "collect"))
    parser.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.operation == "prepare":
        prepare(args.path)
    else:
        collect(args.path)


if __name__ == "__main__":
    main()
