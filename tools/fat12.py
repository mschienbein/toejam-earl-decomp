"""Read files out of FAT12 floppy images (the archived Microtec distribution disks)."""
from __future__ import annotations

import struct


class Fat12Error(ValueError):
    pass


def extract(image: bytes) -> dict[str, bytes]:
    """Every file on the image, keyed by its DOS path ("68000/MCC68KAB.LIB")."""
    if len(image) < 512 or (image[510:512] != b"\x55\xaa" and image[0] not in (0xE9, 0xEB)):
        raise Fat12Error("not a DOS boot sector")
    bytes_per_sector, sectors_per_cluster, reserved, fats, root_entries, _, _, fat_sectors = \
        struct.unpack_from("<HBHBHHBH", image, 11)
    if bytes_per_sector not in (512, 1024) or not sectors_per_cluster or not fat_sectors:
        raise Fat12Error("implausible BIOS parameter block")
    fat = image[reserved * bytes_per_sector:(reserved + fat_sectors) * bytes_per_sector]
    root = (reserved + fats * fat_sectors) * bytes_per_sector
    data = root + (root_entries * 32 + bytes_per_sector - 1) // bytes_per_sector * bytes_per_sector
    cluster_size = sectors_per_cluster * bytes_per_sector

    def next_cluster(cluster: int) -> int:
        offset = cluster * 3 // 2
        value = fat[offset] | fat[offset + 1] << 8
        return value >> 4 if cluster & 1 else value & 0xFFF

    def chain(cluster: int, size: int | None) -> bytes:
        out, seen = bytearray(), set()
        while 2 <= cluster < 0xFF8:
            if cluster in seen:
                raise Fat12Error("cluster loop")
            seen.add(cluster)
            start = data + (cluster - 2) * cluster_size
            out += image[start:start + cluster_size]
            cluster = next_cluster(cluster)
        return bytes(out if size is None else out[:size])

    files: dict[str, bytes] = {}

    def walk(table: bytes, prefix: str) -> None:
        for k in range(0, len(table) - 31, 32):
            entry = table[k:k + 32]
            if entry[0] == 0:
                break
            if entry[0] == 0xE5 or entry[11] == 0x0F or entry[11] & 0x08:
                continue  # deleted, long-name or volume-label entries
            name = entry[0:8].decode("latin-1").rstrip()
            ext = entry[8:11].decode("latin-1").rstrip()
            full = name + ("." + ext if ext else "")
            if full in (".", ".."):
                continue
            cluster, size = struct.unpack_from("<H", entry, 26)[0], struct.unpack_from("<I", entry, 28)[0]
            if entry[11] & 0x10:
                walk(chain(cluster, None), prefix + full + "/")
            else:
                files[prefix + full] = chain(cluster, size)

    walk(image[root:root + root_entries * 32], "")
    return files
