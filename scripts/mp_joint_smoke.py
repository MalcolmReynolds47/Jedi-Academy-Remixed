"""Native Weld/Hinge physics and JRW5 import and JRW6 restoration in the private test server."""
import json
import math
import re
import struct
import time
from mp_world_smoke import checksum, decode, spawn_points
from mp_attachment_smoke import endpoint


def check_joints(root, mod, game_data, rcon):
    rcon('set jar_worldLoad ""');rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('devmap mp/ffa3');assert 'world cleared' in rcon('jar_world_clear')
    directory=mod/'jar/worlds';assert 'Saved joint_seed revision 1' in rcon('jar_world_save joint_seed')
    seed=(directory/'joint_seed.0.jrw').read_bytes();assert struct.unpack_from('<I',seed)[0]==0x3657524a
    catalog=(root/'codemp/game/jar_mp_catalog.h').read_text(encoding='utf-8')

    def entry(name):
        m=re.search(r'\{(\d+),(\d+),\d+,"'+re.escape(name)+r'","[^"]*","[^"]*",([^,]+),([^,]+),([^}]+)\}',catalog)
        assert m,name
        ident,kind=int(m[1]),int(m[2]);radius,height,bottom=map(float,m.groups()[2:])
        return ident,kind,(-radius,-radius,bottom,radius,radius,bottom+height)

    prop=entry('models/chunks/crate/crate2_1.md3');npc=entry('stormtrooper2')
    origin=spawn_points(game_data,'maps/mp/ffa3.bsp')[0]
    def node(model,position,frozen=0,no_gravity=0,spin=0):
        return struct.pack('<6I19fI7fI',model[0],frozen,0,no_gravity,10 if model[1] in (2,3) else 0,0,
                           200.,*position,0.,0.,0.,*model[2],0.,0.,0.,0.,0.,spin,0,3000.,0.,0.,0.,0.,0.,1.,0)

    def joint(first,second,a,b,kind):
        return struct.pack('<ii7fI8f',first,second,*a,*b,0.,kind,0.,0.,0.,1.,0.,0.,0.,1.)

    def fixture(slot,nodes,links):
        out=bytearray(seed[:84]);struct.pack_into('<I',out,0,0x3557524a);struct.pack_into('<2I',out,12,len(nodes),len(links))
        out+=b''.join(nodes)+b''.join(links);out+=struct.pack('<I',checksum(out))
        (directory/(slot+'.0.jrw')).write_bytes(out)
        return rcon('jar_world_load '+slot)

    def save(slot):
        assert 'Saved '+slot+' revision 1' in rcon('jar_world_save '+slot)
        return decode(directory/(slot+'.0.jrw'))

    def error(saved,link):
        a,b=link[:2]
        first=link[2:5] if a<0 else endpoint(saved['nodes'][a],link[2:5])
        second=link[5:8] if b<0 else endpoint(saved['nodes'][b],link[5:8])
        return math.dist(first,second)

    assert 'Loaded joint_weld' in fixture('joint_weld',[node(prop,origin)],[joint(0,-1,(0,0,0),origin,1)])
    time.sleep(3);weld=save('joint_weld_seen');assert error(weld,weld['ropes'][0])<.05
    assert math.dist(weld['nodes'][0][7:10],origin)<.05 and weld['ropes'][0][9]==1
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded joint_hinge' in fixture('joint_hinge',[node(prop,origin,no_gravity=1,spin=1)],[joint(-1,0,origin,(0,0,0),2)])
    time.sleep(2);hinge=save('joint_hinge_seen');assert error(hinge,hinge['ropes'][0])<.1
    assert abs(hinge['nodes'][0][11])>20 and abs(hinge['nodes'][0][10])<.1 and abs(hinge['nodes'][0][12])<.1
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded joint_hinge_seen' in rcon('jar_world_load joint_hinge_seen')
    time.sleep(1);restored=save('joint_hinge_restored');assert error(restored,restored['ropes'][0])<.1
    assert restored['ropes']==hinge['ropes'] and abs(restored['nodes'][0][11]-hinge['nodes'][0][11])>5
    assert 'world cleared' in rcon('jar_world_clear')
    # A native NPC anchors a two-prop component; every link must use the same
    # native graph solver. Initialization and collision still belong to the game.
    npc_point=(origin[0],origin[1],origin[2]+64)
    p=(npc_point[0]+60,npc_point[1],npc_point[2]+24);q=(p[0]+60,p[1],p[2])
    assert 'Loaded joint_mixed' in fixture('joint_mixed',[node(prop,p),node(prop,q),node(npc,npc_point,frozen=1)],
                                         [joint(0,1,(0,0,0),(-60,0,0),1),joint(0,2,(0,0,0),(60,0,24),1)])
    time.sleep(4);mixed=save('joint_mixed_seen');assert len(mixed['nodes'])==3 and mixed['nodes'][2][4]==10
    assert all(error(mixed,r)<.2 for r in mixed['ropes'])
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded joint_mixed_seen' in rcon('jar_world_load joint_mixed_seen')
    time.sleep(4);mixed_restore=save('joint_mixed_restored');assert all(error(mixed_restore,r)<.2 for r in mixed_restore['ropes'])
    assert mixed_restore['ropes']==mixed['ropes'] and mixed_restore['nodes'][2][4]==10
    assert 'world cleared' in rcon('jar_world_clear')
    # Native-only graphs have no Box3D prop to trigger solver initialization.
    # Freeze AI decisions, not native movement or the attachment itself.
    assert 'cheat protected' not in rcon('set d_npcfreeze 1')
    native=[]
    try:
        assert 'Loaded joint_npc' in fixture('joint_npc',[node(npc,npc_point)],[joint(0,-1,(0,0,0),npc_point,1)])
        time.sleep(4);held=save('joint_npc_seen');assert held['nodes'][0][1]==0 and held['nodes'][0][4]==10
        assert error(held,held['ropes'][0])<.2
        native.append(dict(name='stormtrooper2',error=error(held,held['ropes'][0]),health=10))
        assert 'world cleared' in rcon('jar_world_clear')
        rcon('devmap mp/ffa5');assert 'cheat protected' not in rcon('set d_npcfreeze 1');assert 'Saved joint_air revision 1' in rcon('jar_world_save joint_air')
        seed=(directory/'joint_air.0.jrw').read_bytes();points=spawn_points(game_data,'maps/mp/ffa5.bsp',airspace=True)
        for index,name in enumerate(('swoop_mp','tie-fighter','wildtauntaun','atst_vehicle')):
            model=entry(name);slot='joint_vehicle_'+str(index);created=False
            for point in points:
                reply=fixture(slot,[node(model,point)],[joint(0,-1,(0,0,0),point,1)])
                if 'Loaded '+slot in reply:created=True;break
            assert created,(name,'no native hull placement')
            time.sleep(4);vehicle=save(slot+'_seen');assert len(vehicle['nodes'])==1 and vehicle['nodes'][0][4]==10
            assert vehicle['nodes'][0][1]==0 and error(vehicle,vehicle['ropes'][0])<.2,(name,vehicle)
            native.append(dict(name=name,error=error(vehicle,vehicle['ropes'][0]),health=10))
            assert 'world cleared' in rcon('jar_world_clear')
    finally:
        rcon('set d_npcfreeze 0')
    (mod.parent/'joint-result.json').write_text(json.dumps(dict(native_module=True,world_write_version=6,
        rigid_world_weld=True,rigid_axial_hinge=True,hinge_spin_restored=True,immutable_frames=True,
        mixed_two_prop_npc_graph=True,mixed_graph_restored=True,native_only_world_welds=native,
        graphical_clients=0,ai_decisions_frozen=True),indent=2)+'\n',encoding='utf-8')
