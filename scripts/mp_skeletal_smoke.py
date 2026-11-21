"""Real Ghoul2 poses through the private native server; no graphical client."""
import json
import math
import re
import struct
from mp_world_smoke import checksum, decode
from mp_attachment_smoke import endpoint


def check_skeletal(root, mod, game_data, rcon):
    catalog=(root/'codemp/game/jar_mp_catalog.h').read_text()
    models=[(int(i),n) for i,n in re.findall(r'\{(\d+),1,\d+,"[^"]*","([^"]+)"',catalog) if n.endswith('.glm')]
    assert len(models)==132
    rcon('set jar_worldLoad ""');rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('map mp/ffa3');assert 'world cleared' in rcon('jar_world_clear')
    directory=mod/'jar/worlds';assert 'Saved skeletal_seed revision 1' in rcon('jar_world_save skeletal_seed')
    seed=(directory/'skeletal_seed.0.jrw').read_bytes();audit=[];failures=[]
    for ident,name in models:
        entry=dict(id=ident,model=name,poses=[])
        for frame in (0,-1):
            if frame<0:
                frame=entry.get('frames',1)-1
                if frame==0 and entry['poses']:
                    entry['poses'].append(entry['poses'][0]);continue
            reply=rcon(f'jar_model_info {ident} {frame}')
            match=re.search(r'Model (\d+) frame (\d+) of (\d+): ([^\r\n]+)',reply)
            assert match,(name,reply)
            entry['frames']=int(match[3])
            if 'unavailable' in match[4]:failures.append((name,frame,entry['frames']));continue
            bounds=tuple(map(float,match[4].split()));assert len(bounds)==6 and all(math.isfinite(v) for v in bounds)
            entry['poses'].append(dict(frame=frame,bounds=bounds))
        audit.append(entry)
    (mod.parent/'skeletal-audit.json').write_text(json.dumps(dict(models=audit,failures=failures),indent=2)+'\n')
    assert not failures, failures

    def fixture(slot,mode):
        out=bytearray(seed[:84]);struct.pack_into('<I',out,0,0x3457524a);struct.pack_into('<2I',out,12,len(audit),0)
        for i,row in enumerate(audit):
            selected=row['poses'][mode];point=(-24576+4096*(i%12),-24576+4096*(i//12),12000)
            out+=struct.pack('<6I19fI7fI',row['id'],1,0,1,0,0,200.,*point,17.,35.,12.,-1.,-1.,-1.,1.,1.,1.,0.,0.,0.,0.,0.,0.,0,3000.,0.,0.,0.,0.,0.,1.,selected['frame'])
        out+=struct.pack('<I',checksum(out));(directory/(slot+'.0.jrw')).write_bytes(out)
        assert 'Loaded '+slot in rcon('jar_world_load '+slot)
        assert 'Box3D active' in rcon('jar_physics_status')
        assert 'Saved '+slot+'_seen revision 1' in rcon('jar_world_save '+slot+'_seen')
        nodes=decode(directory/(slot+'_seen.0.jrw'))['nodes'];assert len(nodes)==len(audit)
        for node,row in zip(nodes,audit):
            selected=row['poses'][mode];bounds=selected['bounds'];assert node[0]==row['id'] and node[33]==selected['frame']
            center=tuple((bounds[k]+bounds[k+3])*.5 for k in range(3));half=tuple(max(.25,(bounds[k+3]-bounds[k])*.5) for k in range(3))
            corners=[endpoint(node,tuple(center[k]+sign[k]*half[k] for k in range(3))) for sign in ((x,y,z) for x in (-1,1) for y in (-1,1) for z in (-1,1))]
            expected=tuple(min(p[k] for p in corners)-node[7+k] for k in range(3))+tuple(max(p[k] for p in corners)-node[7+k] for k in range(3))
            assert math.dist(node[13:19],expected)<.03,(row['model'],mode,node[13:19],expected)
        assert 'world cleared' in rcon('jar_world_clear')
        assert 'Loaded '+slot+'_seen revision 1' in rcon('jar_world_load '+slot+'_seen')
        assert 'Box3D active' in rcon('jar_physics_status')
        assert 'world cleared' in rcon('jar_world_clear')
    fixture('skeletal_default',0);fixture('skeletal_last',1)
    (mod.parent/'skeletal-result.json').write_text(json.dumps(dict(native_module=True,models=len(audit),
        multiple_frame_models=sum(m['frames']>1 for m in audit),all_default_and_last_frames_spawned=True,
        rotated_posed_hulls=True,all_restored=True,graphical_clients=0,grid_above_retail_map=True,
        load_performance_test=False),indent=2)+'\n')
