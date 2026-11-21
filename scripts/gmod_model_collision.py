"""Read terminal convex pieces of the stock Source static prop collision models."""
import math
import struct


def collision_pieces(path, read):
    model, physics = read(path), read(path.removesuffix(".mdl")+".phy")
    if not model or len(model) < 12 or not physics or len(physics) < 96:
        raise ValueError(f"Missing model collision data for {path}")
    header, _, count, checksum = struct.unpack_from("<4I", physics)
    if header != 16 or count != 1 or checksum != struct.unpack_from("<I", model, 8)[0]:
        raise ValueError(f"Unsupported static collision header for {path}")
    size, magic, version, kind, surface_size = struct.unpack_from("<I4sHHI", physics, header)
    start, end = header+32, header+4+size
    if (magic, version, kind) != (b"VPHY", 256, 0) or not start+48 < end <= len(physics):
        raise ValueError(f"Unsupported compact collision surface for {path}")
    if surface_size != end-start or physics[start+44:start+48] != b"IVPS":
        raise ValueError(f"Invalid compact collision surface for {path}")

    def read_at(fmt, pos):
        if pos < start or pos+struct.calcsize(fmt) > end:
            raise ValueError(f"Collision data outside its solid for {path}")
        return struct.unpack_from(fmt, physics, pos)

    root, = read_at("<I", start+32)
    pending, visited, pieces = [start+root], set(), []
    while pending:
        node = pending.pop()
        if node < start+48 or node in visited or node+28 > end:
            raise ValueError(f"Invalid collision tree for {path}")
        visited.add(node)
        right, ledge = read_at("<2i", node)
        if right:
            if right <= 28:
                raise ValueError(f"Invalid right collision branch for {path}")
            pending.extend((node+right, node+28))
            continue
        at = node+ledge
        offset, _, flags, count = read_at("<3IH", at)
        if not ledge or flags & 3 or count < 4 or at+16*(count+1) > at+offset:
            raise ValueError(f"Invalid terminal collision piece for {path}")
        vertices = {}
        triangles = []
        for i in range(count):
            indices = read_at("<4xH2xH2xH2x", at+16*(i+1))
            for index in indices:
                if index not in vertices:
                    x, y, z = read_at("<3f", at+offset+16*index)
                    if not all(math.isfinite(value) for value in (x, y, z)):
                        raise ValueError(f"Invalid collision vertex for {path}")
                    # The compact model stores metres in IVP axes.
                    # Convert to Source's Z-up coordinates in inches.
                    vertices[index] = (x/.0254, z/.0254, -y/.0254)
            triangles.append(tuple(vertices[index] for index in indices))
        pieces.append(triangles)
    return pieces
