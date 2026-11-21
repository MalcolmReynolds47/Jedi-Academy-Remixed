"""Independent comparison of the shipped MP skeletal bodies to retail MD3s."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[2];files={};packs=[]
for path in sorted((a.game_data/'base').glob('assets*.pk3')):
 z=zipfile.ZipFile(path);packs.append(z)
 for name in z.namelist():files[name.lower()]=(z,name)
with zipfile.ZipFile(root/'build/jar-mp-legacy-droids.pk3') as z:
 manifest=json.loads(z.read('jar-mp-legacy-droids.json'));assert len(z.namelist())==9
 assert len(manifest['models'])==2 and not set(z.namelist())&files.keys()
 for row in manifest['models']:
  pack,name=files[row['source']];md3=pack.read(name);glm=z.read(row['target']);prefix=row['target'].rsplit('/',1)[0]+'/'
  assert hashlib.sha256(md3).hexdigest()==row['sourceSHA256']
  assert hashlib.sha256(glm).hexdigest()==row['targetSHA256']
  gla=z.read(prefix+'model.gla');assert gla[:4]==b'2LGA' and len(gla)==294
  assert struct.unpack_from('<6i',gla,76)==(1,276,1,280,104,294)
  assert gla[104:168].split(b'\0')[0]==b'model_root' and struct.unpack_from('<i',gla,172)[0]==-1
  assert struct.unpack_from('<24f',gla,176)==(1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.)*2
  assert struct.unpack_from('<7H',gla,280)==(49149,32766,32766,32766,32768,32768,32768)
  count=struct.unpack_from('<i',md3,84)[0];assert struct.unpack_from('<i',glm,152)[0]==count
  assert glm[72:136].split(b'\0')[0].decode()==prefix+'model'
  lod=struct.unpack_from('<i',glm,148)[0];mo=struct.unpack_from('<i',md3,100)[0];vertices=triangles=0
  for i in range(count):
   sf=164+struct.unpack_from('<i',glm,164+i*4)[0];gs=lod+4+struct.unpack_from('<i',glm,lod+4+i*4)[0]
   assert struct.unpack_from('<i',glm,gs+8)[0]==-gs
   sn=md3[mo+4:mo+68].split(b'\0')[0].lower();assert glm[sf:sf+64].split(b'\0')[0]==sn
   nv,nt,ot,osh,ost,ov,size=struct.unpack_from('<7i',md3,mo+80)
   gn,gv,gt,gto=struct.unpack_from('<4i',glm,gs+12);assert (gn,gt)==(nv,nt)
   assert glm[gs+gto:gs+gto+nt*12]==md3[mo+ot:mo+ot+nt*12]
   assert glm[gs+gv+nv*32:gs+gv+nv*40]==md3[mo+ost:mo+ost+nv*8]
   shader=md3[mo+osh:mo+osh+64].split(b'\0')[0];assert glm[sf+68:sf+132].split(b'\0')[0]==shader
   stem=str(Path(shader.decode()).with_suffix('')).replace('\\','/').lower()
   assert any(stem+ext in files for ext in ('.tga','.jpg','.png'))
   for v in range(nv):
    x,y,zv,n=struct.unpack_from('<3hH',md3,mo+ov+8*v);nx,ny,nz,gx,gy,gz,packed=struct.unpack_from('<6fI',glm,gs+gv+32*v)
    # Apply the engine root matrix to the bound vertex, then compare in MD3 space.
    assert (-gy,gx,gz)==(x/64,y/64,zv/64) and packed==0
    lat=(n>>8)*math.tau/256;lng=(n&255)*math.tau/256
    assert math.dist((-ny,nx,nz),(math.cos(lat)*math.sin(lng),math.sin(lat)*math.sin(lng),math.cos(lng)))<1e-6
   vertices+=nv;triangles+=nt;mo+=size
  assert vertices==row['vertices'] and triangles==row['triangles']
  print('PASS: exact retail geometry, normals, UVs, triangles, materials and identity rig:',row['source'],vertices,triangles)
