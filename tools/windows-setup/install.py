"""Install a verified Windows release using media from a local copy of Jedi Academy."""
from argparse import ArgumentParser
from datetime import datetime
from hashlib import file_digest
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import NamedTemporaryFile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "source"
SP = "JediAcademyRemixed"
MP = "JediAcademyRemixedMP"


def status(message):
    print("STATUS " + message, flush=True)


def digest(path):
    with path.open("rb") as stream:
        return file_digest(stream, "sha256").hexdigest()


def check_closed(game, setup):
    shell = Path(os.environ["SystemRoot"]) / "System32/WindowsPowerShell/v1.0/powershell.exe"
    command = ("[Console]::OutputEncoding=[Text.UTF8Encoding]::new($false); "
               "Get-Process | ForEach-Object { if ($_.Path) { $_.Path } }")
    result = subprocess.run([str(shell), "-NoProfile", "-NonInteractive", "-Command", command],
                            capture_output=True, encoding="utf-8", errors="replace", check=True)
    for value in result.stdout.splitlines():
        path = Path(value)
        if path in (Path(sys.executable), setup):
            continue
        if path.is_absolute() and path.resolve().is_relative_to(game):
            raise RuntimeError("Close " + path.name + " and run setup again.")


def builder(script, *arguments):
    # Embedded Python isolates the import path. Add only the packaged builders.
    start = ("import sys,runpy,os;sys.argv.pop(0);"
             "sys.path.insert(0,os.path.dirname(sys.argv[0]));"
             "runpy.run_path(sys.argv[0],run_name='__main__')")
    subprocess.run([sys.executable, "-X", "utf8", "-B", "-c", start, str(SOURCE / "scripts" / script),
                    *map(str, arguments)], check=True)


def generate(game, payload, outcast):
    sp, mp = payload / SP, payload / MP
    generated = SOURCE / "build"
    generated.mkdir(exist_ok=True)
    status("Preparing sandbox icons, vehicles and player models")
    builder("build-sandbox-icons.py", "--game-data", game, "--output", sp / "jar-sandbox-icons.pk3")
    shutil.copyfile(sp / "jar-sandbox-icons.pk3", mp / "jar-sandbox-icons.pk3")
    builder("build-yt1300.py", "--game-data", game)
    for folder in (sp, mp):
        shutil.copyfile(generated / "jar-yt1300.pk3", folder / "jar-yt1300.pk3")
    builder("build-mp-legacy-droids.py", "--game-data", game)
    droids = mp / "jar-mp-legacy-droids.pk3"
    shutil.copyfile(generated / droids.name, droids)
    companions = []
    if (outcast / "base/assets0.pk3").is_file():
        legacy = mp / "jar-legacy-npcs.pk3"
        builder("build-legacy-npcs.py", "--game-data", game, "--outcast-data", outcast,
                "--output", legacy)
        shutil.copyfile(legacy, sp / legacy.name)
        companions.extend(["--companion", legacy])
    builder("build-mp-player-models.py", "--game-data", game, "--output",
            mp / "jar-mp-player-models.pk3", *companions, "--companion", droids)
    status("Building interface textures and fonts from your game. This can take several minutes.")
    appearance = sp / "jar-appearance.pk3"
    builder("build-appearance.py", "--game-data", game, "--output", appearance)
    status("Preparing multiplayer interface and effects")
    builder("build-mp-interface.py", "--source", appearance, "--output", mp / "jar-interface.pk3")
    builder("build-mp-effects.py", "--source", appearance, "--game-data", game,
            "--output", mp / "jar-effects.pk3")


def replace_file(source, dest):
    dest.parent.mkdir(parents=True, exist_ok=True)
    with NamedTemporaryFile(prefix=".jar-setup-", dir=dest.parent, delete=False) as stream:
        temporary = Path(stream.name)
    try:
        shutil.copyfile(source, temporary)
        os.replace(temporary, dest)
    finally:
        temporary.unlink(missing_ok=True)


def deploy(payload, game, setup):
    status("Checking installed files")
    files = sorted(p.relative_to(payload) for p in payload.rglob("*") if p.is_file())
    changes = []
    backup = game / "JA-R Backups" / datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    for name in files:
        dest = game / name
        if dest.resolve() != dest or backup.resolve() != backup:
            raise ValueError("A destination is redirected outside its expected path: " + str(name))
        expected = digest(payload / name)
        previous = digest(dest) if dest.exists() else None
        if expected != previous:
            changes.append((name, expected, previous))
    check_closed(game, setup)
    for name, _, previous in changes:
        if previous:
            replace_file(game / name, backup / name)
            if digest(backup / name) != previous:
                raise IOError("Could not verify backup of " + str(name))
    status("Installing and verifying " + str(len(changes)) + " changed files")
    written = []
    try:
        for name, expected, previous in changes:
            dest = game / name
            replace_file(payload / name, dest)
            written.append((name, previous))
            if digest(dest) != expected:
                raise IOError("Could not verify " + str(name))
    except Exception:
        for name, previous in reversed(written):
            if previous:
                replace_file(backup / name, game / name)
            else:
                (game / name).unlink()
        raise
    print(f"Verified {len(files)} files; {len(changes)} changed.", flush=True)


def main():
    parser = ArgumentParser()
    parser.add_argument("game", type=Path)
    parser.add_argument("--setup-exe", type=Path)
    args = parser.parse_args()
    game = args.game.resolve(strict=True)
    for index in range(4):
        if not (game / "base" / f"assets{index}.pk3").is_file():
            raise ValueError("Choose the GameData folder containing base/assets0.pk3 through assets3.pk3.")
    check_closed(game, args.setup_exe)
    # Check folder access before expensive local conversions.
    try:
        with NamedTemporaryFile(dir=game, prefix=".jar-setup-"):
            pass
    except PermissionError as error:
        raise PermissionError("Windows denied access to this game folder. Close setup and run it as administrator.") from error
    payload = ROOT / "GameData"
    manifest = json.loads((ROOT / "native.json").read_text(encoding="utf-8"))
    actual = {p.relative_to(payload).as_posix(): digest(p) for p in payload.rglob("*") if p.is_file()}
    if manifest != actual:
        raise ValueError("Setup files failed verification. Download the installer again.")
    generate(game, payload, game.parent.parent / "Jedi Outcast/GameData")
    deploy(payload, game, args.setup_exe)


if __name__ == "__main__":
    try:
        main()
    except (Exception, KeyboardInterrupt) as error:
        print("ERROR " + str(error), file=sys.stderr, flush=True)
        sys.exit(1)
