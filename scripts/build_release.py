#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path


def run(cmd: list[str], cwd: Path) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=cwd, check=True)


def discover_ports(ports_root: Path) -> list[Path]:
    ports: list[Path] = []
    for child in sorted(ports_root.iterdir()):
        if not child.is_dir():
            continue
        if child.name in {"shared", "tests"}:
            continue
        if (child / "CMakeLists.txt").exists():
            ports.append(child)
    return ports


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sc-path", required=True, type=Path)
    parser.add_argument("--ports-root", default=Path("ports"), type=Path)
    parser.add_argument("--build-root", required=True, type=Path)
    parser.add_argument("--stage-root", required=True, type=Path)
    parser.add_argument("--cmake-generator")
    parser.add_argument("--cmake-arch")
    parser.add_argument("--cmake-osx-architectures")
    parser.add_argument("--cmake-extra-arg", action="append", default=[])
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    sc_path = (repo_root / args.sc_path).resolve() if not args.sc_path.is_absolute() else args.sc_path.resolve()
    ports_root = (repo_root / args.ports_root).resolve() if not args.ports_root.is_absolute() else args.ports_root.resolve()
    build_root = (repo_root / args.build_root).resolve() if not args.build_root.is_absolute() else args.build_root.resolve()
    stage_root = (repo_root / args.stage_root).resolve() if not args.stage_root.is_absolute() else args.stage_root.resolve()

    if not sc_path.exists():
        raise SystemExit(f"SuperCollider path does not exist: {sc_path}")

    ports = discover_ports(ports_root)
    if not ports:
        raise SystemExit(f"No ports found under {ports_root}")

    if build_root.exists():
        shutil.rmtree(build_root)
    if stage_root.exists():
        shutil.rmtree(stage_root)
    build_root.mkdir(parents=True, exist_ok=True)
    stage_root.mkdir(parents=True, exist_ok=True)

    for port_dir in ports:
        build_dir = build_root / port_dir.name
        configure_cmd = [
            "cmake",
            "-S",
            str(port_dir),
            "-B",
            str(build_dir),
            f"-DSC_PATH={sc_path}",
            "-DCMAKE_BUILD_TYPE=Release",
        ]
        if args.cmake_generator:
            configure_cmd.extend(["-G", args.cmake_generator])
        if args.cmake_arch:
            configure_cmd.extend(["-A", args.cmake_arch])
        if args.cmake_osx_architectures:
            configure_cmd.append(f"-DCMAKE_OSX_ARCHITECTURES={args.cmake_osx_architectures}")
        configure_cmd.extend(args.cmake_extra_arg)
        if os.name == "nt":
            configure_cmd.append("-DCMAKE_CXX_FLAGS=/D_USE_MATH_DEFINES")

        run(
            configure_cmd,
            cwd=repo_root,
        )
        run(["cmake", "--build", str(build_dir), "--config", "Release"], cwd=repo_root)
        run(["cmake", "--install", str(build_dir), "--config", "Release", "--prefix", str(stage_root)], cwd=repo_root)

    print(f"Built {len(ports)} ports into {stage_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
