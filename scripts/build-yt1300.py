"""Restore the unused YT-1300 with retail Falcon geometry and native rigging.

No stock files are replaced. Mesh, UVs, normals and materials are unchanged;
only skeletal weights, attachment points and gear translations are added.
Missing audio/effect/UI paths reuse identified, unedited retail assets.
"""
import argparse,hashlib,importlib.util,json,struct,zipfile
from pathlib import Path
from jar_catalog_assets import ROOT

spec=importlib.util.spec_from_file_location('jar_md3_adapter',ROOT/'scripts/build-mp-legacy-droids.py')
adapter=importlib.util.module_from_spec(spec);spec.loader.exec_module(adapter)
SOURCE='models/map_objects/ships/falcon2.md3'
PREFIX='models/players/YT-1300/'
IDENTITY=(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.)

def pose(point,backward=False):
    # Ghoul2's native root maps (x,y,z) to (-y,x,z).
    x,y,z=point;s=-1. if backward else 1.
    return (s,0.,0.,y,0.,s,0.,-x,0.,0.,1.,z)

def inverse(m):
    return tuple(m[c*4+r] if c<3 else -sum(m[k*4+r]*m[k*4+3] for k in range(3)) for r in range(3) for c in range(4))

def bones():
    result=[]
    def add(name,parent=0,point=(0,0,0),backward=False):
        result.append((name,parent,pose(point,backward)));return len(result)-1
    add('model_root',-1)
    add('gear_rear');add('gear_front')
    top=add('guntop_yaw',point=(-132,-4,64));top=add('guntop_pitch',top,(-132,-4,64))
    bottom=add('gunbot_yaw',point=(-132,-4,-77));bottom=add('gunbot_pitch',bottom,(-132,-4,-77))
    add('*driver',point=(172,-315,14))
    add('*droidunit',point=(-180,-100,18))
    for i,y in enumerate((-180,0,180),1):add(f'*exhaust{i}',point=(-469,y,-6),backward=True)
    for i,parent,x,y,z in [(1,bottom,-9,-13,-85),(2,bottom,-9,5,-85),(3,top,-9,-13,73),(4,top,-9,5,73),(5,0,285,-110,-22),(6,0,285,110,-22)]:add(f'*muzzle{i}',parent,(x,y,z))
    add('*turretview_t',top,(-100,-4,96));add('*turretview_b',bottom,(-100,-4,-105))
    for i in range(8):add(f'*passenger{i+1}',point=(-80-50*(i//2),-55+110*(i%2),8))
    return result

def skeleton(rig):
    table=[];body=bytearray();count=len(rig)
    for i,(name,parent,base) in enumerate(rig):
        children=[j for j,b in enumerate(rig) if b[1]==i];table.append(4*count+len(body))
        body+=adapter.string(name)+struct.pack('<Ii24fi',0,parent,*base,*inverse(base),len(children))+struct.pack('<'+'i'*len(children),*children)
    frames=bytearray();pool=[];unique={}
    for frame in range(17):
        progress=frame if frame<=8 else 16-frame
        for i in range(count):
            z=10*progress if i in (1,2) else 0
            compressed=struct.pack('<7H',49149,32766,32766,32766,32768,32768,32768+64*z)
            if compressed not in unique:unique[compressed]=len(pool);pool.append(compressed)
            frames+=unique[compressed].to_bytes(3,'little')
    frame_offset=100+4*count+len(body);frames+=bytes((-len(frames))%4)
    pool_offset=frame_offset+len(frames)
    return (struct.pack('<4sI64sf6i',b'2LGA',6,adapter.string(PREFIX+'model'),1.,17,frame_offset,count,pool_offset,100+4*count,pool_offset+14*len(pool))
            +struct.pack('<'+'i'*count,*table)+body+frames+b''.join(pool))

def model(data,rig):
    original,gla,skin,nv,nt=adapter.convert(data,'YT-1300')
    count=struct.unpack_from('<i',original,152)[0];lod_offset=struct.unpack_from('<i',original,148)[0]
    result=bytearray(original[:lod_offset]);geometry=bytearray();offsets=[];mapping={}
    for i in range(count):
        hierarchy=164+struct.unpack_from('<i',original,164+i*4)[0]
        name=original[hierarchy:hierarchy+64].split(b'\0')[0].decode();start=lod_offset+4+struct.unpack_from('<i',original,lod_offset+4+i*4)[0]
        _,_,_,verts,ov,tris,ot,_,_,_=struct.unpack_from('<10i',original,start)
        vertices=bytearray(original[start+ov:start+ov+verts*32]);triangles=original[start+ot:start+ot+tris*12];uv=original[start+ov+verts*32:start+ov+verts*40]
        refs=[1] if name=='landing_gear' else [2] if name=='gear_front' else [4,6] if name=='guns' else [0]
        for v in range(verts):
            bone=1 if name=='guns' and struct.unpack_from('<f',vertices,v*32+20)[0]<0 else 0
            struct.pack_into('<I',vertices,v*32+24,bone)
        relative=count*4+len(geometry);offsets.append(relative);vertex_offset=40+len(triangles);bone_offset=vertex_offset+len(vertices)+len(uv)
        geometry+=struct.pack('<10i',0,i,-(lod_offset+4+relative),verts,vertex_offset,tris,40,len(refs),bone_offset,bone_offset+4*len(refs))+triangles+vertices+uv+struct.pack('<'+'i'*len(refs),*refs)
        mapping[name]=[rig[n][0] for n in refs]
    lod=struct.pack('<i',4+count*4+len(geometry))+struct.pack('<'+'i'*count,*offsets)+geometry
    struct.pack_into('<i',result,140,len(rig));struct.pack_into('<i',result,160,lod_offset+len(lod))
    return bytes(result)+lod,skin,dict(vertices=nv,triangles=nt,surfaces=count,weights=mapping)

def build(game_data):
    files={};packs=[]
    for path in sorted((game_data/'base').glob('assets*.pk3')):
        z=zipfile.ZipFile(path);packs.append(z)
        for n in z.namelist():files[n.lower()]=(z,n)
    def read(name):z,n=files[name.lower()];return z.read(n)
    source=read(SOURCE);assert len(source)==114552 and adapter.fnv(source)==3607401044,'Unqualified Falcon source'
    rig=bones();glm,skin,stats=model(source,rig)
    content={PREFIX+'model.glm':glm,PREFIX+'model.gla':skeleton(rig),PREFIX+'model_default.skin':skin,
             PREFIX+'animation.cfg':b'BOTH_VS_IDLE 0 1 0 20\nBOTH_STAND1 0 1 0 20\nBOTH_GEARS_CLOSE 1 8 -1 20\nBOTH_GEARS_OPEN 9 8 -1 20\nBOTH_WINGS_OPEN 1 8 -1 20\nBOTH_WINGS_CLOSE 8 1 -1 20\n'}
    aliases={}
    for target,source_name in [('on.mp3','on.mp3'),('off.mp3','off.mp3'),('loop.wav','loop.wav'),('flyby.mp3','flyby.mp3'),('flyby2.mp3','flyby2.mp3'),('enginestart.mp3','on.mp3')]:aliases['sound/vehicles/yt-1300/'+target]='sound/vehicles/z-95/'+source_name
    for target,source_name in [('yt_exhaust','xwing_exhaust'),('yt_exhaust_turbo','xwing_exhaust_turbo'),('YT_nose','xw_nose'),('YT_lwing','xw_lwing'),('YT_rwing','xw_rwing'),('yt_turbomuzzleflash','reb_blastermuzzleflash')]:aliases['effects/ships/'+target+'.efx']='effects/ships/'+source_name+'.efx'
    for part in ['front','back','left','right']:aliases['gfx/menus/radar/yt-1300_'+part+'.tga']='gfx/menus/radar/yt-1300.tga'
    copied=[]
    for name,source_name in aliases.items():
        if name.lower() in files:continue
        content[name]=read(source_name);copied.append(dict(target=name,source=source_name,SHA256=hashlib.sha256(content[name]).hexdigest()))
    assert all(n.lower() not in files for n in content),'Do not replace existing retail resources'
    manifest=dict(schema=1,source=SOURCE,sourceSHA256=hashlib.sha256(source).hexdigest(),target=PREFIX+'model.glm',modelSHA256=hashlib.sha256(glm).hexdigest(),frames=17,bones=len(rig),**stats,attachments=[dict(name=n,parent=p,bind=m) for n,p,m in rig],aliases=copied)
    content['jar-yt1300.json']=(json.dumps(manifest,indent=2)+'\n').encode()
    output=ROOT/'build/jar-yt1300.pk3'
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(content.items()):
            info=zipfile.ZipInfo(name,date_time=(2026,10,11,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,data)
    for z in packs:z.close()
    print('YT-1300 retail body and native rig:',stats['vertices'],'vertices;',len(rig),'bones;',len(content),'files;',output)
    return manifest

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);args=p.parse_args();build(args.game_data)
