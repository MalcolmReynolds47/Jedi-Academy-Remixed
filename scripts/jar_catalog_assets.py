"""Exact asset inputs shared by catalog generation and offline qualification."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEGACY_PACK = ROOT / 'build/jar-legacy-npcs.pk3'


def catalog_asset_paths(game_data):
    packs = sorted((Path(game_data) / 'base').glob('assets*.pk3'))
    if not packs:
        raise FileNotFoundError('Jedi Academy retail assets are required')
    if not LEGACY_PACK.is_file():
        raise FileNotFoundError('Run scripts/build-legacy-npcs.py before catalog qualification')
    yt1300 = ROOT / 'build/jar-yt1300.pk3'
    if not yt1300.is_file():
        raise FileNotFoundError('Run scripts/build-yt1300.py before catalog qualification')
    return packs + [LEGACY_PACK, yt1300]
