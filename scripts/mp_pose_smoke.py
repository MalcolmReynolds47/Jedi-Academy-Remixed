"""Retail MD3 pose bounds and persistence through the installed native server."""
import json
import math
import re
import struct
import zipfile
from mp_world_smoke import checksum, decode, spawn_points
from mp_attachment_smoke import endpoint


def audit_models(root, game_data):
    catalog = (root/'codemp/game/jar_mp_catalog.h').read_text(encoding='utf-8')
    models = {name:int(ident) for ident,name in re.findall(r'\{(\d+),1,\d+,"[^"]*","([^"]+)"',catalog) if name.endswith('.md3')}
    assets = {}
    for path in sorted((game_data/'base').glob('assets*.pk3')):
        with zipfile.ZipFile(path) as z:
            for name in models.keys() & set(z.namelist()):
                assets[name] = z.read(name)
    assert len(assets)==len(models)
    frame_total=animated=0
    for name, data in assets.items():
        ident,version=struct.unpack_from('<2i',data)
        count=struct.unpack_from('<i',data,76)[0]
        offset=struct.unpack_from('<i',data,92)[0]
        end=struct.unpack_from('<i',data,104)[0]
        assert ident==0x33504449 and version==15 and 1<=count<=65536 and 108<=offset<=end<=len(data) and count*56<=end-offset, name
        for i in range(count):
            bounds=struct.unpack_from('<6f',data,offset+i*56)
            assert all(math.isfinite(v) and abs(v)<1024 for v in bounds) and all(bounds[k]<=bounds[k+3] for k in range(3)),(name,i)
        frame_total+=count;animated+=count>1
    return models, assets, frame_total, animated


def check_poses(root, mod, game_data, rcon):
    models, assets, frame_total, animated = audit_models(root, game_data)
    rcon('set jar_worldLoad ""');rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('map mp/ffa3')
    assert 'world cleared' in rcon('jar_world_clear')
    directory=mod/'jar/worlds'
    assert 'Saved pose_seed revision 1' in rcon('jar_world_save pose_seed')
    seed=(directory/'pose_seed.0.jrw').read_bytes()
    assert struct.unpack_from('<I',seed)[0]==0x3657524a
    name='models/map_objects/wedge/beacon.md3';data=assets[name]
    count=struct.unpack_from('<i',data,76)[0];offset=struct.unpack_from('<i',data,92)[0]
    first=struct.unpack_from('<6f',data,offset);last=struct.unpack_from('<6f',data,offset+(count-1)*56)
    points=spawn_points(game_data,'maps/mp/ffa3.bsp')
    def node(point,frame,yaw):
        # Deliberately retain first-frame bounds: restore must derive the selected hull.
        return struct.pack('<6I19fI7fI',models[name],1,0,1,0,0,250.,*point,0.,yaw,0.,*first,
                           0.,0.,0.,0.,0.,0.,0,3000.,0.,0.,0.,0.,0.,1.,frame)
    def fixture(slot, records):
        out=bytearray(seed[:84]);struct.pack_into('<I',out,0,0x3457524a);struct.pack_into('<2I',out,12,len(records),0)
        out+=b''.join(records);out+=struct.pack('<I',checksum(out))
        (directory/(slot+'.0.jrw')).write_bytes(out)
        return rcon('jar_world_load '+slot)
    assert 'Loaded pose_fixture' in fixture('pose_fixture',[node(points[0],count-1,35),node(points[1],0,0)])
    assert 'Box3D active' in rcon('jar_physics_status')
    assert 'Saved pose_observed revision 1' in rcon('jar_world_save pose_observed')
    saved=decode(directory/'pose_observed.0.jrw');a,b=saved['nodes']
    assert a[33]==count-1 and b[33]==0
    corners=[endpoint(a,(x,y,z)) for x in (last[0],last[3]) for y in (last[1],last[4]) for z in (last[2],last[5])]
    low=tuple(min(p[k] for p in corners)-a[7+k] for k in range(3));high=tuple(max(p[k] for p in corners)-a[7+k] for k in range(3))
    assert math.dist(a[13:16],low)<.01 and math.dist(a[16:19],high)<.01,(a,low,high)
    assert math.dist(b[13:19],first)<.01,b
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded pose_observed revision 1' in rcon('jar_world_load pose_observed')
    assert 'Saved pose_restored revision 1' in rcon('jar_world_save pose_restored')
    restored=decode(directory/'pose_restored.0.jrw')['nodes']
    assert restored[0][33]==count-1 and restored[1][33]==0 and math.dist(restored[0][13:19],a[13:19])<.01
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded pose_invalid' not in fixture('pose_invalid',[node(points[0],count,0)])
    assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
    (mod.parent/'pose-result.json').write_text(json.dumps(dict(native_module=True,world_write_version=6,
        retail_md3_models=len(models),retail_md3_frames=frame_total,multiple_frame_models=animated,
        model=name,selected_frame=count-1,rotated_bounds_match=True,independent_instance_poses=True,
        saved_pose_restored=True,unavailable_pose_rejected_atomically=True,graphical_clients=0),indent=2)+'\n',encoding='utf-8')

    # Instantiate every prop added beyond the original size gate in a spacious
    # isolated grid above the retail map. This checks native model/body/restore
    # paths, not graphical appearance, ordinary playable placement or load FPS.
    entries = re.findall(r'\{(\d+),1,\d+,"[^"]*","([^"]+)","[^"]*",([^,]+),([^,]+),([^}]+)\}', (root/'codemp/game/jar_mp_catalog.h').read_text())
    large=[(int(i),n) for i,n,r,h,b in entries if float(r)>100 or float(h)>192]
    assert len(large)==132
    records=[]; expected=[]
    for i,(ident,name) in enumerate(large):
        data=assets[name];frames=struct.unpack_from('<i',data,76)[0];offset=struct.unpack_from('<i',data,92)[0]
        frame=frames-1; bounds=struct.unpack_from('<6f',data,offset+frame*56)
        point=(-16000+2000*(i%12),-16000+2000*(i//12),8192)
        # Tiny serialized bounds intentionally exercise recomputation from assets.
        records.append(struct.pack('<6I19fI7fI',ident,1,0,1,0,0,1500.,*point,17.,35.,12.,
            -1.,-1.,-1.,1.,1.,1.,0.,0.,0.,0.,0.,0.,0,3000.,0.,0.,0.,0.,0.,1.,frame))
        expected.append((name,ident,frame,bounds))
    assert 'Loaded large_fixture' in fixture('large_fixture',records)
    assert 'Box3D active' in rcon('jar_physics_status')
    assert 'Saved large_observed revision 1' in rcon('jar_world_save large_observed')
    observed=decode(directory/'large_observed.0.jrw')['nodes'];assert len(observed)==len(large)
    for node,(name,ident,frame,bounds) in zip(observed,expected):
        assert node[0]==ident and node[33]==frame
        # Match the solver's quarter-unit minimum half-extent for planar props.
        center=tuple((bounds[k]+bounds[k+3])*.5 for k in range(3));half=tuple(max(.25,(bounds[k+3]-bounds[k])*.5) for k in range(3))
        corners=[endpoint(node,(center[0]+sx*half[0],center[1]+sy*half[1],center[2]+sz*half[2])) for sx in (-1,1) for sy in (-1,1) for sz in (-1,1)]
        actual=tuple(min(p[k] for p in corners)-node[7+k] for k in range(3))+tuple(max(p[k] for p in corners)-node[7+k] for k in range(3))
        assert math.dist(node[13:19],actual)<.02,(name,node[13:19],actual)
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded large_observed revision 1' in rcon('jar_world_load large_observed')
    assert 'Box3D active' in rcon('jar_physics_status')
    assert 'Saved large_restored revision 1' in rcon('jar_world_save large_restored')
    restored=decode(directory/'large_restored.0.jrw')['nodes'];assert len(restored)==len(observed)
    for a,b in zip(observed,restored):
        assert a[0]==b[0] and a[33]==b[33] and math.dist(a[7:19],b[7:19])<.02
    assert 'world cleared' in rcon('jar_world_clear')
    (mod.parent/'large-prop-result.json').write_text(json.dumps(dict(native_module=True,
        newly_exposed_models=len(large),all_new_models_instantiated=True,all_new_models_restored=True,
        rotated_selected_frame_bounds_verified=True,world_write_version=6,grid_above_retail_map=True,
        graphical_clients=0,load_performance_test=False,models=[n for _,n in large]),indent=2)+'\n',encoding='utf-8')


if __name__ == '__main__':
    import argparse
    from pathlib import Path
    parser=argparse.ArgumentParser();parser.add_argument('--game-data',required=True);args=parser.parse_args()
    models,assets,frames,animated=audit_models(Path(__file__).resolve().parents[1],Path(args.game_data))
    print(f'MP asset bounds: {len(models)} MD3 props / {frames} frames / {animated} multi-frame models passed')
