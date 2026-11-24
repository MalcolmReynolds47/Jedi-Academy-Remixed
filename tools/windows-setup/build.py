"""Build the offline Windows installer from a clean CMake install tree."""
from argparse import ArgumentParser
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import subprocess
import urllib.request
from zipfile import ZIP_DEFLATED, ZipFile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
DOWNLOADS = {
    "python.zip": ("https://www.python.org/ftp/python/3.13.17/python-3.13.17-embed-amd64.zip",
                   "9e35319bffedab8b36ad44d8f377cb4f1733bcec257a0b6d04d94f01486ddb38"),
    "pillow.whl": ("https://files.pythonhosted.org/packages/d7/ed/1b16fb66a37e1b9e41370b93b2c614c7da761a5b2a41dc333f7af4639962/pillow-13.0.0-cp313-cp313-win_amd64.whl",
                   "9dda0788a9932776a75d8bcc020076edc308ce2508b51f6739a45e3158b3fc42"),
}
SP, MP = "JediAcademyRemixed", "JediAcademyRemixedMP"
CRT = ("concrt140.dll", "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll",
       "msvcp140_atomic_wait.dll", "msvcp140_codecvt_ids.dll", "vcruntime140.dll", "vcruntime140_1.dll")
BUILDERS = ("build-appearance", "build-mp-interface", "build-mp-effects", "build-mp-legacy-droids",
            "build-yt1300", "build-mp-player-models", "build-legacy-npcs", "build-sandbox-icons", "jar_catalog_assets",
            "jar_zip_members")


def dependency(directory, name):
    url, expected = DOWNLOADS[name]
    path = directory / name
    if not path.is_file():
        with urllib.request.urlopen(url, timeout=90) as response:
            path.write_bytes(response.read())
    if sha256(path.read_bytes()).hexdigest() != expected:
        raise ValueError(f"Unexpected SHA-256 for {name}")
    return path


def native_files(stage):
    files = {name: stage / name for name in ("jar_sp.x86_64.exe", "jar_mp.x86_64.exe",
                                              "OpenJK/jar_gamex86_64.dll")}
    for folder, names in (
        (SP, ("jar_sp.x86_64.exe", "rdjar-rend2_x86_64.dll", "rdjar-vanilla_x86_64.dll", "SDL2.dll")),
        (MP, ("jar_mp.x86_64.exe", "jar_server.x86_64.exe", "rdjar-mp-rend2_x86_64.dll",
              "rdjar-mp-vanilla_x86_64.dll", "cgamex86_64.dll", "jampgamex86_64.dll", "uix86_64.dll")),
    ):
        for name in (*names, *CRT):
            files[f"{folder}/{name}"] = stage / folder / name
        for target, source in (("LICENSE.txt", "LICENSE.txt"), ("Box3D-LICENSE.txt", "lib/box3d/LICENSE"),
                               ("SDL2-license.txt", "lib/SDL2/include/SDL_copying.h"),
                               ("Renderer-notices.txt", "docs/JAR-renderer-notices.txt")):
            files[f"{folder}/{target}"] = ROOT / source
    files[f"{MP}/SDL2.dll"] = files[f"{SP}/SDL2.dll"]
    for name in ("Host-JA-R-Server.cmd", "Play-JA-R-Multiplayer.cmd", "Play-JA-R-Multiplayer-Rend2.cmd"):
        files[name] = ROOT / "assets/mp" / name
    for name in ("jar-mp-controls.cfg", "jar-server-example.cfg"):
        files[f"{MP}/{name}"] = ROOT / "assets/mp" / name
    files[f"{MP}/JAR-multiplayer.md"] = ROOT / "docs/JAR-multiplayer.md"
    return files


def release_version():
    header = (ROOT / "shared/qcommon/jar_release.h").read_text()
    return ".".join(re.search(r"#define JAR_VERSION_" + part + r" (\d+)", header)[1]
                    for part in ("MAJOR", "MINOR", "PATCH"))


def make_payload(stage, downloads, payload):
    manifest = {}
    with ZipFile(payload, "w", ZIP_DEFLATED, compresslevel=9) as archive:
        for name, path in sorted(native_files(stage).items()):
            data = path.read_bytes()
            for marker in ("c:\\users\\", "c:/users/", "onedrive"):
                if any(marker.encode(enc) in data.lower() for enc in ("ascii", "utf-16-le")):
                    raise ValueError(f"Private build path in {name}. Use a neutral native build directory.")
            archive.writestr("GameData/" + name, data)
            manifest[name] = sha256(data).hexdigest()
        version = release_version()
        info = json.dumps({"Schema": 1, "Version": version, "BuildId": "release-" + version,
                           "Configuration": "Release", "SourceTag": "v" + version}, indent=2).encode()
        archive.writestr("GameData/" + SP + "/build-info.json", info)
        manifest[SP + "/build-info.json"] = sha256(info).hexdigest()
        archive.writestr("native.json", json.dumps(manifest, sort_keys=True))
        archive.write(HERE / "install.py", "install.py")
        source_files = [ROOT / "scripts" / (name + ".py") for name in BUILDERS]
        source_files += [ROOT / name for name in ("code/game/anims.h", "codemp/game/anims.h",
                         "shared/qcommon/jar_galak_anims.h", "codemp/cgame/cg_weaponinit.c")]
        for folder in ("font-sources", "projectile-fallbacks", "melee-definitions", "jar"):
            source_files.extend(p for p in (ROOT / "assets" / folder).rglob("*") if p.is_file())
        for path in sorted(source_files):
            archive.write(path, "source/" + path.relative_to(ROOT).as_posix())
        for name, prefix in (("python.zip", "python/"), ("pillow.whl", "python/Lib/")):
            with ZipFile(dependency(downloads, name)) as library:
                for member in library.namelist():
                    if member.endswith("/") or member.endswith("._pth"):
                        continue
                    archive.writestr(prefix + member, library.read(member))
        archive.writestr("python/python313._pth", "python313.zip\n.\nLib\n")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path, help="CMake installation's JediAcademy folder")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    scratch = ROOT / "build/installer"
    scratch.mkdir(parents=True, exist_ok=True)
    downloads = ROOT / "build/downloads"
    downloads.mkdir(exist_ok=True)
    if args.output.suffix.lower() == ".zip":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        make_payload(args.native, downloads, args.output)
        with ZipFile(args.output, "a", ZIP_DEFLATED) as archive:
            for name in ("Install JA-R.cmd", "START-HERE.txt"):
                archive.writestr(name, (HERE / name).read_text(encoding="utf-8").replace("\n", "\r\n").encode())
        print(sha256(args.output.read_bytes()).hexdigest(), args.output.name)
        return
    payload = scratch / "payload.zip"
    make_payload(args.native, downloads, payload)
    version = release_version()
    metadata = scratch / "Release.cs"
    checksum = sha256(payload.read_bytes()).hexdigest()
    metadata.write_text(f'[assembly: System.Reflection.AssemblyVersion("{version}.0")]\n'
                        f'internal static class Release {{ public const string Version = "{version}"; '
                        f'public const string Hash = "{checksum}"; }}', encoding="ascii")
    csc = Path(os.environ["WINDIR"]) / "Microsoft.NET/Framework64/v4.0.30319/csc.exe"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(csc), "/nologo", "/codepage:65001", "/target:winexe", "/platform:x64", "/optimize+",
                    "/win32manifest:" + str(HERE / "setup.manifest"),
                    "/resource:" + str(payload) + ",bundle", "/out:" + str(args.output),
                    "/reference:System.IO.Compression.dll", "/reference:System.IO.Compression.FileSystem.dll",
                    "/reference:System.Windows.Forms.dll", "/reference:System.Drawing.dll",
                    str(metadata), str(HERE / "Setup.cs")], check=True)
    print(sha256(args.output.read_bytes()).hexdigest(), args.output.name)


if __name__ == "__main__":
    main()
