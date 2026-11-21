"""Read rigid LOD0 scenery meshes from Source Studio model files."""
import math
import struct


def c_string(blob, offset):
    return blob[offset:blob.index(b"\0", offset)].decode("latin1").replace("\\", "/")


def mesh_parts(path, skin, read):
    """Yield material, VVD vertices and triangle indices for each LOD0 mesh."""
    stem = path.casefold().removesuffix(".mdl")
    mdl, vvd = read(stem+".mdl"), read(stem+".vvd")
    vtx = read(stem+".dx90.vtx") or read(stem+".vtx")
    if not mdl or not vvd or not vtx:
        raise ValueError("missing MDL, VVD or VTX")
    if mdl[:4] != b"IDST" or vvd[:4] != b"IDSV" or struct.unpack_from("<i", vtx)[0] != 7:
        raise ValueError("unrecognized Studio model")
    if mdl[8:12] != vvd[8:12] or mdl[8:12] != vtx[16:20]:
        raise ValueError("model checksum mismatch")
    texture_count, texture_at, directory_count, directory_at, skin_refs, skins, skin_at, bodies, body_at = (
        struct.unpack_from("<9i", mdl, 204))
    textures = [c_string(mdl, at+struct.unpack_from("<i", mdl, at)[0])
                for at in range(texture_at, texture_at+64*texture_count, 64)]
    directories = [c_string(mdl, struct.unpack_from("<i", mdl, directory_at+4*i)[0]).rstrip("/")
                   for i in range(directory_count)]
    skin_table = struct.unpack_from(f"<{skin_refs}h", mdl, skin_at+2*skin_refs*max(0, min(skin, skins-1))) if skins else ()
    fixups, fix_at, vertices_at, tangents_at = struct.unpack_from("<4i", vvd, 48)
    sources = []
    if fixups:
        for _, first, count in struct.iter_unpack("<3i", vvd[fix_at:fix_at+12*fixups]):
            sources.extend(range(first, first+count))
    else:
        sources.extend(range((tangents_at-vertices_at)//48))
    vtx_bodies, vtx_body_at = struct.unpack_from("<2i", vtx, 28)
    if bodies != vtx_bodies:
        raise ValueError("mismatched body parts")
    for body in range(bodies):
        body_start = body_at+16*body
        body_models, model_at = struct.unpack_from("<i4xi", mdl, body_start+4)
        vtx_body = vtx_body_at+8*body
        vtx_models, vtx_model_at = struct.unpack_from("<2i", vtx, vtx_body)
        if not body_models or body_models != vtx_models:
            continue
        model = body_start+model_at
        mesh_count, mesh_at, _, vertex_at = struct.unpack_from("<4i", mdl, model+72)
        vtx_model = vtx_body+vtx_model_at
        lods, lod_at = struct.unpack_from("<2i", vtx, vtx_model)
        if not lods:
            continue
        lod = vtx_model+lod_at
        vtx_meshes, vtx_mesh_at = struct.unpack_from("<2i", vtx, lod)
        if mesh_count != vtx_meshes:
            raise ValueError("mismatched mesh counts")
        for mesh_id in range(mesh_count):
            mesh = model+mesh_at+116*mesh_id
            material_id, _, _, local_vertex = struct.unpack_from("<4i", mdl, mesh)
            if skin_table:
                material_id = skin_table[material_id]
            material = textures[material_id].casefold()
            candidates = [material]+[directory+"/"+material for directory in directories if directory]
            material = next((name for name in candidates if read("materials/"+name+".vmt")), candidates[0])
            mesh_vtx = lod+vtx_mesh_at+9*mesh_id
            groups, groups_at = struct.unpack_from("<2i", vtx, mesh_vtx)
            all_vertices, all_triangles = [], []
            for group_id in range(groups):
                group = mesh_vtx+groups_at+25*group_id
                vertex_count, vertex_offset, index_count, index_offset, strips, strips_at = (
                    struct.unpack_from("<6i", vtx, group))
                if not vertex_count or not index_count or not strips:
                    continue
                base = len(all_vertices)
                for i in range(vertex_count):
                    original = struct.unpack_from("<H", vtx, group+vertex_offset+9*i+4)[0]
                    source = sources[vertex_at//48+local_vertex+original]
                    all_vertices.append(struct.unpack_from("<8f", vvd, vertices_at+48*source+16))
                indices = struct.unpack_from(f"<{index_count}H", vtx, group+index_offset)
                for strip_id in range(strips):
                    at = group+strips_at+27*strip_id
                    count, start, _, _, _, flags = struct.unpack_from("<4ihB", vtx, at)
                    if start+count > len(indices):
                        raise ValueError("invalid strip extent")
                    if flags & 1:
                        triangles = zip(indices[start:start+count:3], indices[start+1:start+count:3],
                                        indices[start+2:start+count:3])
                    else:
                        triangles = ((indices[i+1], indices[i], indices[i+2]) if (i-start)&1 else
                                     (indices[i], indices[i+1], indices[i+2]) for i in range(start, start+count-2))
                    for a, b, c in triangles:
                        if a != b and b != c and a != c:
                            all_triangles.append((base+a, base+b, base+c))
            if all_triangles:
                yield material, all_vertices, all_triangles


def basis(angles):
    pitch, yaw, roll = (math.radians(v) for v in angles)
    sp, sy, sr = math.sin(pitch), math.sin(yaw), math.sin(roll)
    cp, cy, cr = math.cos(pitch), math.cos(yaw), math.cos(roll)
    return ((cp*cy, cp*sy, -sp), (sr*sp*cy-cr*sy, sr*sp*sy+cr*cy, sr*cp),
            (cr*sp*cy+sr*sy, cr*sp*sy-sr*cy, cr*cp))
