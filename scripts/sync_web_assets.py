Import("env")

import os
from pathlib import Path
import shutil
import subprocess
import sys

from SCons.Script import COMMAND_LINE_TARGETS


PROJECT_DIR = Path(env["PROJECT_DIR"])
DATA_DIR = PROJECT_DIR / "data"

WEB_TARGETS = {"buildfs", "uploadfs", "upload", "deploy"}
WEB_DIR_CANDIDATES = ["interface", "assarte"]
SKIP_WEB_BUILD_ENV = "ASSARTE_SKIP_WEB_BUILD"
SKIP_AUTO_UPLOADFS_ENV = "ASSARTE_SKIP_AUTO_UPLOADFS"


def find_web_dir() -> Path:
    for candidate in WEB_DIR_CANDIDATES:
        candidate_dir = PROJECT_DIR / candidate
        if candidate_dir.exists():
            return candidate_dir

    raise RuntimeError(
        "Could not find the frontend directory. Expected one of: "
        + ", ".join(str(PROJECT_DIR / candidate) for candidate in WEB_DIR_CANDIDATES)
    )


WEB_DIR = find_web_dir()
WEB_BUILD_DIR = WEB_DIR / "build"
WEB_NODE_MODULES_DIR = WEB_DIR / "node_modules"


def find_pnpm() -> str:
    pnpm = shutil.which("pnpm")
    if pnpm:
        return pnpm

    raise RuntimeError(
        "pnpm is required to build the Svelte frontend automatically. Install pnpm or build the frontend manually."
    )


def find_platformio() -> list[str]:
    pio = shutil.which("pio")
    if pio:
        return [pio]

    return [sys.executable, "-m", "platformio"]


def run_command(
    args: list[str], cwd: Path, description: str, extra_env: dict[str, str] | None = None
) -> None:
    print(description)
    command_env = os.environ.copy()
    if extra_env:
        command_env.update(extra_env)

    completed = subprocess.run(args, cwd=cwd, env=command_env)
    if completed.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(args)}")


def ensure_web_dependencies(pnpm: str) -> None:
    if WEB_NODE_MODULES_DIR.exists():
        return

    run_command([pnpm, "install", "--frozen-lockfile"], WEB_DIR, "Installing frontend dependencies")


def build_web_assets() -> None:
    pnpm = find_pnpm()
    ensure_web_dependencies(pnpm)
    run_command([pnpm, "build"], WEB_DIR, "Building frontend")


def sync_tree(src: Path, dst: Path) -> None:
    if dst.exists():
        shutil.rmtree(dst)

    dst.mkdir(parents=True, exist_ok=True)

    for item in src.iterdir():
        target = dst / item.name
        if item.is_dir():
            shutil.copytree(item, target)
        else:
            shutil.copy2(item, target)


def prepare_filesystem_assets() -> None:
    build_web_assets()

    if not WEB_BUILD_DIR.exists():
        raise RuntimeError(f"Frontend build did not produce {WEB_BUILD_DIR}")

    print(f"Syncing web assets from {WEB_BUILD_DIR} into data/")
    sync_tree(WEB_BUILD_DIR, DATA_DIR)


requested_targets = set(COMMAND_LINE_TARGETS)

if requested_targets & WEB_TARGETS and os.environ.get(SKIP_WEB_BUILD_ENV) != "1":
    prepare_filesystem_assets()


def uploadfs_after_upload(*_args, **_kwargs) -> None:
    if os.environ.get(SKIP_AUTO_UPLOADFS_ENV) == "1":
        return

    platformio = find_platformio()
    env_name = env["PIOENV"]

    run_command(
        [*platformio, "run", "-e", env_name, "-t", "uploadfs"],
        PROJECT_DIR,
        "Uploading LittleFS image",
        extra_env={
            SKIP_WEB_BUILD_ENV: "1",
            SKIP_AUTO_UPLOADFS_ENV: "1",
        },
    )


def deploy_action(*_args, **_kwargs) -> None:
    platformio = find_platformio()
    env_name = env["PIOENV"]

    run_command(
        [*platformio, "run", "-e", env_name, "-t", "upload"],
        PROJECT_DIR,
        "Uploading firmware",
    )


if "upload" in requested_targets and "uploadfs" not in requested_targets:
    env.AddPostAction("upload", uploadfs_after_upload)


env.AddCustomTarget(
    name="deploy",
    dependencies=None,
    actions=[deploy_action],
    title="Deploy",
    description="Build the frontend and run the standard upload flow",
)
