"""Validate the actual locally generated appearance pack before deployment."""
import argparse
import io
import json
from pathlib import Path
import re
import struct
import zipfile
from PIL import Image

parser=argparse.ArgumentParser()
parser.add_argument('--pack',type=Path,required=True)
parser.add_argument('--game-data',type=Path,required=True)
args=parser.parse_args()
base={}
archives=[]
for path in sorted((args.game_data/'base').glob('assets*.pk3')):
    z=zipfile.ZipFile(path); archives.append(z)
    for name in z.namelist(): base[name.lower()]=(z,name)

base_shaders=set()
for name in base:
    if name.startswith('shaders/') and name.endswith('.shader'):
        z,path=base[name]
        raw=re.sub(r'/\*.*?\*/|//[^\n]*','',z.read(path).decode('latin1'),flags=re.S)
        base_shaders.update(n.lower() for n in re.findall(r'(?m)^\s*([^\s{}]+/[^\s{}]+)\s*\{',raw))

def retail(name):
    z,path=base[name]; return z.read(path)

with zipfile.ZipFile(args.pack) as pack:
    names=set(pack.namelist())
    assert len(names)==len(pack.infolist()), 'duplicate archive entry'
    assert pack.testzip() is None, 'corrupt archive entry'
    assert all(n.startswith(('jar/','shaders/jar_','effects/jar/')) or n=='ext_data/sabers/jar_melee.sab' for n in names), 'global asset override'
    assert all('..' not in n and '\\' not in n for n in names), 'unsafe asset name'
    manifest=json.loads(pack.read('jar/appearance-manifest.json'))
    assert not manifest['unresolvedDependencies'], manifest['unresolvedDependencies']
    # The only global-path additions are original uniquely named definitions.
    original=Path(__file__).resolve().parents[2]/'assets/melee-definitions'
    melee=pack.read('ext_data/sabers/jar_melee.sab')
    assert melee==(original/'jar_melee.sab').read_bytes(), 'packaged melee differs from source'
    assert pack.read('jar/licenses/melee-notice.txt')==(original/'NOTICE.txt').read_bytes()
    definitions=re.findall(r'(?m)^(jar_\w+)\s*\{',melee.decode('ascii'))
    assert definitions==['jar_shortblade','jar_pike']
    assert manifest['counts']['originalMeleeDefinitions']==len(definitions)
    for model in re.findall(r'(?im)^\s*saberModel\s+(\S+)',melee.decode('ascii')):
        model=model.strip(chr(34))
        assert model.lower() in base and model.endswith('.glm') and len(model)<64, ('missing native melee model',model)

    metrics=0
    for name in names:
        if name.endswith('.fontdat'):
            src=retail(name.split('/',3)[3]); data=pack.read(name)
            assert len(src)==len(data)==7180
            # Actual retail glyph layout must remain byte-for-byte unchanged.
            assert src[7168:]==data[7168:]
            for c in range(256):
                assert src[c*28:c*28+12]==data[c*28:c*28+12]
                s,t,s2,t2=struct.unpack_from('<4f',data,c*28+12)
                assert 0<=s<=s2<=1 and 0<=t<=t2<=1
            metrics+=1
        if name.endswith('.tga'):
            header=pack.read(name)[:18]
            assert header[2] in (2,3,10), (name,'unsupported TGA type')
            assert header[16] in (24,32), (name,'unsupported TGA depth')
            if header[2]==10: assert not header[17]&0x30, (name,'unsupported TGA orientation')
    assert metrics>=8
    # Every shader map resolves to an IMAGE, never to another shader definition.
    shader_text=pack.read('shaders/jar_mbii.shader').decode('latin1') if 'shaders/jar_mbii.shader' in names else ''
    for line in shader_text.splitlines():
        tokens=line.strip().split()
        if not tokens: continue
        key=tokens[0].lower()
        images=tokens[1:] if key in ('map','clampmap') else tokens[2:] if key in ('animmap','oneshotanimmap') else []
        for image in images:
            if image.startswith('$'): continue
            stem=image.rsplit('.',1)[0] if image.endswith(('.jpg','.png','.tga')) else image
            assert image in names or any(stem+e in names or stem+e in base for e in ('.jpg','.png','.tga')), (line,'missing image')
    # SP muzzle effects carry the shot sound; MBII's client code does that job.
    # Verify both firing modes in the ACTUAL pack, not just a dependency manifest.
    for weapon in ('bryar','blaster','bowcaster'):
        for mode in ('muzzle_flash','altmuzzle_flash'):
            source=f'effects/{weapon}/{mode}.efx'
            text=pack.read('jar/fx/mbii/'+source).decode('latin1')
            stock=retail(source).decode('latin1')
            expected=re.findall(r'sound/weapons/[^\s\]"{}]+\.(?:wav|mp3)',stock,re.I)
            actual=re.findall(r'sound/weapons/[^\s\]"{}]+\.(?:wav|mp3)',text,re.I)
            assert expected and actual==expected,(source,'missing/changed/duplicate firing sounds',actual,expected)
            for sound in actual:
                sound=sound.lower()
                assert sound in base or sound[:-4]+'.mp3' in base or sound[:-4]+'.wav' in base,(source,sound)
    # Require useful, individually addressable weapon coverage; a marker file alone
    # must never turn an incomplete import into a passing release.
    for effect in ('bowcaster/shot','repeater/projectile','repeater/alt_projectile',
                   'repeater/concussion','demp2/projectile','demp2/wall_impact',
                   'rocket/shot','rocket/explosion','thermal/explosion',
                   'tripmine/explosion','detpack/explosion','concussion/explosion',
                   'flechette/wall_impact','disruptor/alt_miss'):
        assert 'jar/fx/mbii/effects/'+effect+'.efx' in names,('missing weapon adaptation',effect)
    # Projectile trails are emitted every rendered frame. Impact bursts here
    # produce continuous explosions, repeated audio and screen-filling flashes.
    for effect in ('bowcaster/shot','repeater/projectile','repeater/alt_projectile',
                   'demp2/projectile','rocket/shot'):
        text=pack.read('jar/fx/mbii/effects/'+effect+'.efx').decode('latin1')
        assert not re.search(r'(?im)^\s*(sound|flash|decal|camerashake)\s*\{',text),('impact primitive in projectile trail',effect)
    # Check references embedded in nested effects too; shader-only closure missed
    # audio and secondary particle runners in the previous importer.
    for name in names:
        if not name.endswith('.efx'): continue
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',pack.read(name).decode('latin1'),flags=re.S)
        for ref in re.findall(r'[^\s{}\[\]"]+/[^\s{}\[\]"]+',text):
            ref=ref.lower()
            candidates=[ref]+[ref+ext for ext in ('.tga','.jpg','.png','.wav','.mp3')]+['effects/'+ref+'.efx']
            if ref.endswith(('.wav','.mp3')): candidates += [ref[:-4]+ext for ext in ('.wav','.mp3')]
            if ref.startswith('fffx/'):continue # Force feedback hardware hint, not a file dependency.
            assert ref in base_shaders or any(n in names or n in base for n in candidates) or re.search(r'(?m)^'+re.escape(ref)+r'\s*$',shader_text),(name,ref)
    # Both proven missing vehicle visuals ship as original fallback media.
    for effect in ('bomb','ion_blastershot'):
        name='jar/fx/fallback/effects/ships/'+effect+'.efx'
        text=pack.read(name).decode('ascii')
        assert re.search(r'(?im)^Particle\s*$',text) and re.search(r'(?im)^Line\s*$',text), name
        assert not re.search(r'(?im)^\s*(sound|flash|decal|camerashake|emitter|light)\s*\{',text), name
        assert re.search(r'(?m)^\s*shaders\s*\n\s*\[\s*\n\s*gfx/',text), ('native shader lists require separate lines',name)
        assert 'effects/ships/'+effect+'.efx' not in names, ('must not override custom effect',effect)
    # Font atlases have useful coverage and are larger than their retail sources.
    for style in ('sans','serif'):
        n=f'jar/font/{style}/fonts/arialnb.tga'
        image=Image.open(io.BytesIO(pack.read(n)))
        assert image.size==(2048,2048)
        assert image.getchannel('A').getextrema()==(0,255)
    # Line textures must remain full blade-length strips, not spherical glows.
    for style in ('cinematic','soft'):
        im=Image.open(io.BytesIO(pack.read(f'jar/saber/{style}/gfx/effects/sabers/blue_line.jpg')))
        mid=im.width//2
        assert im.getpixel((mid,im.height//4))[0]>180, (style,'blade core ends too early')
    for category,style in (('font','sans'),('font','serif'),('ui','up2'),('ui','up4'),('saber','cinematic'),('saber','soft')):
        assert f'jar/{category}/{style}/pack.txt' in names
    print(f'Appearance assets verified: {len(names)} entries, {metrics} matching font metric tables, closed shader dependencies.')
