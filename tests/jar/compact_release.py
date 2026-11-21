"""Check range-based reconstruction, including refusal of a damaged source."""
import hashlib
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import sys
from zipfile import ZIP_DEFLATED, ZipFile

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'scripts'))
from jar_compact_release import recipe, logical_hash


with TemporaryDirectory() as work:
    a,b = (Path(work)/n for n in ('source.pk3','target.pk3'))
    samples = [hashlib.shake_256(str(n).encode()).digest(65536) for n in range(3)]
    for archive, indexes in ((a,[2,0,1]),(b,[1,2,0])):
        with ZipFile(archive,'w',ZIP_DEFLATED) as out:
            for n in indexes:
                out.writestr('ui/'+str(n),samples[n])
            out.writestr('manifest',archive.name)
    plan=json.loads(json.dumps(recipe(a,b)))
    expected=hashlib.sha256(b.read_bytes()).hexdigest()
    with a.open('rb') as src:
        assert logical_hash(src,plan)==expected
    first=next(op for op in plan['segments'] if isinstance(op,list))
    with a.open('r+b') as source:
        source.seek(first[0]+3)
        byte=source.read(1)[0]; source.seek(-1,1); source.write(bytes([byte^1]))
        try:
            logical_hash(source,plan)
        except ValueError:
            pass
        else:
            raise AssertionError('Damaged input passed hash validation')
    print('Compact interface round trip and damaged-source rejection passed.')
