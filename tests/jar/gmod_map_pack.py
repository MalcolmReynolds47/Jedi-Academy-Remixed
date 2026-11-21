"""Check both converted maps and the authored Construct room dependencies."""
from io import BytesIO
from pathlib import Path, PurePosixPath
import re
from struct import Struct, unpack_from
import sys
import wave
from zipfile import ZipFile

path = Path(sys.argv[1])
with ZipFile(path) as pack:
    names = [name for name in pack.namelist() if not name.endswith("/")]
    assert len(set(name.lower() for name in names)) == len(names), "Duplicate map archive path"
    for name in names:
        parts = PurePosixPath(name).parts
        assert "\\" not in name and ".." not in parts, name
        assert any(name.startswith(prefix) for prefix in ("textures/jg/", "sound/jg/", "models/jg/",
                                                          "maps/gm_", "levelshots/gm_", "scripts/jg_",
                                                          "shaders/jg_")), name
    assert pack.testzip() is None, "Corrupt local map archive"
    for name in ("gm_construct", "gm_flatgrass"):
        with pack.open(f"maps/{name}.bsp") as bsp:
            assert bsp.read(8) == b"RBSP\x01\x00\x00\x00", name
        arena = pack.read(f"scripts/jg_{name}.arena").decode("ascii")
        assert re.search(rf'^map "{name}"$', arena, re.M) and re.search(r'^type "ffa"$', arena, re.M)
        assert f"levelshots/{name}.png" in names, name
    construct = pack.read("maps/gm_construct.bsp")
    def lump(index):
        offset, length = unpack_from("<2i", construct, 8 + 8 * index)
        assert 0 <= offset <= len(construct) and 0 <= length <= len(construct) - offset
        return construct[offset:offset + length]

    entities = [dict(re.findall(r'"([^"]+)"\s+"([^"]*)"', block))
                for block in re.findall(r"\{([^{}]*)\}", lump(0).decode("ascii"))]
    switch, = [e for e in entities if e.get("classname") == "func_usable" and e.get("target") == "jg_room_lights"]
    light, speaker = sorted((e for e in entities if e.get("targetname") == switch["target"]),
                            key=lambda e: e["classname"])
    assert light["classname"] == "light" and speaker["classname"] == "target_speaker"
    assert switch["model"] == "*1" and int(switch["spawnflags"]) & 72 == 72
    assert speaker["origin"] == switch["origin"] and light["style"] == "32"
    with wave.open(BytesIO(pack.read(speaker["noise"]))) as audio:
        assert (audio.getnchannels(), audio.getsampwidth(), audio.getframerate()) == (1, 2, 22050)
        assert any(audio.readframes(audio.getnframes()))
    mirror, = [e for e in entities if e.get("classname") == "misc_portal_surface"]
    assert mirror["origin"] == "-2064 -2067 -286" and not mirror.get("target")
    shaders = list(Struct("<64s2i").iter_unpack(lump(1)))
    pages = len(lump(14)) // (128 * 128 * 3)
    styled = [s for s in Struct("<7i8B14i12f2i").iter_unpack(lump(13)) if s[8] == 32]
    assert len(styled) == 174 and all(0 <= s[15] < pages and 0 <= s[16] < pages for s in styled)
    portal_shaders = pack.read("shaders/jg_gm_construct.shader").decode("ascii")
    portal = re.findall(r"(?m)^([^\s]+)\n\{\nportal\n", portal_shaders)
    assert len(portal) == 1 and any(name.split(b"\0", 1)[0] == portal[0].encode("ascii") for name, *_ in shaders)
print(f"Verified local map archive: {len(names)} files, both RBSPs, FFA metadata, previews, and Construct room assets")
