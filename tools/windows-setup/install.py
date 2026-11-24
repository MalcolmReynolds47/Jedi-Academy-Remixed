"""Install a verified Windows release using media from a local copy of Jedi Academy."""
from argparse import ArgumentParser
import ctypes
from ctypes import wintypes
from datetime import datetime
from hashlib import file_digest, sha256
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import NamedTemporaryFile, TemporaryDirectory
import winreg
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "source"
SP = "JediAcademyRemixed"
MP = "JediAcademyRemixedMP"
RECEIPT = Path(SP) / "setup-media.json"


def status(message):
    print("STATUS " + message, flush=True)


def digest(path):
    with path.open("rb") as stream:
        return file_digest(stream, "sha256").hexdigest()


def check_closed(game, setup):
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    enum = kernel.K32EnumProcesses
    enum.argtypes = (ctypes.POINTER(wintypes.DWORD), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD))
    kernel.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.QueryFullProcessImageNameW.argtypes = (wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR,
                                                  ctypes.POINTER(wintypes.DWORD))
    kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
    count = 512
    while True:
        pids = (wintypes.DWORD * count)()
        returned = wintypes.DWORD()
        if not enum(pids, ctypes.sizeof(pids), ctypes.byref(returned)):
            raise ctypes.WinError(ctypes.get_last_error())
        if returned.value < ctypes.sizeof(pids):
            break
        count *= 2
    image = ctypes.create_unicode_buffer(32768)
    for pid in pids[:returned.value // ctypes.sizeof(wintypes.DWORD)]:
        handle = kernel.OpenProcess(0x1000, False, pid)  # PROCESS_QUERY_LIMITED_INFORMATION
        if not handle:
            continue
        try:
            size = wintypes.DWORD(len(image))
            if kernel.QueryFullProcessImageNameW(handle, 0, image, ctypes.byref(size)):
                path = Path(image.value).resolve()
                if path.is_relative_to(game) and path not in (Path(sys.executable), setup):
                    raise RuntimeError("Close " + path.name + " and run setup again.")
        finally:
            kernel.CloseHandle(handle)


def builder(source, script, *arguments):
    # Embedded Python isolates the import path. Add only the packaged builders.
    start = ("import sys,runpy,os;sys.argv.pop(0);"
             "sys.path.insert(0,os.path.dirname(sys.argv[0]));"
             "runpy.run_path(sys.argv[0],run_name='__main__')")
    subprocess.run([sys.executable, "-X", "utf8", "-B", "-c", start, str(source / "scripts" / script),
                    *map(str, arguments)], check=True)


def generate(game, payload, outcast, source):
    sp, mp = payload / SP, payload / MP
    generated = source / "build"
    generated.mkdir(exist_ok=True)
    status("Preparing sandbox icons, vehicles and player models")
    builder(source, "build-sandbox-icons.py", "--game-data", game, "--output", sp / "jar-sandbox-icons.pk3")
    shutil.copyfile(sp / "jar-sandbox-icons.pk3", mp / "jar-sandbox-icons.pk3")
    builder(source, "build-yt1300.py", "--game-data", game)
    for folder in (sp, mp):
        shutil.copyfile(generated / "jar-yt1300.pk3", folder / "jar-yt1300.pk3")
    builder(source, "build-mp-legacy-droids.py", "--game-data", game)
    droids = mp / "jar-mp-legacy-droids.pk3"
    shutil.copyfile(generated / droids.name, droids)
    companions = []
    if (outcast / "base/assets0.pk3").is_file():
        legacy = mp / "jar-legacy-npcs.pk3"
        builder(source, "build-legacy-npcs.py", "--game-data", game, "--outcast-data", outcast,
                "--output", legacy)
        shutil.copyfile(legacy, sp / legacy.name)
        companions.extend(["--companion", legacy])
    builder(source, "build-mp-player-models.py", "--game-data", game, "--output",
            mp / "jar-mp-player-models.pk3", *companions, "--companion", droids)
    status("Building interface textures and fonts from your game. This can take several minutes.")
    appearance = sp / "jar-appearance.pk3"
    builder(source, "build-appearance.py", "--game-data", game, "--output", appearance)
    status("Preparing multiplayer interface and effects")
    builder(source, "build-mp-interface.py", "--source", appearance, "--output", mp / "jar-interface.pk3")
    builder(source, "build-mp-effects.py", "--source", appearance, "--game-data", game,
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


def game_data(path):
    for folder in (path, path / "GameData"):
        if all((folder / "base" / f"assets{i}.pk3").is_file() for i in range(4)):
            return folder.resolve()
    return None


def choose_game():
    steam = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Steam"
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
            steam = Path(winreg.QueryValueEx(key, "SteamPath")[0])
    except OSError:
        pass
    libraries = [steam]
    manifest = steam / "steamapps/libraryfolders.vdf"
    if manifest.is_file():
        for value in re.findall(r'"path"\s+"([^"]+)"', manifest.read_text(encoding="utf-8-sig")):
            libraries.append(Path(value.replace("\\\\", "\\")))
    for library in libraries:
        found = game_data(library / "steamapps/common/Jedi Academy")
        if found:
            print(f"Jedi Academy found in {found}")
            choice = input("Press Enter to install here, or paste a different game folder: ").strip().strip('"')
            return Path(choice) if choice else found
    return Path(input("Paste your installed Jedi Academy or GameData folder: ").strip().strip('"'))


def media_names(outcast):
    shared = ["jar-sandbox-icons", "jar-yt1300"]
    if (outcast / "base/assets0.pk3").is_file():
        shared.append("jar-legacy-npcs")
    return sorted(f"{folder}/{name}.pk3" for folder, names in [
        (SP, shared + ["jar-appearance"]),
        (MP, shared + ["jar-interface", "jar-effects", "jar-mp-legacy-droids", "jar-mp-player-models"])
    ] for name in names)


def media_key(game, outcast):
    from PIL import __version__ as pillow
    recipe = sha256((sys.version + pillow).encode())
    for path in sorted(p for p in SOURCE.rglob("*") if p.is_file()):
        recipe.update(path.relative_to(SOURCE).as_posix().encode() + bytes.fromhex(digest(path)))
    # ZIP entry CRC, size, name and precedence describe the media read by the builders.
    for group, paths in (
        ("academy", sorted((game / "base").glob("assets*.pk3"))),
        ("outcast", sorted((outcast / "base").glob("assets*.pk3"))),
        ("mbii", [game / "MBII" / name for name in ("MBAssets.pk3", "MBAssets2.pk3", "MBAssets3.pk3",
                 "MBHilts.pk3", "MB_Effects.pk3", "z_MB_BaseAssets.pk3", "zzzzzzzzzSaber_GFX.pk3")]),
    ):
        for path in paths:
            if path.is_file():
                recipe.update(json.dumps([group, path.name]).encode())
                with ZipFile(path) as archive:
                    for entry in archive.infolist():
                        recipe.update(json.dumps([entry.filename, entry.CRC, entry.file_size]).encode())
    return recipe.hexdigest()


def reuse_media(game, key, names):
    try:
        receipt = json.loads((game / RECEIPT).read_text(encoding="utf-8"))
        if receipt["recipe"] != key or sorted(receipt["files"]) != names:
            return False
        return all((game / name).resolve() == game / name and digest(game / name) == receipt["files"][name]
                   for name in names)
    except (OSError, TypeError, ValueError, KeyError, AttributeError):
        return False


def install(game, setup):
    check_closed(game, setup)
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
    outcast = game.parent.parent / "Jedi Outcast/GameData"
    key = media_key(game, outcast)
    names = media_names(outcast)
    if reuse_media(game, key, names):
        status("Existing media verified. Skipping texture and model generation.")
        deploy(payload, game, setup)
        return
    # Work on disposable copies so a ZIP can be extracted once and safely reused.
    with TemporaryDirectory(prefix="JAR-media-") as work:
        stage = Path(work) / "GameData"
        source = Path(work) / "source"
        shutil.copytree(payload, stage)
        shutil.copytree(SOURCE, source)
        generate(game, stage, outcast, source)
        files = {name: digest(stage / name) for name in names}
        (stage / RECEIPT).write_text(json.dumps({"recipe": key, "files": files}, indent=2), encoding="utf-8")
        deploy(stage, game, setup)


def main():
    parser = ArgumentParser()
    parser.add_argument("game", nargs="?", type=Path)
    parser.add_argument("--setup-exe", type=Path)
    args = parser.parse_args()
    game = game_data(args.game or choose_game())
    if game is None:
        raise ValueError("Choose the Jedi Academy game folder containing GameData/base/assets0.pk3 through assets3.pk3.")
    if ROOT.is_relative_to(game) or game.is_relative_to(ROOT):
        raise ValueError("Extract the whole ZIP to a separate folder outside your game, then open Install JA-R.cmd.")
    if args.setup_exe:  # The older GUI already owns this mutex.
        install(game, args.setup_exe)
        return
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateMutexW.argtypes = (wintypes.LPVOID, wintypes.BOOL, wintypes.LPCWSTR)
    kernel.CreateMutexW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
    handle = kernel.CreateMutexW(None, False, r"Local\JediAcademyRemixed.WindowsSetup")
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        if ctypes.get_last_error() == 183:  # ERROR_ALREADY_EXISTS
            raise RuntimeError("Another JA:R setup is already running.")
        install(game, None)
        print(f"Ready to play. Open {game} and run jar_mp.x86_64.exe for multiplayer or jar_sp.x86_64.exe for single-player.")
    finally:
        kernel.CloseHandle(handle)


if __name__ == "__main__":
    try:
        main()
    except (Exception, KeyboardInterrupt) as error:
        print("ERROR " + str(error), file=sys.stderr, flush=True)
        sys.exit(1)
