"""Build isolated JA:R appearance assets from this owner's local installation.

No retail or MBII archives are modified. The generated pack is a local derivative,
not a grant to redistribute its third-party assets. Fonts: bundled OFL sources.
Requires Pillow. Generated filenames and shader names never override stock names.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import re
import shutil
import struct
import zipfile
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[1]
IMAGE_EXT = ('.tga', '.jpg', '.png')
UI_ROOTS = ('gfx/menus/', 'gfx/hud/', 'gfx/2d/', 'gfx/interface/', 'menu/art/', 'ui/assets/')


class ArchiveSet:
    def __init__(self, paths):
        self.entries = {}
        self.archives = []
        for path in paths:
            if not path.is_file():
                continue
            archive = zipfile.ZipFile(path)
            self.archives.append(archive)
            for info in archive.infolist():
                if not info.is_dir():
                    self.entries[info.filename.replace('\\', '/').lower()] = (archive, info)

    def read(self, name):
        archive, info = self.entries[name.lower()]
        return archive.read(info)

    def find_image(self, name):
        name = name.lower()
        stem = name.rsplit('.',1)[0] if name.endswith(IMAGE_EXT) else name
        return next((stem + ext for ext in IMAGE_EXT if stem + ext in self.entries), None)


def image_bytes(image, extension):
    stream = io.BytesIO()
    if extension == '.jpg':
        image.convert('RGB').save(stream, 'JPEG', quality=97, subsampling=0)
    elif extension == '.tga':
        image.save(stream, 'TGA', compression='tga_rle', orientation=-1)
    else:
        image.save(stream, 'PNG')
    return stream.getvalue()


def upscale(image, factor, sharpen=False):
    # Pillow's premultiplied-alpha resizing avoids dark fringes around HUD icons.
    ratio = min(factor, 4096 / max(image.size))
    size = tuple(max(1, round(n * ratio)) for n in image.size)
    result = image.resize(size, Image.Resampling.LANCZOS)
    if sharpen:
        # Preserve alpha while sharpening only color. Avoid ringing opaque text.
        rgb = result.convert('RGB').filter(ImageFilter.UnsharpMask(radius=1, percent=65, threshold=3))
        rgb.putalpha(result.getchannel('A'))
        result = rgb
    return result



def effect_blocks(text):
    """Top-level EFX primitives, with offsets for safe removal of nested blocks."""
    clean=re.sub(r'/\*.*?\*/|//[^\n]*',lambda m:' '*len(m[0]),text,flags=re.S)
    blocks=[]; depth=0; start=0; kind=''
    for token in re.finditer(r'[{}]',clean):
        if token[0]=='{':
            if depth==0:
                match=re.search(r'([A-Za-z]+)\s*$',clean[:token.start()])
                if not match: raise ValueError('EFX block has no primitive name')
                kind=match[1].lower(); start=match.start()
            depth+=1
        else:
            depth-=1
            if depth<0: raise ValueError('Unbalanced EFX block')
            if depth==0: blocks.append((kind,start,token.end()))
    if depth: raise ValueError('Unbalanced EFX block')
    return blocks


def preserve_firing_sounds(imported, retail):
    """MP supplies firing audio in code; SP supplies it in muzzle EFX files.

    Use the exact retail Sound primitives (including primary/charged variants),
    without rewriting their paths into the visual pack or playing audio twice.
    """
    sounds=[retail[a:b] for kind,a,b in effect_blocks(retail) if kind=='sound']
    if not sounds: return imported
    for kind,a,b in reversed(effect_blocks(imported)):
        if kind=='sound': imported=imported[:a]+imported[b:]
    return imported+'\n'+'\n'.join(sounds)+'\n'


def build(game_data: Path, output: Path):
    inputs = sorted((game_data / 'base').glob('assets*.pk3'))
    optional = [game_data / 'MBII' / name for name in
                ('MBAssets.pk3', 'MBAssets2.pk3', 'MBAssets3.pk3', 'MBHilts.pk3',
                 'MB_Effects.pk3', 'z_MB_BaseAssets.pk3', 'zzzzzzzzzSaber_GFX.pk3')]
    sources = sorted((ROOT / 'assets/font-sources').glob('*'))
    fallbacks = sorted((ROOT / 'assets/projectile-fallbacks').glob('*'))
    melee = sorted((ROOT / 'assets/melee-definitions').glob('*'))
    fingerprint = hashlib.sha256(Path(__file__).read_bytes())
    for path in inputs + optional + sources:
        if path.exists():
            fingerprint.update(str(path).encode())
            fingerprint.update(f'{path.stat().st_size}:{path.stat().st_mtime_ns}'.encode())
    for path in fallbacks + melee:
        fingerprint.update(path.name.encode())
        fingerprint.update(path.read_bytes())
    key = fingerprint.hexdigest()
    cache = ROOT / 'build/appearance-cache' / key
    cached = cache / 'jar-appearance.pk3'
    if cached.exists():
        cached.touch()
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(cached, output)
        print(f'Appearance pack reused: {output}', flush=True)
        return

    base = ArchiveSet(inputs)
    mb = ArchiveSet(optional)
    if 'fonts/arialnb.fontdat' not in base.entries:
        raise RuntimeError('Retail Jedi Academy assets are required to preserve UI/font metrics')
    cache.mkdir(parents=True, exist_ok=True)
    temporary = cache / 'building.pk3'
    provenance = []
    counts = {}
    used = set()
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as pack:
        def put(name, data):
            name = name.lower()
            if name in used:
                raise ValueError(f'Duplicate generated asset: {name}')
            if name.startswith('/') or '..' in name or '\\' in name:
                raise ValueError(f'Unsafe generated asset: {name}')
            # Classic's TGA loader rejects palette/RLE top-origin variants.
            # Normalize imported textures too, keeping the decoded pixels intact.
            if name.endswith('.tga') and (data[2] not in (2,3,10) or data[16] not in (24,32) or (data[2]==10 and data[17]&0x30)):
                data=image_bytes(Image.open(io.BytesIO(data)).convert('RGBA'),'.tga')
            used.add(name)
            # Stable archive timestamp makes regeneration deterministic.
            info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            pack.writestr(info, data)

        def record(source, name, target):
            archive, info = source.entries[name.lower()]
            provenance.append(dict(sourceArchive=Path(archive.filename).name,
                                   sourceEntry=info.filename, destination=target,
                                   sourceSHA256=hashlib.sha256(source.read(name)).hexdigest()))

        # Original, namespaced fallback media; never override an installed effect.
        for path in fallbacks:
            target = ('jar/fx/fallback/effects/ships/' if path.suffix == '.efx' else 'jar/licenses/projectile-fallbacks-') + path.name
            put(target, path.read_bytes())
        counts['originalProjectileFallbacks'] = sum(p.suffix == '.efx' for p in fallbacks)
        for path in melee:
            target = ('ext_data/sabers/' if path.suffix == '.sab' else 'jar/licenses/melee-') + path.name
            put(target, path.read_bytes())
        counts['originalMeleeDefinitions'] = 2

        # Every retail UI image in these namespaces, including animMap frames.
        ui_images = [n for n in base.entries if n.startswith(UI_ROOTS) and n.endswith(IMAGE_EXT)]
        for index, name in enumerate(ui_images):
            if name.startswith('gfx/2d/charsgrid'):
                continue  # font selector owns these
            image = Image.open(io.BytesIO(base.read(name))).convert('RGBA')
            for factor, style in ((2, 'up2'), (4, 'up4')):
                target = f'jar/ui/{style}/{name}'
                put(target, image_bytes(upscale(image, factor, factor == 4), Path(name).suffix))
                record(base, name, target)
            if index % 100 == 0:
                print(f'UI artwork: {index}/{len(ui_images)}', flush=True)
        counts['retailUIImages'] = len(ui_images)

        # Real high-resolution vector glyphs, retaining retail spacing and bounds.
        # Unsupported/control glyphs retain their original image instead of tofu.
        font_files = [n for n in base.entries if n.startswith('fonts/') and n.endswith('.fontdat')]
        for style, family in (('sans', 'NotoSans.ttf'), ('serif', 'NotoSerif.ttf')):
            font = ImageFont.truetype(str(ROOT / 'assets/font-sources' / family), 96)
            mono = ImageFont.truetype(str(ROOT / 'assets/font-sources/NotoSansMono.ttf'), 96)
            for name in font_files:
                if 'aurabesh' in name:  # preserve the fictional alphabet
                    continue
                texture = base.find_image(name[:-8])
                if not texture:
                    continue
                data = bytearray(base.read(name))
                if len(data) != 256 * 28 + 12:
                    raise ValueError(f'Unexpected font metrics layout: {name} ({len(data)})')
                original = Image.open(io.BytesIO(base.read(texture))).convert('RGBA')
                atlas = Image.new('RGBA', (2048, 2048), (255, 255, 255, 0))
                encoding = 'cp1251' if 'russian' in name else 'cp1250' if 'polish' in name else 'cp1252'
                face = mono if 'ocr_a' in name else font
                for ch in range(256):
                    w, h, advance, offset, baseline, s, t, s2, t2 = struct.unpack_from('<4hi4f', data, ch * 28)
                    if w <= 0 or h <= 0:
                        continue
                    glyph = None
                    if ch >= 33 and ch != 127:
                        try:
                            text = bytes([ch]).decode(encoding)
                            box = face.getbbox(text)
                            if box[2] > box[0] and box[3] > box[1]:
                                glyph = Image.new('RGBA', (box[2]-box[0], box[3]-box[1]), (255,255,255,0))
                                ImageDraw.Draw(glyph).text((-box[0],-box[1]), text, font=face, fill='white')
                        except UnicodeDecodeError:
                            pass
                    if glyph is None:
                        glyph = original.crop((round(s*original.width), round(t*original.height),
                                               round(s2*original.width), round(t2*original.height)))
                    gw, gh = min(120, max(1, w*4)), min(120, max(1, h*4))
                    glyph = glyph.resize((gw,gh), Image.Resampling.LANCZOS)
                    x, y = (ch % 16)*128+4, (ch//16)*128+4
                    atlas.paste(glyph, (x,y))
                    struct.pack_into('<4f', data, ch*28+12, x/2048, y/2048, (x+gw)/2048, (y+gh)/2048)
                put(f'jar/font/{style}/{name}', data)
                put(f'jar/font/{style}/{texture}', image_bytes(atlas, Path(texture).suffix))
                record(base, name, f'jar/font/{style}/{name}')
            # Console atlas keeps the half-cell UV convention used by SCR_DrawSmallChar.
            for name in base.entries:
                if not name.startswith('gfx/2d/charsgrid') or not name.endswith(IMAGE_EXT):
                    continue
                original = Image.open(io.BytesIO(base.read(name))).convert('RGBA')
                atlas = original.resize((2048,2048), Image.Resampling.LANCZOS)
                for ch in range(33,256):
                    try:
                        text = bytes([ch]).decode('cp1252')
                    except UnicodeDecodeError:
                        continue
                    box = mono.getbbox(text)
                    if box[2] <= box[0] or box[3] <= box[1]:
                        continue
                    glyph=Image.new('RGBA',(box[2]-box[0],box[3]-box[1]),(255,255,255,0))
                    ImageDraw.Draw(glyph).text((-box[0],-box[1]),text,font=mono,fill='white')
                    # Monospace cell, consistent ascender/baseline; do not stretch punctuation.
                    ratio=min(54/glyph.width,94/96)
                    glyph=glyph.resize((max(1,round(glyph.width*ratio)),max(1,round(glyph.height*ratio))),Image.Resampling.LANCZOS)
                    x,y=(ch%16)*128,(ch//16)*128
                    ImageDraw.Draw(atlas).rectangle((x,y,x+127,y+127),fill=(255,255,255,0))
                    atlas.paste(glyph,(x+(64-glyph.width)//2,y+16+round(box[1]*ratio)))
                put(f'jar/font/{style}/{name}',image_bytes(atlas,Path(name).suffix))
            # Alien/Asian artwork remains original, with high-resolution resampling.
            for name in base.entries:
                if name.startswith('fonts/') and name.endswith(IMAGE_EXT) and f'jar/font/{style}/{name}' not in used:
                    im=Image.open(io.BytesIO(base.read(name))).convert('RGBA')
                    put(f'jar/font/{style}/{name}',image_bytes(upscale(im,2),Path(name).suffix))
        counts['vectorFontFamilies'] = 2
        for source in sources:
            if source.suffix == '.txt' or source.name == 'sources.json':
                put('jar/licenses/'+source.name,source.read_bytes())

        # Original procedural blade profiles, suitable for the existing blade geometry.
        colors={'red':(1,.035,.01),'orange':(1,.32,.015),'yellow':(1,.82,.015),
                'green':(.035,1,.08),'blue':(.04,.22,1),'purple':(.64,.03,1)}
        for style, softness in (('cinematic',.19),('soft',.33)):
            for color, rgb in colors.items():
                for part in ('line','glow2'):
                    size=256
                    image=Image.new('RGB',(size,size))
                    pixels=image.load()
                    for y in range(size):
                        # Stock line/glow textures are circular caps, stretched by saber geometry.
                        for x in range(size):
                            dx,dy=(x+.5-size/2)/(size/2),(y+.5-size/2)/(size/2)
                            if part=='line':
                                # A continuous strip along the blade, with a softly capped tip.
                                cap=min(1,max(0,(1-abs(dy))/.08))
                                core=math.exp(-((abs(dx)/(softness*.65))**4))
                                halo=.25*math.exp(-((dx/softness)**2))
                                pixels[x,y]=tuple(round(255*cap*min(1,core+halo*c)) for c in rgb)
                            else:
                                v=math.exp(-((math.hypot(dx,dy)/softness)**2))
                                pixels[x,y]=tuple(round(255*v*c) for c in rgb)
                    put(f'jar/saber/{style}/gfx/effects/sabers/{color}_{part}.jpg', image_bytes(image,'.jpg'))
        addon=ArchiveSet([game_data/'MBII/zzzzzzzzzSaber_GFX.pk3'])
        for name in addon.entries:
            if name.startswith(('gfx/effects/sabers/','sfx_sabers/')) and name.endswith(IMAGE_EXT):
                target=f'jar/saber/saber_gfx/{name}'
                put(target,addon.read(name)); record(addon,name,target)

        # Each sound variant independently maps retail basenames; absent samples use stock.
        for style, folder in (('mbii','saber'),('ot','saberot'),('bf','bfsaber'),('tfu','saber_tfu'),('plasma','saber_plasma')):
            count=0
            for name in mb.entries:
                prefix=f'sound/weapons/{folder}/'
                if name.startswith(prefix) and name.endswith(('.wav','.mp3')):
                    basename=name[len(prefix):]
                    if '/' in basename: continue
                    target=f'jar/sound/{style}/sound/weapons/saber/{basename}'
                    if len(target)>=64: continue
                    put(target,mb.read(name)); record(mb,name,target); count+=1
            counts['sound_'+style]=count

        # Dependency-closed MBII combat effects with short, private resource names.
        # Namespaced shader definitions cannot override any stock/mod shader.
        def tokens(text):
            clean=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
            return re.findall(r'"[^"\n]*"|[{}\[\]]|[^\s{}\[\]]+',clean)
        base_shaders=set()
        for name in base.entries:
            if name.startswith('shaders/') and name.endswith('.shader'):
                raw=base.read(name).decode('latin1')
                raw=re.sub(r'/\*.*?\*/|//[^\n]*','',raw,flags=re.S)
                base_shaders.update(n.lower() for n in re.findall(r'(?m)^\s*([^\s{}]+/[^\s{}]+)\s*\{',raw))
        shaders={}
        for name in mb.entries:
            if not name.startswith('shaders/') or not name.endswith('.shader'): continue
            ts=tokens(mb.read(name).decode('latin1')); i=0
            while i+1<len(ts):
                shader=ts[i].strip('"').lower(); i+=1
                if ts[i]!='{': continue
                start=i; depth=0
                while i<len(ts):
                    if ts[i]=='{': depth+=1
                    elif ts[i]=='}': depth-=1
                    i+=1
                    if depth==0: break
                shaders[shader]=(name,ts[start:i])
        dependencies={}; shader_defs=[]; unresolved=set()

        def texture_dependency(value):
            n=value.strip('"').replace('\\','/').lower()
            if '/' not in n or n.startswith('$'): return value
            key='image:'+n
            if key in dependencies: return dependencies[key]
            candidate=mb.find_image(n)
            if candidate:
                dest='jar/m/i'+hashlib.sha256(candidate.encode()).hexdigest()[:16]+Path(candidate).suffix
                dependencies[key]=dest
                if dest not in used:
                    put(dest,mb.read(candidate)); record(mb,candidate,dest)
                return dest
            if not base.find_image(n): unresolved.add(n)
            return value

        def dependency(value):
            n=value.strip('"').replace('\\','/').lower()
            if '/' not in n or n.startswith('$'): return value
            if n in dependencies: return dependencies[n]
            tag=hashlib.sha256(n.encode()).hexdigest()[:16]
            if n in shaders:
                dest='jar/m/s'+tag; dependencies[n]=dest
                source,body=shaders[n]
                # Shader grammar is line-sensitive (animMap/map/blendFunc). Preserve lines.
                raw=mb.read(source).decode('latin1')
                # Parse the source block by balanced braces, preserving comments/newlines.
                match=re.search(r'(?im)^\s*'+re.escape(n)+r'\s*\{',raw)
                if not match:
                    raise ValueError(f'Cannot isolate shader {n}')
                begin=raw.index('{',match.start()); end=begin+1; depth=1
                while depth:
                    if raw[end]=='{': depth+=1
                    elif raw[end]=='}': depth-=1
                    end+=1
                body=raw[begin:end]
                body=re.sub(r'/\*.*?\*/|//[^\n]*','',body,flags=re.S)
                body=re.sub(r'(?im)^\s*qer_[^\r\n]*','',body) # editor-only hints are not image dependencies
                body=re.sub(r'[^\s{}\[\]"]+/[^\s{}\[\]"]+',lambda m:texture_dependency(m[0]),body)
                shader_defs.append(dest+'\n'+body+'\n')
                record(mb,source,'shaders/jar_mbii.shader')
                return dest
            candidate=n if n in mb.entries else mb.find_image(n)
            if not candidate and n.endswith('.wav') and n[:-4]+'.mp3' in mb.entries: candidate=n[:-4]+'.mp3'
            if not candidate and n.endswith('.mp3') and n[:-4]+'.wav' in mb.entries: candidate=n[:-4]+'.wav'
            if not candidate and n.startswith('sound/') and '.' not in n.rsplit('/',1)[1]:
                candidate=next((n+ext for ext in ('.wav','.mp3') if n+ext in mb.entries),None)
            if candidate and candidate.endswith(IMAGE_EXT+('.wav','.mp3')):
                dest='jar/m/a'+tag+Path(candidate).suffix
                dependencies[n]=dest; put(dest,mb.read(candidate)); record(mb,candidate,dest); return dest
            effect='effects/'+n.removeprefix('effects/').removesuffix('.efx')+'.efx'
            if effect in mb.entries:
                dest='jar/m/e'+tag; dependencies[n]=dest
                data=mb.read(effect).decode('latin1')
                data=re.sub(r'/\*.*?\*/|//[^\n]*','',data,flags=re.S)
                data=re.sub(r'[^\s{}\[\]"]+/[^\s{}\[\]"]+',lambda m:dependency(m[0]),data)
                put('effects/'+dest+'.efx',data.encode('latin1')); record(mb,effect,'effects/'+dest+'.efx'); return dest
            # Retail dependencies remain resolvable from the owner's base assets.
            if n not in base.entries and n not in base_shaders and not base.find_image(n) and effect not in base.entries and not (n.endswith('.wav') and n[:-4]+'.mp3' in base.entries) and not any(n+e in base.entries for e in ('.wav','.mp3')): unresolved.add(n)
            return value

        effect_roots=('effects/saber/','effects/blaster/','effects/bowcaster/','effects/repeater/',
                      'effects/rocket/','effects/thermal/','effects/detpack/','effects/tripmine/',
                      'effects/concussion/','effects/demp2/','effects/flechette/','effects/bryar/','effects/disruptor/')
        selected={n:n for n in mb.entries if n.startswith(effect_roots) and n.endswith('.efx') and n in base.entries}
        # MBII renamed these effects. Adapt to the retail SP event names.
        adaptations={'blaster/wall_impact':'blaster/impact_wall01r','blaster/flesh_impact':'blaster/impact_flesh',
                     'blaster/droid_impact':'blaster/impact_droid','blaster/shot':'blaster/shot01r',
                     'blaster/npcshot':'blaster/shot01r','blaster/muzzle_flash':'blaster/muzzleflash01r',
                     'blaster/altmuzzle_flash':'blaster/muzzleflash01r','bryar/wall_impact':'blaster/impact_wall01r',
                     'bryar/flesh_impact':'blaster/impact_flesh','bryar/droid_impact':'blaster/impact_droid',
                     'bryar/shot':'blaster/shot01r','bryar/npcshot':'blaster/shot01r',
                     'bryar/muzzle_flash':'blaster/muzzleflash01r','bryar/altmuzzle_flash':'blaster/muzzleflash01r'}
        # MBII groups the same cosmetic families under different directories.
        # Keep SP damage, trajectories, hit detection and weapon models untouched.
        adaptations.update({
            'bowcaster/shot':'blaster/shot02g',
            'bowcaster/explosion':'blaster/impact_wall01g',
            'bowcaster/muzzle_flash':'blaster/muzzleflash01g',
            'bowcaster/altmuzzle_flash':'blaster/muzzleflash01g',
            'repeater/projectile':'blaster/shot02y',
            'repeater/wall_impact':'blaster/impact_wall01y',
            'repeater/alt_projectile':'blaster/shot04b',
            'repeater/concussion':'grenades/exp_conc',
            'demp2/projectile':'blaster/shot06b',
            'demp2/wall_impact':'arc_caster/arccastershock',
            'flechette/wall_impact':'flechette/wall_impact_enhanced2',
            'flechette/flesh_impact':'flechette/flesh_impact',
            'rocket/shot':'rockets/missile',
            'rocket/explosion':'grenades/exp_explosion',
            'thermal/explosion':'grenades/exp_basethermal',
            'thermal/shockwave':'grenades/exp_shockwave',
            'tripmine/explosion':'grenades/exp_explosion',
            'detpack/explosion':'grenades/exp_explosion',
            'concussion/explosion':'concussion/explosion_new',
        })
        coverage={}
        for target,source in adaptations.items():
            target='effects/'+target+'.efx'; source='effects/'+source+'.efx'
            if source in mb.entries and target in base.entries: selected[target]=source
        for name,source_name in selected.items():
            data=mb.read(source_name).decode('latin1')
            data=re.sub(r'/\*.*?\*/|//[^\n]*','',data,flags=re.S)
            # Cosmetic primitives only; models need separate skin/material adaptation.
            for kind,a,b in reversed(effect_blocks(data)):
                if re.search(r'(?im)^\s*models?\s*[\[{]',data[a:b]):
                    data=data[:a]+data[b:]
            if not effect_blocks(data): continue
            data=re.sub(r'[^\s{}\[\]"]+/[^\s{}\[\]"]+',lambda m:dependency(m[0]),data)
            if 'muzzle' in name:
                data=preserve_firing_sounds(data,base.read(name).decode('latin1'))
            target='jar/fx/mbii/'+name
            if len(target)>=64: continue
            put(target,data.encode('latin1')); record(mb,source_name,target)
            coverage[name]=source_name
        if shader_defs: put('shaders/jar_mbii.shader','\n'.join(shader_defs).encode('latin1'))
        counts['combatEffects']=sum(n.startswith('jar/fx/mbii/') for n in used)
        counts['effectDependencies']=len(dependencies)
        # Marker files allow the runtime to reject unavailable selections before restarting.
        for category, styles in {'ui':['up2','up4'],'font':['sans','serif'],
                                 'saber':['cinematic','soft']+(['saber_gfx'] if addon.entries else []),
                                 'sound':[s for s in ('mbii','ot','bf','tfu','plasma') if counts['sound_'+s]],
                                 'fx':['mbii'] if counts['combatEffects'] else []}.items():
            for style in styles: put(f'jar/{category}/{style}/pack.txt',f'JA:R {category}: {style}\n'.encode())
        manifest=dict(schema=2,counts=counts,effectCoverage=coverage,unresolvedDependencies=sorted(unresolved),sources=provenance,
                      notice='Local derivatives of installed Jedi Academy and MBII assets. No third-party redistribution permission is asserted. Noto-derived glyphs use the included OFL 1.1 licenses.')
        put('jar/appearance-manifest.json',json.dumps(manifest,indent=2).encode())
    temporary.replace(cached)
    output.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(cached,output)
    print(json.dumps(counts,indent=2),flush=True)
    print(f'Unresolved effect references: {len(unresolved)}; pack: {output.stat().st_size/1048576:.1f} MiB',flush=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--game-data',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    build(args.game_data,args.output)
