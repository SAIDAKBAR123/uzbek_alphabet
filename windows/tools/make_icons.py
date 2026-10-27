#!/usr/bin/env python3
"""Собирает app.ico и app-off.ico (серый — замена выключена) из PNG расширения.

Usage: make_icons.py <uzb_ext/icons> <out-dir>

Без сторонних библиотек: PNG (8-bit RGBA) разбирается вручную, в ICO кладутся PNG-кадры
(Windows Vista+ поддерживает их для всех размеров).
"""
import os
import struct
import sys
import zlib

SIZES = (16, 32, 48, 128)


def read_png(path):
    with open(path, "rb") as f:
        data = f.read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", path
    pos, idat = 8, b""
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", body)
            assert (depth, color, interlace) == (8, 6, 0), f"{path}: need 8-bit RGBA, non-interlaced"
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length

    raw, stride, bpp = zlib.decompress(idat), width * 4, 4
    pixels, prev = bytearray(), bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        kind, row = raw[start], bytearray(raw[start + 1:start + 1 + stride])
        for i in range(stride):
            a = row[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if kind == 1:
                row[i] = (row[i] + a) & 0xFF
            elif kind == 2:
                row[i] = (row[i] + b) & 0xFF
            elif kind == 3:
                row[i] = (row[i] + (a + b) // 2) & 0xFF
            elif kind == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                row[i] = (row[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        pixels += row
        prev = row
    return width, height, pixels


def write_png(width, height, pixels):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    stride = width * 4
    raw = b"".join(b"\0" + bytes(pixels[y * stride:(y + 1) * stride]) for y in range(height))
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 9))
            + chunk(b"IEND", b""))


def grayscale(pixels):
    out = bytearray(pixels)
    for i in range(0, len(out), 4):
        r, g, b = out[i:i + 3]
        # Светлее исходного, чтобы «выключенный» значок читался и на тёмной панели задач.
        luma = int(0.299 * r + 0.587 * g + 0.114 * b)
        out[i:i + 3] = bytes([min(255, luma + 40)] * 3)
    return out


def write_ico(path, frames):
    header = struct.pack("<HHH", 0, 1, len(frames))
    offset, entries, images = 6 + 16 * len(frames), b"", b""
    for size, png in frames:
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(png), offset)
        images += png
        offset += len(png)
    with open(path, "wb") as f:
        f.write(header + entries + images)


def main():
    src, out = sys.argv[1], sys.argv[2]
    on, off = [], []
    for size in SIZES:
        path = os.path.join(src, f"icon-{size}.png")
        with open(path, "rb") as f:
            on.append((size, f.read()))
        width, height, pixels = read_png(path)
        off.append((size, write_png(width, height, grayscale(pixels))))
    os.makedirs(out, exist_ok=True)
    write_ico(os.path.join(out, "app.ico"), on)
    write_ico(os.path.join(out, "app-off.ico"), off)


if __name__ == "__main__":
    main()
