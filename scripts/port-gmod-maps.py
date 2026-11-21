"""Convert the locally installed stock GMod maps to an isolated JA:R PK3."""
from __future__ import annotations

import argparse
from collections import Counter
import io
import json
import math
from pathlib import Path
import re
import struct
import wave
import zipfile

from PIL import Image, ImageOps
import numpy as np
from srctools import Keyvalues
from srctools.bsp import BSP
from srctools.vmt import Material
from srctools.vpk import VPK
from srctools.vtf import VTF
from gmod_model_mesh import basis, mesh_parts
from gmod_model_collision import collision_pieces

MAPS = ("gm_flatgrass", "gm_construct")
SOURCE_SKY = 0x0006
SOURCE_NODRAW = 0x0080
SOURCE_BUMPLIGHT = 0x0800
RBSP_SOLID, RBSP_WATER, RBSP_PLAYERCLIP = 1, 4, 16
RBSP_MONSTERCLIP, RBSP_OPAQUE = 32, 32768
RBSP_LADDER = 8192
NO_LIGHTMAP = -3


def rows(fmt, data):
    return list(struct.iter_unpack("<" + fmt, data))


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x-y for x, y in zip(a, b))


def offset_by(point, direction, distance):
    return tuple(a+b*distance for a, b in zip(point, direction))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def normal(a):
    length = math.sqrt(dot(a, a))
    return tuple(v/length for v in a) if length else (0, 0, 1)


def convex_brush(triangles):
    points = list(dict.fromkeys(point for triangle in triangles for point in triangle))
    if len(points) < 4:
        raise ValueError("Static prop collision piece has no volume")
    center = tuple(sum(point[i] for point in points)/len(points) for i in range(3))
    axes = [(1, 0, 0), (0, 1, 0), (0, 0, 1)]
    planes = []

    def add(direction, distance):
        if not any(dot(direction, n) > 1-1e-8 and abs(distance-d) < .001 for n, d in planes):
            planes.append((direction, distance))

    # CM_BoundBrush requires the six axial supports first.
    for axis in axes:
        for direction in (tuple(-v for v in axis), axis):
            add(direction, max(dot(direction, p) for p in points))
    edges = set()
    for a, b, c in triangles:
        face = cross(sub(b, a), sub(c, a))
        if dot(face, face) < 1e-10:
            continue
        n = normal(face)
        if dot(n, sub(center, a)) > 0:
            n = tuple(-v for v in n)
        distance = max(dot(n, p) for p in points)
        if distance-dot(n, a) > .002 or distance-dot(n, center) < 1e-5:
            raise ValueError("Non-convex or flat static prop collision piece")
        add(n, distance)
        edges.update(tuple(sorted(edge)) for edge in ((a, b), (b, c), (c, a)))
    # Edge/axis separating planes keep swept player boxes off empty corners.
    for a, b in sorted(edges):
        for axis in axes:
            bevel = cross(sub(b, a), axis)
            if dot(bevel, bevel) < 1e-10:
                continue
            n = normal(bevel)
            for direction in (n, tuple(-v for v in n)):
                distance = max(dot(direction, p) for p in points)
                if distance-dot(direction, a) < .001:
                    add(direction, distance)
    return planes, points


class SourceMap:
    def __init__(self, path):
        self.blob = path.read_bytes()
        if self.blob[:8] != b"VBSP\x14\x00\x00\x00":
            raise ValueError(f"Unsupported Source BSP {path}")
        self.lumps = [struct.unpack_from("<4i", self.blob, 8+16*i) for i in range(64)]
        self.pak = zipfile.ZipFile(io.BytesIO(self.lump(40)))
        self.files = {name.casefold(): name for name in self.pak.namelist()}
        self.entities = [dict(re.findall(r'"([^"]*)"\s+"([^"]*)"', text))
                         for text in re.findall(r"\{([^{}]*)\}", self.lump(0).decode("latin1"))]

    def lump(self, index):
        offset, size, _, compressed = self.lumps[index]
        if compressed or offset < 0 or size < 0 or offset+size > len(self.blob):
            raise ValueError(f"Unsupported Source lump {index}")
        return self.blob[offset:offset+size]


class Materials:
    def __init__(self, gmod, source, map_name, files):
        self.gmod, self.source, self.files, self.map_name = gmod, source, files, map_name
        paths = [gmod/"garrysmod"/"garrysmod_dir.vpk"]
        paths.extend((gmod/"sourceengine").glob("*_dir.vpk"))
        self.packs = [VPK(path) for path in paths if path.is_file()]
        self.shaders = []
        self.ids = {}
        self.definitions = []
        self.images = {}
        self.sky_stem = None
        self.missing = set()
        self.add("$collision", 0, RBSP_SOLID | RBSP_OPAQUE)

    def read(self, name):
        name = name.casefold().replace("\\", "/")
        if name in self.source.files:
            return self.source.pak.read(self.source.files[name])
        for directory in ("garrysmod", "sourceengine"):
            path = self.gmod/directory/name
            if path.is_file():
                return path.read_bytes()
        for pack in self.packs:
            if name in pack:
                return pack[name].read()
        return None

    def material(self, name, visited=()):
        blob = self.read("materials/"+name+".vmt")
        if blob is None:
            return None
        material = Material.parse(io.StringIO(blob.decode("latin1")), name)
        if material.shader.casefold() == "patch":
            target = material.get("include", "").casefold().removeprefix("materials/").removesuffix(".vmt")
            if not target or target in visited:
                return None
            base = self.material(target, (*visited, name))
            if base is None:
                return None
            for block in material.blocks:
                if block.name in ("insert", "replace"):
                    for item in block:
                        if not item.has_children():
                            base[item.name] = item.value
            return base
        return material

    def texture(self, name):
        name = name.casefold().replace("\\", "/").removesuffix(".vtf")
        if name not in self.images:
            raw = self.read("materials/"+name+".vtf")
            if raw is None:
                return None
            with io.BytesIO(raw) as source:
                vtf = VTF.read(source)
                image = vtf.get().to_PIL()
                buffer = io.BytesIO()
                image.save(buffer, format="TGA", compression="tga_rle")
            path = f"textures/jg/{self.map_name}/t{len(self.images):03}"
            self.files[path+".tga"] = buffer.getvalue()
            self.images[name] = path
        return self.images[name]

    def add(self, key, flags, contents, body=""):
        if key not in self.ids:
            index = len(self.shaders)
            name = f"textures/jg/{self.map_name}/m{index:03}"
            self.ids[key] = index
            self.shaders.append(struct.pack("<64s2I", name.encode(), flags, contents))
            if body:
                self.definitions.append(f"{name}\n{{\n{body}\n}}")
        return self.ids[key]

    def render(self, name, flags, lit, displacement, atlas, *, model_tint=None):
        name = name.casefold()
        key = (name, flags, lit, displacement, atlas, model_tint)
        if key in self.ids:
            return self.ids[key]
        if flags & SOURCE_SKY:
            stem = self.sky()
            body = f"cull disable\n surfaceparm sky\n surfaceparm noimpact\n skyParms {stem} - -"
            return self.add(key, 0x2000, 0, body)
        if name == "glass/reflectiveglass001":
            body = ("portal\n cull disable\n"
                    " { map $whiteimage\n blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
                    "   rgbGen identity\n alphaGen const 0\n depthWrite\n }")
            return self.add(key, 0, 0, body)
        material = self.material(name)
        base = material.get("$basetexture", "") if material else ""
        second = material.get("$basetexture2", "") if material and displacement else ""
        water = bool(material and material.shader.casefold() == "water")
        if water and not base:
            base = material.get("$normalmap", "")
        texture = self.texture(base or name)
        if texture is None:
            self.missing.add(name)
            texture = "$whiteimage"
        secondary = self.texture(second) if second else None
        cutout = bool(material and material.get("$alphatest") == "1")
        translucent = bool(material and material.get("$translucent") == "1")
        body = " cull disable"
        if water:
            body += "\n surfaceparm water\n surfaceparm trans\n sort underwater"
            body += f"\n {{ map {texture}\n blendFunc blend\n rgbGen const ( 0.36 0.58 0.66 )\n alphaGen const 0.65\n }}"
        elif atlas and cutout:
            body += (f"\n {{ map {texture}\n alphaFunc GE128\n depthWrite\n rgbGen identity\n }}"
                     "\n { map $lightmap\n blendFunc filter\n depthFunc equal\n rgbGen identity\n }")
        elif atlas and not translucent:
            body += f"\n {{ map $lightmap\n rgbGen identity\n }}\n {{ map {texture}\n blendFunc filter\n rgbGen identity\n }}"
        elif model_tint is not None:
            modifier = "\n alphaFunc GE128\n depthWrite" if cutout else "\n blendFunc blend" if translucent else ""
            body += f"\n {{ map {texture}\n rgbGen lightingDiffuse{modifier}\n }}"
            if model_tint != (255, 255, 255):
                color = " ".join(f"{value/255:.5f}" for value in model_tint)
                body += ("\n { map $whiteimage\n blendFunc filter\n depthFunc equal"
                         f"\n rgbGen const ( {color} )\n }}")
        else:
            modifier = "\n alphaFunc GE128\n depthWrite" if cutout else "\n blendFunc blend" if translucent else ""
            body += f"\n {{ map {texture}\n rgbGen {'vertex' if lit else 'identity'}{modifier}\n }}"
            if secondary:
                body += f"\n {{ map {secondary}\n blendFunc blend\n rgbGen vertex\n alphaGen vertex\n }}"
        return self.add(key, 0, RBSP_WATER if water else 0, body)

    def decal(self, name):
        material = self.material(name)
        if not material or material.shader.casefold() != "lightmappedgeneric" or material.get("$decal") != "1":
            raise ValueError(f"Unsupported Source decal material {name}")
        texture = self.texture(material["$basetexture"])
        if texture is None:
            raise ValueError(f"Missing Source decal texture {name}")
        with Image.open(io.BytesIO(self.files[texture+".tga"])) as image:
            width, height = image.size
        scale = float(material.get("$decalscale", "1"))
        return texture, width*scale, height*scale

    def decal_shader(self, texture, layer):
        # Both native renderers reserve the interval from decal (4) to
        # see-through (5) for polygon-offset marks.
        body = (f"polygonOffset\n cull disable\n sort {4+layer*.001:.3f}\n"
                f" {{ clampMap {texture}\n blendFunc blend\n rgbGen exactVertex\n alphaGen identity\n }}")
        return self.add(("decal", texture, layer), 0, 0, body)

    def sky(self):
        if self.sky_stem:
            return self.sky_stem
        sky = next(e for e in self.source.entities if e.get("classname") == "env_skypaint")
        top = np.array([float(x) for x in sky["topcolor"].split()])
        horizon = np.array([float(x) for x in sky["bottomcolor"].split()])
        clouds_name = sky.get("startexture", "skybox/clouds")
        clouds_path = self.texture(clouds_name)
        clouds = None
        if clouds_path:
            with Image.open(io.BytesIO(self.files[clouds_path+".tga"])) as image:
                clouds = np.asarray(image.convert("RGBA"), dtype=np.float32)/255
        stem = self.sky_stem = f"textures/jg/{self.map_name}/sky"
        u, v = np.meshgrid((np.arange(512)+.5)/256-1, (np.arange(512)+.5)/256-1)
        one = np.ones_like(u)
        directions = {
            "rt": (one, -u, -v), "lf": (-one, u, -v), "ft": (u, one, -v),
            "bk": (-u, -one, -v), "up": (u, -v, one), "dn": (u, v, -one),
        }
        for face, (x, y, z) in directions.items():
            altitude = z/np.sqrt(x*x+y*y+z*z)
            mix = np.maximum(0, altitude)**.65
            rgb = horizon+(top-horizon)*mix[..., None]
            if clouds is not None and sky.get("drawstars") == "1":
                denom = np.maximum(.15, z)
                tx = np.mod(x/denom*.29+.5, 1)
                ty = np.mod(y/denom*.29+.5, 1)
                sample = clouds[(ty*clouds.shape[0]).astype(int), (tx*clouds.shape[1]).astype(int)]
                opacity = sample[..., 3]*np.max(sample[..., :3], axis=2)*np.maximum(0, altitude)*.65
                rgb = rgb*(1-opacity[..., None])+opacity[..., None]
            buffer = io.BytesIO()
            Image.fromarray(np.clip(rgb*255, 0, 255).astype(np.uint8)).save(buffer, format="TGA", compression="tga_rle")
            self.files[f"{stem}_{face}.tga"] = buffer.getvalue()
        return stem


class Lightmaps:
    def __init__(self):
        self.pages = []
        self.x = self.y = self.height = 0

    def add(self, width, height, rgb):
        if width > 126 or height > 126:
            return None
        if not self.pages:
            self.pages.append(Image.new("RGB", (128, 128)))
        if self.x+width+2 > 128:
            self.x = 0
            self.y += self.height
            self.height = 0
        if self.y+height+2 > 128:
            self.pages.append(Image.new("RGB", (128, 128)))
            self.x = self.y = self.height = 0
        page = self.pages[-1]
        x, y = self.x+1, self.y+1
        image = Image.frombytes("RGB", (width, height), rgb)
        page.paste(image, (x, y))
        for xx, side in ((x-1, 0), (x+width, width-1)):
            page.paste(image.crop((side, 0, side+1, height)), (xx, y))
        for yy, side in ((y-1, 0), (y+height, height-1)):
            page.paste(page.crop((x-1, y+side, x+width+1, y+side+1)), (x-1, yy))
        self.x += width+2
        self.height = max(self.height, height+2)
        return len(self.pages)-1, x, y

    def bytes(self):
        return b"".join(page.tobytes() for page in self.pages)


def sample_colors(blob, offset, width, height, flags, styles, switch_style):
    if offset < 0:
        return None, None
    size = width*height*4
    # Each style starts with the ordinary luxels followed by the bump basis.
    stride = size*(4 if flags & SOURCE_BUMPLIGHT else 1)
    if not styles or width <= 0 or height <= 0 or offset+(len(styles)-1)*stride+size > len(blob):
        raise ValueError("Invalid Source light samples")

    def tone(linear):
        return np.rint(255*np.minimum(1, linear/255)**.55).astype(np.uint8)

    base = np.zeros((width*height, 3))
    switched = None
    for layer, style in enumerate(styles):
        sample = np.frombuffer(blob, np.uint8, size, offset+layer*stride).reshape(-1, 4)
        linear = np.ldexp(sample[:, :3].astype(np.float64), sample[:, 3:].view(np.int8))
        if style == switch_style:
            switched = linear
        else:
            base += linear
    base_rgb = tone(base)
    # Add the tone-mapped difference in JA's native switchable style slot.
    delta = (tone(base+switched)-base_rgb).tobytes() if switched is not None else None
    return base_rgb.tobytes(), delta


class DecalReceiver:
    def __init__(self, corners, direction, offset, tx, face, width, height, rgb):
        self.point = offset_by(corners[0], offset, 1)
        self.direction = direction
        self.edges = []
        self.offset = offset
        self.bounds = tuple(tuple(offset[i]+bound(p[i] for p in corners) for i in range(3)) for bound in (min, max))
        self.vectors = tx[8:16]
        self.uv_origin = face[14:16]
        self.width, self.height = width, height
        self.rgb = rgb
        center = tuple(offset[i]+sum(p[i] for p in corners)/len(corners) for i in range(3))
        for a, b in zip(corners, corners[1:]+corners[:1]):
            edge = sub(b, a)
            if dot(edge, edge) < 1e-8:
                continue
            inward = normal(cross(direction, edge))
            a = offset_by(a, offset, 1)
            if dot(inward, sub(center, a)) < 0:
                inward = tuple(-v for v in inward)
            self.edges.append((a, inward))

    def distance(self, point):
        return dot(self.direction, sub(point, self.point))

    def contains(self, point):
        lo, hi = self.bounds
        if any(point[i] < lo[i]-4.01 or point[i] > hi[i]+4.01 for i in range(3)):
            return False
        return abs(self.distance(point)) <= 4.01 and all(
            dot(n, sub(point, a)) >= -.01 for a, n in self.edges)

    def lighting(self, point):
        if self.rgb is None:
            return 255, 255, 255
        local = sub(point, self.offset)
        coords = [min(size-1, max(0, dot(self.vectors[k:k+3], local)+self.vectors[k+3]-base))
                  for k, size, base in zip((0, 4), (self.width, self.height), self.uv_origin)]
        x, y = coords
        ix, iy = int(x), int(y)
        blend = [0., 0., 0.]
        for row, wy in ((iy, 1-y+iy), (min(iy+1, self.height-1), y-iy)):
            for col, wx in ((ix, 1-x+ix), (min(ix+1, self.width-1), x-ix)):
                first = (row*self.width+col)*3
                for channel, value in enumerate(self.rgb[first:first+3]):
                    blend[channel] += value*wx*wy
        return tuple(round(value) for value in blend)

    def project(self, corners, axis):
        alignment = dot(axis, self.direction)
        if abs(alignment) < .01:
            return []
        poly = [(*offset_by(point[:3], axis, -self.distance(point[:3])/alignment), *point[3:]) for point in corners]
        for origin, inward in self.edges:
            if not poly:
                break
            clipped = []
            a, da = poly[-1], dot(inward, sub(poly[-1][:3], origin))
            for b in poly:
                db = dot(inward, sub(b[:3], origin))
                if (da >= 0) != (db >= 0):
                    clipped.append(offset_by(a, sub(b, a), da/(da-db)))
                if db >= 0:
                    clipped.append(b)
                a, da = b, db
            poly = clipped
        return poly


def decal_corners(origin, u, v, bounds, uv):
    (left, bottom, right, top), (u0, u1, v0, v1) = bounds, uv
    return [(*offset_by(offset_by(origin, u, x), v, y), s, t)
            for x, y, s, t in ((left, bottom, u0, v0), (left, top, u0, v1),
                               (right, top, u1, v1), (right, bottom, u1, v0))]


def decal_triangles(poly, direction):
    # Add samples along long triangles so large signs follow the local lighting.
    pending = [(poly[0], poly[i], poly[i+1]) for i in range(1, len(poly)-1)]
    while pending:
        a, b, c = pending.pop()
        area = dot(cross(sub(b[:3], a[:3]), sub(c[:3], a[:3])), direction)
        if abs(area) < 1e-4:
            continue
        if area < 0:
            b, c = c, b
        length, p, q, r = max((dot(sub(p[:3], q[:3]), sub(p[:3], q[:3])), p, q, r)
                              for p, q, r in ((a, b, c), (b, c, a), (c, a, b)))
        if length > 16**2:
            middle = offset_by(p, sub(q, p), .5)
            pending.extend(((p, middle, r), (middle, q, r)))
        else:
            yield a, b, c


def baked_decals(source, textures, receivers, texinfo, texdata, names):
    entries, skipped = [], []
    for entity in source.entities:
        if entity.get("classname") != "infodecal":
            continue
        name = entity["texture"].casefold()
        origin = tuple(map(float, entity["origin"].split()))
        key = entity["hammerid"]
        primary = min((face for face in receivers.values() if face.contains(origin)),
                      key=lambda face: abs(face.distance(origin)), default=None)
        if primary is None:
            skipped.append(dict(id=key, material=name, origin=origin))
            continue
        texture, width, height = textures.decal(name)
        axis = primary.direction
        u = normal(cross((0, 0, 1), axis)) if abs(axis[2]) < .99 else (1, 0, 0)
        v = normal(cross(axis, u))
        quad = decal_corners(origin, u, v, (-width/2, -height/2, width/2, height/2), (0, 1, 1, 0))
        lo = tuple(min(p[i] for p in quad)-4.01 for i in range(3))
        hi = tuple(max(p[i] for p in quad)+4.01 for i in range(3))
        faces = [i for i, face in receivers.items() if dot(face.direction, axis) > .999
                 and abs(face.distance(origin)-primary.distance(origin)) < .125
                 and all(lo[a] <= face.bounds[1][a] and hi[a] >= face.bounds[0][a] for a in range(3))]
        entries.append(("decal", key, name, texture, axis, quad, faces))

    overlays = rows("ihH64i22f", source.lump(45))
    if source.lump(50) or any(fade != (-1, 0) for fade in rows("2f", source.lump(60))):
        raise ValueError("Unsupported water or distance-faded Source overlay")
    for row in sorted(overlays, key=lambda row: (row[2] >> 14, row[0])):
        key, ti, packed = row[:3]
        face_count = packed & 0x3fff
        if not 0 < face_count <= 64:
            raise ValueError(f"Invalid Source overlay {key} face count")
        name = names[texdata[texinfo[ti][17]][3]].casefold()
        texture, _, _ = textures.decal(name)
        axis = row[86:89]
        u = (row[73], row[76], row[79])
        v = cross(axis, u)
        if row[82]:
            v = tuple(-x for x in v)
        left, bottom, right, top = row[71], row[72], row[77], row[78]
        if (row[74], row[75], row[80], row[81]) != (left, top, right, bottom):
            raise ValueError(f"Unsupported non-rectangular Source overlay {key}")
        quad = decal_corners(row[83:86], u, v, (left, bottom, right, top), row[67:71])
        entries.append(("overlay", str(key), name, texture, axis, quad, row[3:3+face_count]))
    return entries, skipped


def brush_placements(source, models):
    classes = {"func_brush", "func_illusionary", "func_reflective_glass", "func_button"}
    placements = {}
    for entity in source.entities:
        model = entity.get("model", "")
        if not model.startswith("*") or entity.get("classname") not in classes:
            continue
        index = int(model[1:])
        origin = tuple(map(float, entity["origin"].split()))
        angles = tuple(map(float, entity.get("angles", "0 0 0").split()))
        if not 0 < index < len(models) or index in placements or any(angles):
            raise ValueError("Unsupported Source brush entity placement")
        placements[index] = entity, origin
    return placements


def light_switch(placements, entities):
    switches = [(index, entity, origin) for index, (entity, origin) in placements.items()
                if entity["classname"] == "func_button"]
    if not switches:
        return None
    (index, entity, origin), = switches
    target, action, argument, delay, repeat = entity["OnPressed"].split(",")
    if (action, argument, delay, repeat) != ("Toggle", "", "0", "-1"):
        raise ValueError("Unsupported Source light switch action")
    lights = [e for e in entities if e.get("targetname") == target and e.get("classname") == "light"]
    (style, starts_off), = {(int(e["style"]), bool(int(e.get("spawnflags", "0")) & 1)) for e in lights}
    if not 1 <= style < 64 or not int(entity["spawnflags"]) & 1024:
        raise ValueError("Invalid Source light switch settings")
    return index, entity, origin, style, starts_off


def displacement(corners, info, vertices):
    start, first, power = info[:3], info[3], info[5]
    rotate = min(range(4), key=lambda i: dot(sub(corners[i], start), sub(corners[i], start)))
    a, b, c, d = corners[rotate:]+corners[:rotate]
    size = 2**power+1
    points, alpha = [], []
    for row in range(size):
        t = row/(size-1)
        for col in range(size):
            s = col/(size-1)
            dx, dy, dz, dist, blend = vertices[first+row*size+col]
            base = tuple((1-t)*((1-s)*a[k]+s*d[k])+t*((1-s)*b[k]+s*c[k]) for k in range(3))
            points.append(tuple(base[k]+dist*v for k, v in enumerate((dx, dy, dz))))
            alpha.append(max(0, min(255, round(blend))))
    triangles = []
    for row in range(size-1):
        for col in range(size-1):
            i = row*size+col
            j, k, l = i+1, i+size, i+size+1
            triangles.extend(((i, k, l), (i, l, j)) if (row+col)%2==0 else ((i, k, j), (j, k, l)))
    return points, alpha, triangles


def native_wav(data, volume, pitch, channel=None):
    with wave.open(io.BytesIO(data), "rb") as stream:
        rate = stream.getframerate()
        channels = stream.getnchannels()
        width = stream.getsampwidth()
        samples = stream.readframes(stream.getnframes())
    if width == 1:
        pcm = (np.frombuffer(samples, np.uint8).astype(np.float32)-128)*256
    elif width == 2:
        pcm = np.frombuffer(samples, "<i2").astype(np.float32)
    else:
        raise ValueError("Unsupported Source WAV sample width")
    pcm = pcm.reshape(-1, channels)
    pcm = pcm.mean(axis=1) if channel is None else pcm[:, channel]
    indices = np.arange(0, len(pcm), rate*pitch/(22050*100))
    mono = np.clip(np.interp(indices, np.arange(len(pcm)), pcm)*volume, -32768, 32767).astype("<i2")
    output = io.BytesIO()
    with wave.open(output, "wb") as stream:
        stream.setparams((1, 2, 22050, 0, "NONE", "not compressed"))
        stream.writeframes(mono.tobytes())
    return output.getvalue()


def write_soundscapes(source, assets, name, files, shift):
    soundscapes = [entity for entity in source.entities if entity.get("classname") == "env_soundscape"]
    if not soundscapes:
        return None
    definitions = {}
    for script in ("hl2/soundscapes.txt", "soundscapes_garrysmod.txt"):
        data = assets.read("scripts/soundscapes/"+script)
        if data:
            definitions.update((group.name, group) for group in Keyvalues.parse(io.StringIO(data.decode("latin1"))))

    def layers(group, volume=1., parents=()):
        if group in parents or group not in definitions:
            raise ValueError(f"Missing or cyclic Source soundscape {group}")
        for entry in definitions[group]:
            if entry.name == "dsp":
                continue
            gain = volume*entry.float("volume")
            if entry.name == "playsoundscape":
                yield from layers(entry["name"].casefold(), gain, (*parents, group))
            elif entry.name == "playlooping" and "position" not in entry:
                yield entry["wave"].lstrip("@*#^").casefold(), entry.float("pitch"), gain
            else:
                raise ValueError(f"Unsupported Source soundscape operation {entry.name}")

    clips, clip_ids, profiles, profile_ids, zones = [], {}, [], {}, []
    for entity in soundscapes:
        if int(entity.get("StartDisabled", "0")):
            raise ValueError("Disabled Source soundscape needs its activation controller")
        group = entity["soundscape"].casefold()
        if group not in profile_ids:
            profile_ids[group] = len(profiles)
            profile = dict(name=group, layers=[])
            for sound, pitch, gain in layers(group):
                key = (sound, pitch)
                if key not in clip_ids:
                    data = assets.read("sound/"+sound)
                    if not data:
                        raise ValueError(f"Missing Source ambient loop {sound}")
                    with wave.open(io.BytesIO(data), "rb") as stream:
                        channels = stream.getnchannels()
                    if channels not in (1, 2) or not 0 < pitch <= 255:
                        raise ValueError(f"Unsupported Source ambient loop {sound}")
                    ids = clip_ids[key] = []
                    for channel in range(channels):
                        index = len(clips)
                        path = f"sound/jg/{name}/scene{index}.wav"
                        pan = 0 if channels == 1 else (2*channel-1)
                        files[path] = native_wav(data, 1, pitch, channel)
                        clips.append(dict(source=sound, pitch=pitch, path=path, pan=pan))
                        ids.append(index)
                if not 0 < gain <= 1:
                    raise ValueError(f"Invalid Source soundscape gain for {sound}")
                profile["layers"].extend((i, max(1, round(gain*255))) for i in clip_ids[key])
            profiles.append(profile)
        x, y, z = map(float, entity["origin"].split())
        zones.append(dict(origin=(x, y, z+shift), radius=float(entity["radius"]), profile=profile_ids[group]))
    lines = [f"JAR_SOUNDSCAPE 1 {len(clips)}", *(f'{c["pan"]} {c["path"]}' for c in clips), str(len(profiles))]
    for profile in profiles:
        entries = profile["layers"]
        lines.append(" ".join(map(str, [len(entries), *(value for layer in entries for value in layer)])))
    lines.append(str(len(zones)))
    for zone in zones:
        lines.append(" ".join(f"{v:g}" for v in (*zone["origin"], zone["radius"], zone["profile"])))
    files[f"maps/{name}.jarsound"] = ("\n".join(lines)+"\n").encode("ascii")
    return dict(clips=clips, profiles=profiles, zones=zones)


def rigid_prop_mass(read, path, scale):
    stem = path.removesuffix(".mdl")
    mdl, phy = read(path), read(stem+".phy")
    if not mdl or not phy or len(phy) < 20:
        raise ValueError(f"Missing rigid body for {path}")
    header_size, _, solids, checksum = struct.unpack_from("<4i", phy)
    if header_size != 16 or solids != 1 or checksum != struct.unpack_from("<i", mdl, 8)[0]:
        raise ValueError(f"Unsupported rigid body header for {path}")
    solid_size, = struct.unpack_from("<i", phy, header_size)
    if not 0 < solid_size < len(phy)-header_size-4:
        raise ValueError(f"Invalid rigid body size for {path}")
    metadata = phy[header_size+4+solid_size:].rstrip(b"\0").decode("ascii")
    properties = Keyvalues.parse(metadata, single_line=True).find_key("solid")
    mass = properties.float("mass") * (scale if scale > 0 else 1)
    if not math.isfinite(mass) or mass <= 0:
        raise ValueError(f"Invalid rigid body mass for {path}")
    return mass


def rigid_prop_md3(path, parts, textures, tint):
    surfaces, positions = [], []
    for material, vertices, triangles in parts:
        shader = textures.render(material, 0, False, False, False, model_tint=tint)
        shader_path = f"textures/jg/{textures.map_name}/m{shader:03}".encode()
        for first in range(0, len(triangles), 225):
            batch = triangles[first:first+225]
            used = dict.fromkeys(index for triangle in batch for index in triangle)
            local = {index: i for i, index in enumerate(used)}
            index_data = b"".join(struct.pack("<3i", *(local[index] for index in triangle))
                                  for triangle in batch)
            uv_data, vertex_data = bytearray(), bytearray()
            for index in used:
                vertex = vertices[index]
                xyz = tuple(round(value*64) for value in vertex[:3])
                positions.append(tuple(value/64 for value in xyz))
                nx, ny, nz = normal(vertex[3:6])
                yaw = round(math.atan2(ny, nx)*256/math.tau) & 255
                polar = round(math.acos(max(-1, min(1, nz)))*256/math.tau) & 255
                uv_data.extend(struct.pack("<2f", *vertex[6:8]))
                vertex_data.extend(struct.pack("<3hH", *xyz, (yaw << 8) | polar))
            triangle_at = 108
            shader_at = triangle_at+len(index_data)
            uv_at = shader_at+68
            vertex_at = uv_at+len(uv_data)
            header = struct.pack("<4s64s10i", b"IDP3", f"mesh{len(surfaces)}".encode(),
                                 0, 1, 1, len(used), len(batch), triangle_at, shader_at,
                                 uv_at, vertex_at, vertex_at+len(vertex_data))
            surfaces.append(header+index_data+struct.pack("<64si", shader_path, 0)+uv_data+vertex_data)
    if not positions or len(surfaces) > 64:
        raise ValueError(f"Unsupported rigid prop mesh for {path}")
    surface_data = b"".join(surfaces)
    lower = [min(p[axis] for p in positions) for axis in range(3)]
    upper = [max(p[axis] for p in positions) for axis in range(3)]
    radius = max(math.sqrt(dot(p, p)) for p in positions)
    frame = struct.pack("<10f16s", *lower, *upper, 0, 0, 0, radius, b"rigid")
    header = struct.pack("<4si64s9i", b"IDP3", 15, path.encode(), 0, 1, 0, len(surfaces),
                         0, 108, 164, 164, 164+len(surface_data))
    return header+frame+surface_data, positions


def convert(gmod, name, files):
    source_path = gmod/"garrysmod"/"maps"/(name+".bsp")
    source = SourceMap(source_path)
    textures = Materials(gmod, source, name, files)
    vecs = rows("3f", source.lump(3))
    original_planes = rows("4fi", source.lump(1))
    nodes = rows("3i6h2Hh2x", source.lump(5))
    leaves = rows("ihH6h4Hh2x", source.lump(10))
    face_indices = [x for x, in rows("H", source.lump(16))]
    brush_indices = [x for x, in rows("H", source.lump(17))]
    faces = rows("HBBihhhh4Bif4iiHHI", source.lump(7))
    edges = rows("2H", source.lump(12))
    surfedges = [x for x, in rows("i", source.lump(13))]
    texinfo = rows("16f2i", source.lump(6))
    texdata = rows("3f5i", source.lump(2))
    names = [source.lump(43)[x:].split(b"\0", 1)[0].decode("ascii")
             for x, in rows("i", source.lump(44))]
    models = rows("9f3i", source.lump(14))
    placements = brush_placements(source, models)
    switch = light_switch(placements, source.entities)
    switch_style = switch[3] if switch else None
    static_models = {i: origin for i, (_, origin) in placements.items() if not switch or i != switch[0]}
    face_offsets = {f: origin for i, origin in static_models.items()
                    for f in range(models[i][10], models[i][10]+models[i][11])}
    world_faces = models[0][10]+models[0][11]
    disps = rows("3f4ifiH138x", source.lump(26))
    disp_verts = rows("5f", source.lump(33))
    lights = source.lump(8)
    shift = 12288 if name == "gm_flatgrass" else 0
    atlas = Lightmaps()
    planes, plane_ids = [], {}
    def plane(direction, distance):
        key = tuple(round(v, 7) for v in direction)+(round(distance, 5),)
        if key not in plane_ids:
            inverse = tuple(-v for v in key)
            plane_ids[key] = len(planes)
            plane_ids[inverse] = len(planes)+1
            planes.extend((key, inverse))
        return plane_ids[key]
    plane_map = [plane(p[:3], p[3]+p[2]*shift) for p in original_planes]
    verts, indexes, surfaces, face_map, terrain, mirrors = [], [], [], {}, [], []
    decal_receivers = {}
    converted_faces = 0
    styled_faces, sunlight_faces = 0, 0
    for face_id, face in enumerate(faces):
        if face_id >= world_faces and face_id not in face_offsets:
            continue
        offset = face_offsets.get(face_id, (0, 0, 0))
        pnum, side, _, first, count, ti, disp = face[:7]
        if count < 3 or ti < 0:
            continue
        tx = texinfo[ti]
        flags = tx[16]
        if flags & SOURCE_NODRAW and not flags & SOURCE_SKY:
            continue
        data = texdata[tx[17]]
        material = names[data[3]]
        corners = [vecs[edges[abs(edge)][edge < 0]] for edge in surfedges[first:first+count]]
        if material.casefold() == "glass/reflectiveglass001":
            mirrors.append(tuple(offset[i]+sum(p[i] for p in corners)/len(corners) for i in range(3)))
        direction = tuple(v*(-1 if side else 1) for v in original_planes[pnum][:3])
        if disp >= 0:
            if len(corners) != 4:
                raise ValueError(f"Non-quad displacement in {name} face {face_id}")
            points, alpha, triangles = displacement(corners, disps[disp], disp_verts)
            terrain.append((face_id, points, triangles))
        else:
            points, alpha = corners, [255]*len(corners)
            triangles = [(0, i, i+1) for i in range(1, count-1)]
        width, height = face[16]+1, face[17]+1
        source_styles = tuple(i for i in face[8:12] if i != 255)
        rgb, switched = sample_colors(lights, face[12], width, height, flags, source_styles, switch_style)
        if disp < 0 and not flags & SOURCE_SKY and material.casefold() != "glass/reflectiveglass001":
            decal_receivers[face_id] = DecalReceiver(corners, direction, offset, tx, face, width, height, rgb)
        placed = atlas.add(width, height, rgb) if rgb and disp < 0 and not flags & SOURCE_SKY else None
        second = atlas.add(width, height, switched) if placed and switched else None
        if switched and not second:
            raise ValueError(f"Missing switchable lightmap for {name} face {face_id}")
        styled_faces += bool(second)
        sunlight_faces += rgb is not None and 33 in source_styles
        shader = textures.render(material, flags, rgb is not None, disp >= 0, placed is not None)
        # Split large terrain grids into index batches under native renderer limits.
        for batch_start in range(0, len(triangles), 225):
            batch = triangles[batch_start:batch_start+225]
            unique = dict.fromkeys(i for tri in batch for i in tri)
            local = {index: k for k, index in enumerate(unique)}
            vi, ii = len(verts), len(indexes)
            for point_index in unique:
                point = points[point_index]
                st = [(dot(tx[k:k+3], point)+tx[k+3])/max(1, data[4+k//4]) for k in (0, 4)]
                lux = [dot(tx[k:k+3], point)+tx[k+3]-face[14+(k-8)//4] for k in (8, 12)]
                lm = [(placed[i+1]+lux[i]+.5)/128 for i in range(2)] if placed else [0, 0]
                lm_switch = [(second[i+1]+lux[i]+.5)/128 for i in range(2)] if second else [0, 0]
                color = (255, 255, 255)
                if rgb and (disp >= 0 or not placed):
                    x, y = [max(0, min(limit-1, round(v))) for v, limit in zip(lux, (width, height))]
                    sample = (y*width+x)*3
                    color = tuple(rgb[sample:sample+3])
                verts.append(struct.pack("<16f16B", point[0]+offset[0], point[1]+offset[1], point[2]+offset[2]+shift, *st,
                                         *lm, *lm_switch, *([0]*4), *direction, *color, alpha[point_index], *([255]*12)))
            for a, b, c in batch:
                if dot(cross(sub(points[b], points[a]), sub(points[c], points[a])), direction) < 0:
                    b, c = c, b
                indexes.extend((local[a], local[b], local[c]))
            face_map.setdefault(face_id, []).append(len(surfaces))
            styles = (0, switch_style if second else 255, 255, 255)
            vertex_styles = (0, 255, 255, 255)
            light = placed[0] if placed else NO_LIGHTMAP
            surfaces.append(struct.pack("<7i8B14i12f2i", shader, -1, 1 if disp < 0 else 3,
                                        vi, len(unique), ii, len(batch)*3, *styles, *vertex_styles,
                                        light, second[0] if second else -1, -1, -1,
                                        placed[1] if placed else 0, second[1] if second else 0, 0, 0,
                                        placed[2] if placed else 0, second[2] if second else 0, 0, 0, width if placed else 0,
                                        height if placed else 0, *([0]*9), *direction, 0, 0))
        converted_faces += 1

    brushes, brushsides, source_hulls, empty_brushes = [], [], {}, []
    source_brushes = rows("3i", source.lump(18))
    source_sides = rows("H3h", source.lump(19))
    def make_brush(sides, contents):
        shader = textures.add(("brush", contents), 0, contents)
        index = len(brushes)
        brushes.append((len(brushsides), len(sides), shader))
        brushsides.extend((p, shader, -1) for p in sides)
        return index

    def axial(direction):
        for axis in range(3):
            if abs(direction[axis]) == 1 and all(direction[i] == 0 for i in range(3) if i != axis):
                return axis*2+int(direction[axis] > 0)
        return None

    for index, (start, count, flags) in enumerate(source_brushes):
        contents = (RBSP_SOLID | RBSP_OPAQUE) if flags & (1 | 2 | 0x4000) else 0
        contents |= RBSP_WATER if flags & (16 | 32) else 0
        contents |= RBSP_PLAYERCLIP if flags & 0x10000 else 0
        contents |= RBSP_MONSTERCLIP if flags & 0x20000 else 0
        if not contents:
            continue
        ordered, other = {}, []
        for pnum, _, _, _ in source_sides[start:start+count]:
            axis = axial(original_planes[pnum][:3])
            if axis is None:
                other.append(plane_map[pnum])
            else:
                prior = ordered.get(axis)
                candidate = plane_map[pnum]
                if prior is None or planes[candidate][3] < planes[prior][3]:
                    ordered[axis] = candidate
        if len(ordered) != 6:
            raise ValueError(f"Missing axial Source bevels in {name} brush {index}")
        if any(-planes[ordered[2*a]][3] > planes[ordered[2*a+1]][3] for a in range(3)):
            empty_brushes.append(index)
            continue
        source_hulls[index] = [ordered[i] for i in range(6)]+other, contents

    def model_leaves(model):
        pending = [model[9]]
        while pending:
            index = pending.pop()
            if index < 0:
                yield -index-1
            else:
                pending.extend(nodes[index][1:3])

    def model_brushes(leaf_ids):
        result = set()
        for leaf_id in leaf_ids:
            first, count = leaves[leaf_id][11:13]
            result.update(b for b in brush_indices[first:first+count] if b in source_hulls)
        return sorted(result)

    world_leaves = set(model_leaves(models[0]))
    brush_map = {index: make_brush(*source_hulls[index]) for index in model_brushes(world_leaves)}
    leaf_surfs, leaf_brushes = [], []
    for i, leaf in enumerate(leaves):
        first_face, num_faces, first_brush, num_brushes = leaf[9:13]
        sf = face_indices[first_face:first_face+num_faces]
        leaf_surfs.append([s for f in sf if f < world_faces for s in face_map.get(f, ())] if i in world_leaves else [])
        leaf_brushes.append([brush_map[b] for b in brush_indices[first_brush:first_brush+num_brushes]
                             if b in brush_map] if i in world_leaves else [])

    def touched(minimum, maximum, root=0):
        pending, result = [root], []
        while pending:
            index = pending.pop()
            if index < 0:
                result.append(-index-1)
                continue
            node = nodes[index]
            p = original_planes[node[0]]
            low = sum(min(p[a]*minimum[a], p[a]*maximum[a]) for a in range(3))
            high = sum(max(p[a]*minimum[a], p[a]*maximum[a]) for a in range(3))
            if high >= p[3]:
                pending.append(node[1])
            if low <= p[3]:
                pending.append(node[2])
        return result

    def ladder_box(low, high):
        sides = []
        for axis in range(3):
            for sign, end in ((-1, low[axis]), (1, high[axis])):
                normal = [0, 0, 0]
                normal[axis] = sign
                sides.append(plane(normal, sign*(end + (shift if axis == 2 else 0))))
        brush = make_brush(sides, RBSP_LADDER)
        for leaf in touched(low, high):
            leaf_brushes[leaf].append(brush)

    ladder_landings = [tuple(map(float, entity["origin"].split())) for entity in source.entities
                       if entity.get("classname") == "info_ladder_dismount"]
    for entity in source.entities:
        if entity.get("classname") != "func_useableladder":
            continue
        a, b = [tuple(map(float, entity[key].split())) for key in ("point0", "point1")]
        lo = [min(a[i], b[i])-(18 if i < 2 else 0) for i in range(3)]
        hi = [max(a[i], b[i])+(18 if i < 2 else 32) for i in range(3)]
        ladder_box(lo, hi)

        # Source moves the player to an authored dismount. Keep a continuous
        # sweep instead, with a shallow ladder volume across the upper lip.
        top = max((a, b), key=lambda point: point[2])
        landings = [point for point in ladder_landings
                    if 0 <= point[2] - top[2] <= 32 and math.dist(point[:2], top[:2]) <= 64]
        if landings:
            landing = min(landings, key=lambda point: math.dist(top, point))
            # Join the original volume above the shelf. End just before the
            # dismount marker so forward movement returns to floor friction.
            delta = sub(landing, top)
            horizontal = math.hypot(*delta[:2])
            if horizontal > 2:
                edge = offset_by(landing, (delta[0]/horizontal, delta[1]/horizontal, 0), -2)
                low = [min(top[i] - 12, edge[i]) for i in range(2)] + [top[2]-8]
                high = [max(top[i] + 12, edge[i]) for i in range(2)] + [top[2]+32]
                ladder_box(low, high)

    for face_id, points, triangles in terrain:
        minimum = [min(p[a] for p in points) for a in range(3)]
        maximum = [max(p[a] for p in points) for a in range(3)]
        # Flat displacement quads need one brush. Sloping displacement
        # triangles are backed by small convex prisms.
        if maximum[2]-minimum[2] < .001:
            pieces = [(minimum, maximum, None)]
        else:
            pieces = []
            for i, j, k in triangles:
                tri = [points[i], points[j], points[k]]
                lo = [min(p[a] for p in tri) for a in range(3)]
                hi = [max(p[a] for p in tri) for a in range(3)]
                pieces.append((lo, hi, tri))
        for lo, hi, tri in pieces:
            lo = [lo[0], lo[1], lo[2]-4]
            bounds = []
            for axis in range(3):
                for sign, end in ((-1, lo), (1, hi)):
                    n = [0, 0, 0]
                    n[axis] = sign
                    bounds.append(plane(n, sign*(end[axis]+(shift if axis==2 else 0))))
            if tri:
                center = [sum(p[a] for p in tri)/3 for a in range(3)]
                n = normal(cross(sub(tri[1], tri[0]), sub(tri[2], tri[0])))
                if n[2] < 0:
                    n = tuple(-v for v in n)
                bounds.append(plane(n, dot(n, tri[0])+n[2]*shift))
                for a, b in zip(tri, tri[1:]+tri[:1]):
                    n = normal((b[1]-a[1], a[0]-b[0], 0))
                    if dot(n, sub(center, a)) > 0:
                        n = tuple(-v for v in n)
                    bounds.append(plane(n, dot(n, a)))
            brush = make_brush(bounds, RBSP_SOLID | RBSP_OPAQUE)
            for leaf_id in touched(lo, hi):
                leaf_brushes[leaf_id].append(brush)
        # Include displaced geometry in any leaf crossed by its real bounds.
        for leaf_id in touched(minimum, maximum):
            leaf_surfs[leaf_id].extend(face_map[face_id])

    for index, offset in static_models.items():
        entity = placements[index][0]
        m = models[index]
        brush_ids = []
        if entity["classname"] != "func_illusionary" and entity.get("Solidity") != "1":
            for brush in model_brushes(model_leaves(m)):
                sides, contents = source_hulls[brush]
                translated = [plane(planes[p][:3], planes[p][3]+dot(planes[p][:3], offset)) for p in sides]
                brush_ids.append(make_brush(translated, contents))
        draw = [s for i in range(m[10], m[10]+m[11]) for s in face_map.get(i, ())]
        lo = tuple(m[i]+offset[i] for i in range(3))
        hi = tuple(m[i+3]+offset[i] for i in range(3))
        for li in touched(lo, hi):
            leaf_surfs[li].extend(draw)
            leaf_brushes[li].extend(brush_ids)

    props = [(p.model, tuple(p.origin), tuple(p.angles), p.skin, p.solidity, tuple(p.scaling), None)
             for p in BSP(source_path).props]
    props.extend((e["model"], tuple(map(float, e["origin"].split())),
                  tuple(map(float, e.get("angles", "0 0 0").split())), int(e.get("skin", "0")),
                  int(e.get("solid", "0")), (1, 1, 1), e)
                 for e in source.entities if e.get("classname") in {"prop_dynamic", "prop_physics"} and
                 e.get("model", "").startswith("models/") and e.get("origin"))
    model_cache, collision_cache, unavailable_models = {}, {}, {}
    native_props = []
    solid_scenery = []
    rendered_props = 0
    for path, origin, angles, skin, solidity, scale, entity in props:
        key = path.casefold(), skin
        if key not in model_cache:
            try:
                model_cache[key] = list(mesh_parts(path, skin, textures.read))
                if not model_cache[key]:
                    unavailable_models[path] = "no visible LOD0 meshes"
            except (ValueError, IndexError, struct.error) as error:
                model_cache[key] = []
                unavailable_models[path] = str(error)
        parts = model_cache[key]
        if not parts:
            if solidity:
                raise ValueError(f"Cannot render solid scenery {path}")
            continue
        axes = basis(angles)
        def rotated(p):
            return tuple(sum(p[i]*axes[i][j] for i in range(3)) for j in range(3))
        if entity and entity["classname"] == "prop_physics":
            model = f"models/jg/{name}/p{len(native_props):03}.md3"
            tint = tuple(map(int, entity.get("rendercolor", "255 255 255").split()))
            files[model], points = rigid_prop_md3(model, parts, textures, tint)
            points = [rotated(point) for point in points]
            lower = tuple(min(p[i] for p in points) for i in range(3))
            upper = tuple(max(p[i] for p in points) for i in range(3))
            mass = rigid_prop_mass(textures.read, path, float(entity.get("massScale", "1")))
            native_props.append(dict(source=path, model=model, mass=mass, angles=angles,
                                     origin=(origin[0], origin[1], origin[2]+shift),
                                     mins=lower, maxs=upper))
            continue
        if solidity not in (0, 6):
            raise ValueError(f"Unsupported scenery solidity {solidity} for {path}")
        if solidity == 6:
            if path not in collision_cache:
                collision_cache[path] = collision_pieces(path, textures.read)
            first_brush = len(brushes)
            offset = (origin[0], origin[1], origin[2]+shift)
            for piece in collision_cache[path]:
                triangles = [tuple(rotated(tuple(v*s for v, s in zip(point, scale))) for point in face)
                             for face in piece]
                supports, points = convex_brush(triangles)
                sides = [plane(n, d+dot(n, offset)) for n, d in supports]
                brush = make_brush(sides, RBSP_SOLID | RBSP_OPAQUE)
                low = [origin[a]+min(p[a] for p in points)-.001 for a in range(3)]
                high = [origin[a]+max(p[a] for p in points)+.001 for a in range(3)]
                for leaf in touched(low, high):
                    leaf_brushes[leaf].append(brush)
            solid_scenery.append(dict(source=path, origin=offset, angles=angles, scale=scale,
                                      firstBrush=first_brush, brushes=len(brushes)-first_brush))
        for material, model_vertices, triangles in parts:
            shader = textures.render(material, 0, True, False, False)
            for first in range(0, len(triangles), 225):
                batch = triangles[first:first+225]
                unique = dict.fromkeys(i for tri in batch for i in tri)
                local = {index: k for k, index in enumerate(unique)}
                start_vertex, start_index = len(verts), len(indexes)
                points = []
                for index in unique:
                    vertex = model_vertices[index]
                    rotated_point = rotated(tuple(v*s for v, s in zip(vertex[:3], scale)))
                    point = tuple(a+b for a, b in zip(origin, rotated_point))
                    points.append(point)
                    normal_at_vertex = rotated(vertex[3:6])
                    if scale != (1, 1, 1):
                        normal_at_vertex = normal(rotated(tuple(v/s for v, s in zip(vertex[3:6], scale))))
                    color = 210 + round(45*max(0, normal_at_vertex[2]))
                    verts.append(struct.pack("<16f16B", point[0], point[1], point[2]+shift, *vertex[6:8],
                                             *([0]*8), *normal_at_vertex, color, color, color, 255, *([255]*12)))
                for a, b, c in batch:
                    indexes.extend((local[a], local[b], local[c]))
                low = [min(p[a] for p in points) for a in range(3)]
                high = [max(p[a] for p in points) for a in range(3)]
                for li in touched(low, high):
                    leaf_surfs[li].append(len(surfaces))
                styles = (0, 255, 255, 255)
                surfaces.append(struct.pack("<7i8B14i12f2i", shader, -1, 3, start_vertex, len(unique),
                                            start_index, len(batch)*3, *styles, *styles, NO_LIGHTMAP, -1, -1, -1,
                                            *([0]*10), *([0.0]*12), 0, 0))
        rendered_props += 1

    decals, unattached_decals = baked_decals(source, textures, decal_receivers, texinfo, texdata, names)
    used_receivers = {face for *_, receivers in decals for face in receivers}
    by_surface = {face_map[face][0]: face for face in used_receivers}
    receiving_leaves = {face: [] for face in used_receivers}
    for leaf, draw in enumerate(leaf_surfs):
        for surface in draw:
            face = by_surface.get(surface)
            if face is not None:
                receiving_leaves[face].append(leaf)
    decal_report = []
    for layer, (kind, key, material, texture, axis, quad, faces_for_decal) in enumerate(decals):
        shader = textures.decal_shader(texture, layer)
        first = len(surfaces)
        fragments = []
        for face_id in faces_for_decal:
            receiver = decal_receivers[face_id]
            polygon = receiver.project(quad, axis)
            triangles = list(decal_triangles(polygon, receiver.direction))
            if not triangles:
                continue
            fragments.append(face_id)
            for start in range(0, len(triangles), 225):
                batch = triangles[start:start+225]
                points = dict.fromkeys(p for triangle in batch for p in triangle)
                local = {p: i for i, p in enumerate(points)}
                vi, ii = len(verts), len(indexes)
                for x, y, z, u, v in points:
                    color = receiver.lighting((x, y, z))
                    verts.append(struct.pack("<16f16B", x, y, z+shift, u, v, *([0]*8),
                                             *receiver.direction, *color, 255, *([255]*12)))
                indexes.extend(local[p] for triangle in batch for p in triangle)
                styles = (0, 255, 255, 255)
                for leaf in receiving_leaves[face_id]:
                    leaf_surfs[leaf].append(len(surfaces))
                surfaces.append(struct.pack("<7i8B14i12f2i", shader, -1, 3, vi, len(points), ii,
                                            len(batch)*3, *styles, *styles, NO_LIGHTMAP, -1, -1, -1,
                                            *([0]*10), *([0.0]*12), 0, 0))
        if not fragments:
            raise ValueError(f"No receiving geometry for Source {kind} {key}")
        decal_report.append(dict(type=kind, id=key, material=material, receiverFaces=fragments,
                                 firstSurface=first, surfaces=len(surfaces)-first))

    node_bytes = b"".join(struct.pack("<9i", plane_map[n[0]], *n[1:5], n[5]+shift, *n[6:8], n[8]+shift)
                          for n in nodes)
    out_leaves, out_leaf_surfs, out_leaf_brushes = [], [], []
    for leaf, sf, br in zip(leaves, leaf_surfs, leaf_brushes):
        sf, br = sorted(set(sf)), sorted(set(br))
        out_leaves.append(struct.pack("<12i", leaf[1], 0, *leaf[3:5], leaf[5]+shift, *leaf[6:8],
                                      leaf[8]+shift, len(out_leaf_surfs), len(sf), len(out_leaf_brushes), len(br)))
        out_leaf_surfs.extend(sf)
        out_leaf_brushes.extend(br)

    world = ['{\n"classname" "worldspawn"\n"message" "'+name+'"\n"ambient" "80"\n}']
    sky_camera = next(e for e in source.entities if e.get("classname") == "sky_camera")
    x, y, z = map(float, sky_camera["origin"].split())
    world.append(f'{{\n"classname" "misc_skyportal"\n"origin" "{x:g} {y:g} {z+shift:g}"\n}}')
    for x, y, z in mirrors:
        world.append(f'{{\n"classname" "misc_portal_surface"\n"origin" "{x:g} {y:g} {z+shift:g}"\n}}')
    world_brush_count = len(brushes)
    inline_models = []
    if switch:
        index, button, (x, y, z), style, starts_off = switch
        model = models[index]
        first_brush = len(brushes)
        for brush in model_brushes(model_leaves(model)):
            sides, contents = source_hulls[brush]
            unshifted = [plane(planes[p][:3], planes[p][3]-planes[p][2]*shift) for p in sides]
            make_brush(unshifted, contents)
        if first_brush == len(brushes) or model[11]:
            raise ValueError("Unexpected Source light switch model")
        inline_models.append(struct.pack("<6f4i", *model[:6], len(surfaces), 0,
                                         first_brush, len(brushes)-first_brush))
        world.append(f'{{\n"classname" "func_usable"\n"model" "*1"\n"origin" "{x:g} {y:g} {z+shift:g}"\n'
                     f'"spawnflags" "72"\n"target" "jg_room_lights"\n"wait" "{float(button["wait"]):g}"\n}}')
        world.append(f'{{\n"classname" "light"\n"targetname" "jg_room_lights"\n"style" "{style}"\n'
                     f'"spawnflags" "{4 if starts_off else 0}"\n}}')
        script = textures.read("scripts/sounds/hl2_game_sounds_world.txt")
        definitions = Keyvalues.parse(io.StringIO(script.decode("latin1")))
        cue = definitions.find_key("Buttons.snd"+button["sounds"])
        source_audio = textures.read("sound/"+cue["wave"].lstrip("@*#^"))
        sound = f"sound/jg/{name}/room_switch.wav"
        files[sound] = native_wav(source_audio, cue.float("volume"), cue.float("pitch"))
        world.append(f'{{\n"classname" "target_speaker"\n"targetname" "jg_room_lights"\n'
                     f'"origin" "{x:g} {y:g} {z+shift:g}"\n"noise" "{sound}"\n}}')
    starts = [e for e in source.entities if e.get("classname") == "info_player_start"]
    for i, ent in enumerate(starts):
        x, y, z = map(float, ent["origin"].split())
        _, yaw, _ = map(float, ent.get("angles", "0 0 0").split())
        kind = "info_player_start" if i == 0 else "info_player_deathmatch"
        world.append(f'{{\n"classname" "{kind}"\n"origin" "{x:g} {y:g} {z+shift+32:g}"\n"angle" "{yaw:g}"\n}}')
    soundscape = write_soundscapes(source, textures, name, files, shift)
    # Load floor-supported furniture before any props stacked on top.
    for prop in sorted(native_props, key=lambda p: p["origin"][2]+p["mins"][2]):
        fields = ['"classname" "misc_model_breakable"', '"spawnflags" "1"', '"gravity" "1"',
                  f'"model" "{prop["model"]}"', f'"mass" "{prop["mass"]:g}"']
        for key in ("origin", "angles", "mins", "maxs"):
            vector = " ".join(f"{value:.6f}" for value in prop[key])
            fields.append(f'"{key}" "{vector}"')
        world.append("{\n"+"\n".join(fields)+"\n}")

    vis = source.lump(4)
    clusters = struct.unpack_from("<i", vis)[0]
    width = (clusters+7)//8
    target_vis = bytearray(struct.pack("<2i", clusters, width))
    for i in range(clusters):
        offset = struct.unpack_from("<i", vis, 4+8*i)[0]
        row = bytearray()
        if offset < 0:
            row.extend(b"\xff"*width)
        while len(row) < width:
            byte = vis[offset]
            offset += 1
            if byte:
                row.append(byte)
            else:
                run = vis[offset]
                offset += 1
                if not run:
                    raise ValueError("Invalid Source PVS zero run")
                row.extend(bytes(run))
        target_vis.extend(row[:width])
    m = models[0]
    def packed(fmt, seq):
        return b"".join(struct.pack("<"+fmt, *v) for v in seq)
    lumps = [
        ("\n".join(world)+"\n\0").encode(), b"".join(textures.shaders), packed("4f", planes), node_bytes,
        b"".join(out_leaves), packed("i", ((v,) for v in out_leaf_surfs)),
        packed("i", ((v,) for v in out_leaf_brushes)),
        struct.pack("<6f4i", *m[:2], m[2]+shift, *m[3:5], m[5]+shift, 0, len(surfaces), 0, world_brush_count)
        + b"".join(inline_models),
        packed("3i", brushes), packed("3i", brushsides), b"".join(verts),
        packed("i", ((v,) for v in indexes)), b"", b"".join(surfaces), atlas.bytes(), b"", target_vis, b"",
    ]
    header = bytearray(b"RBSP"+struct.pack("<i", 1))
    payload = bytearray()
    for lump in lumps:
        header.extend(struct.pack("<2i", 8+len(lumps)*8+len(payload), len(lump)))
        payload.extend(lump)
        payload.extend(bytes((-len(payload)) % 4))
    files[f"maps/{name}.bsp"] = header+payload
    files[f"shaders/jg_{name}.shader"] = ("\n\n".join(textures.definitions)+"\n").encode()
    files[f"scripts/jg_{name}.arena"] = f'{{\nmap "{name}"\nlongname "{name}"\ntype "ffa"\n}}\n'.encode()
    source_thumbnail = textures.read(f"maps/thumb/{name}.png")
    if source_thumbnail:
        buffer = io.BytesIO()
        with Image.open(io.BytesIO(source_thumbnail)) as image:
            ImageOps.fit(image.convert("RGB"), (144, 96), Image.Resampling.LANCZOS).save(buffer, format="PNG")
        files[f"levelshots/{name}.png"] = buffer.getvalue()
    return dict(map=name, sourceFaces=converted_faces, drawSurfaces=len(surfaces), collisionBrushes=len(brushes),
                displacements=len(terrain), starts=len(starts), lightmaps=len(atlas.pages), textures=len(textures.images),
                emptySourceBrushes=empty_brushes, translatedBrushModels=len(static_models),
                switchedLightmapFaces=styled_faces, sunlightFaces=sunlight_faces,
                decals=decal_report, unattachedDecals=unattached_decals,
                sceneryInstances=rendered_props, solidScenery=solid_scenery,
                nativeProps=native_props, sourcePropInstances=len(props),
                unavailableModels=unavailable_models,
                ambientSpeakers=0, ambientLoopFiles=len(soundscape["clips"]) if soundscape else 0,
                soundscape=soundscape, authoredLevelshot=bool(source_thumbnail),
                missingMaterials=sorted(textures.missing),
                sourceEntities=dict(Counter(e["classname"] for e in source.entities if "classname" in e)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gmod", type=Path, required=True, help="GarrysMod installation directory")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    files = {}
    results = [convert(args.gmod, name, files) for name in MAPS]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", zipfile.ZIP_DEFLATED, compresslevel=5) as archive:
        for name, data in sorted(files.items()):
            entry = zipfile.ZipInfo(name)
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)
    report = args.output.with_suffix(".json")
    report.write_text(json.dumps(results, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
