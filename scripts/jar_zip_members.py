"""Write already-compressed PK3 members without inflating or compressing them again."""
from zipfile import ZIP_DEFLATED, ZipInfo
import zlib


def deflate_member(name, data, date):
    info = ZipInfo(name, date)
    info.compress_type = ZIP_DEFLATED
    info.external_attr = 0o600 << 16
    info.file_size = len(data)
    info.CRC = zlib.crc32(data)
    compressor = zlib.compressobj(6, wbits=-15)
    compressed = compressor.compress(data) + compressor.flush()
    info.compress_size = len(compressed)
    return info, compressed


def append_member(archive, info, blocks):
    # CPython's seekable ZipFile owns the central directory. These generated
    # members use ordinary local headers, without data descriptors or ZIP64.
    if info.flag_bits or max(info.file_size, info.compress_size) >= 2**31:
        raise ValueError(f"Unsupported compressed member: {info.filename}")
    info.header_offset = archive.fp.tell()
    archive.fp.write(info.FileHeader())
    start = archive.fp.tell()
    for block in blocks:
        archive.fp.write(block)
    if archive.fp.tell() - start != info.compress_size:
        raise ValueError(f"Incomplete compressed member: {info.filename}")
    archive.start_dir = archive.fp.tell()
    archive.filelist.append(info)
    archive.NameToInfo[info.filename] = info
    archive._didModify = True
