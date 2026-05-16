#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--version", required=True)
    parser.add_argument("--platform", required=True)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    stage_root = (repo_root / args.stage_root).resolve() if not args.stage_root.is_absolute() else args.stage_root.resolve()
    output_dir = (repo_root / args.output_dir).resolve() if not args.output_dir.is_absolute() else args.output_dir.resolve()

    bundle_name = f"ChowDSP-SC-Ports-{args.version}-{args.platform}"
    bundle_dir = output_dir / bundle_name
    extensions_dir = bundle_dir / "Extensions"

    if bundle_dir.exists():
        shutil.rmtree(bundle_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    extensions_dir.parent.mkdir(parents=True, exist_ok=True)

    shutil.copytree(stage_root, extensions_dir)
    for doc_name in ["README.md", "LICENSE", "NOTICE.md"]:
        shutil.copy2(repo_root / doc_name, bundle_dir / doc_name)

    archive_base = output_dir / bundle_name
    archive_path = shutil.make_archive(str(archive_base), "zip", root_dir=output_dir, base_dir=bundle_name)
    print(f"Created archive: {archive_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
