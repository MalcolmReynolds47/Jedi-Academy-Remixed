"""Independently check Falcon geometry, rig hierarchy, poses and retail aliases."""
import argparse,hashlib,json,math,re,struct,zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);args=p.parse_args()
root=Path(__file__).resolve().parents[2];files={}
for path in sorted((args.game_data/'base').glob('assets*.pk3')):
 z=zipfile.ZipFile(path)
 for n in z.namelist():files[n.lower()]=(z,n)
def source(name):z,n=files[name.lower()];return z.read(n)
def sha(b):return hashlib.sha256(b).hexdigest()
with zipfile.ZipFile(root/'build/jar-yt1300.pk3') as z:
 manifest=json.loads(z.read('jar-yt1300.json'));names=z.namelist();assert len(names)==21 and len(set(n.lower() for n in names))==21
 assert not set(n.lower() for n in names)&files.keys()
 md3=source(manifest['source']);assert sha(md3)==manifest['sourceSHA256']
 glm=z.read(manifest['target']);assert sha(glm)==manifest['modelSHA256']
 prefix=manifest['target'].rsplit('/',1)[0]+'/';gla=z.read(prefix+'model.gla');assert gla[:4]==b'2LGA' and glm[:4]==b'2LGM'
 frames,of,nb,op,os,end=struct.unpack_from('<6i',gla,76);assert frames==17 and nb==28 and end==len(gla) and len(gla[op:])%14==0
 bones=[];children=[]
 for i in range(nb):
  at=100+struct.unpack_from('<i',gla,100+i*4)[0];name=gla[at:at+64].split(b'\0')[0].decode();parent=struct.unpack_from('<i',gla,at+68)[0];base=struct.unpack_from('<12f',gla,at+72);inv=struct.unpack_from('<12f',gla,at+120);count=struct.unpack_from('<i',gla,at+168)[0]
  assert parent<i and (parent==-1)==(i==0)
  for row in range(3):
   for col in range(4):
    value=sum(base[row*4+k]*inv[k*4+col] for k in range(3))+(base[row*4+3] if col==3 else 0)
    assert abs(value-(1 if col==row else 0))<1e-5
  children.append(struct.unpack_from('<'+'i'*count,gla,at+172));bones.append((name,parent,base))
 for i,c in enumerate(children):assert list(c)==[j for j,b in enumerate(bones) if b[1]==i]
 assert bones[0][0]=='model_root' and bones[4][1]==3 and bones[6][1]==5
 for frame in range(frames):
  for bone in range(nb):
   index=int.from_bytes(gla[of+(frame*nb+bone)*3:of+(frame*nb+bone+1)*3],'little');values=struct.unpack_from('<7H',gla,op+index*14)
   assert values[:6]==(49149,32766,32766,32766,32768,32768)
   translation=(values[6]/64)-512;assert translation==(10*(frame if frame<=8 else 16-frame) if bone in (1,2) else 0)
 count=struct.unpack_from('<i',md3,84)[0];assert struct.unpack_from('<i',glm,140)[0]==nb and struct.unpack_from('<i',glm,152)[0]==count==14
 lod=struct.unpack_from('<i',glm,148)[0];mo=struct.unpack_from('<i',md3,100)[0];vertices=triangles=0
 for i in range(count):
  hierarchy=164+struct.unpack_from('<i',glm,164+i*4)[0];gs=lod+4+struct.unpack_from('<i',glm,lod+4+i*4)[0];name=md3[mo+4:mo+68].split(b'\0')[0].lower();assert glm[hierarchy:hierarchy+64].split(b'\0')[0]==name
  nv,nt,ot,osh,ost,ov,size=struct.unpack_from('<7i',md3,mo+80);gn,gv,gt,gto,nrefs,orefs,gsize=struct.unpack_from('<7i',glm,gs+12);assert (gn,gt)==(nv,nt) and struct.unpack_from('<i',glm,gs+8)[0]==-gs
  refs=struct.unpack_from('<'+'i'*nrefs,glm,gs+orefs);expected=(1,) if name==b'landing_gear' else (2,) if name==b'gear_front' else (4,6) if name==b'guns' else (0,);assert refs==expected
  assert glm[gs+gto:gs+gto+nt*12]==md3[mo+ot:mo+ot+nt*12] and glm[gs+gv+nv*32:gs+gv+nv*40]==md3[mo+ost:mo+ost+nv*8]
  shader=md3[mo+osh:mo+osh+64].split(b'\0')[0];assert glm[hierarchy+68:hierarchy+132].split(b'\0')[0]==shader
  stem=str(Path(shader.decode()).with_suffix('')).replace('\\','/').lower();assert any(stem+ext in files for ext in ('.tga','.jpg','.png'))
  for v in range(nv):
   x,y,zv,n=struct.unpack_from('<3hH',md3,mo+ov+8*v);nx,ny,nz,gx,gy,gz,packed=struct.unpack_from('<6fI',glm,gs+gv+32*v)
   assert (-gy,gx,gz)==(x/64,y/64,zv/64) and packed==(1 if name==b'guns' and zv<0 else 0)
   lat=(n>>8)*math.tau/256;lng=(n&255)*math.tau/256;assert math.dist((-ny,nx,nz),(math.cos(lat)*math.sin(lng),math.sin(lat)*math.sin(lng),math.cos(lng)))<1e-6
  vertices+=nv;triangles+=nt;mo+=size
 assert vertices==manifest['vertices']==4218 and triangles==manifest['triangles']==3703
 for row in manifest['aliases']:assert z.read(row['target'])==source(row['source']) and sha(z.read(row['target']))==row['SHA256']
 # The native definitions are deliberately retained; all of their audio/effect
 # paths must resolve against retail assets plus this additive restoration.
 available=set(files)|{n.lower() for n in names}
 for definition in ['ext_data/vehicles/yt-1300.veh','ext_data/vehicles/weapons/yt_turbolaser.vwp','ext_data/vehicles/weapons/conc_missile_straight.vwp']:
  text=re.sub(r'//[^\r\n]*','',source(definition).decode('latin1'))
  for key,value in re.findall(r'^\s*(\w+)\s+"([^"\r\n]+)"',text,re.M):
   value=value.lower()
   if key.lower().endswith('fx'):assert 'effects/'+value+'.efx' in available,(definition,key,value)
   if key.lower().startswith('sound') or key=='loopSound':
    stem=value.rsplit('.',1)[0];assert value in available or stem+'.mp3' in available or stem+'.wav' in available,(definition,key,value)
 print('PASS: 4218 original Falcon vertices, UVs/normals/materials/triangles; 28-bone hierarchy, 17 gear poses, native turret/seat/exhaust attachments, unedited retail aliases; no stock overrides')
