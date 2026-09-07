#!/usr/bin/env python3
import struct
import zlib
import math
import os

def create_png(w, h, rgba_bytes):
    def chunk(name, data):
        c = name + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw.extend(rgba_bytes[y*w*4:(y+1)*w*4])
    idat = chunk(b'IDAT', zlib.compress(bytes(raw), 9))
    iend = chunk(b'IEND', b'')
    return sig + ihdr + idat + iend

def create_dib_entry(s, bgra_bytes):
    header = struct.pack('<IIIHHIIIIII', 40, s, s * 2, 1, 32, 0, s * s * 4, 0, 0, 0, 0)
    row_bytes = ((s + 31) // 32) * 4
    and_mask = b'\x00' * (row_bytes * s)
    return header + bgra_bytes + and_mask

def render_icon(size):
    scale = 2
    sw = size * scale
    sh = size * scale
    rgba = bytearray(sw * sh * 4)
    corner_r = 0.22
    pad = 0.05
    
    for y in range(sh):
        ny = y / sh
        for x in range(sw):
            nx = x / sw
            idx = (y * sw + x) * 4
            
            bx = (nx - pad) / (1.0 - 2 * pad)
            by = (ny - pad) / (1.0 - 2 * pad)
            
            if bx < 0 or bx > 1 or by < 0 or by > 1:
                continue
                
            cx = max(corner_r - bx, 0, bx - (1 - corner_r))
            cy = max(corner_r - by, 0, by - (1 - corner_r))
            dist = math.sqrt(cx*cx + cy*cy)
            
            if dist > corner_r:
                continue
                
            bg_r = int(13 + (22 - 13) * ny)
            bg_g = int(17 + (27 - 17) * ny)
            bg_b = int(23 + (34 - 23) * ny)
            bg_a = 255
            
            r, g, b, a = bg_r, bg_g, bg_b, bg_a
            
            if 0.14 <= bx <= 0.52 and 0.14 <= by <= 0.52:
                tx = (bx - 0.14) / 0.38
                ty = (by - 0.14) / 0.38
                highlight = 0.3 * (1.0 - ty) + 0.2 * (1.0 - tx)
                r = min(255, int(31 * (1.0 + highlight) + 40 * highlight))
                g = min(255, int(111 * (1.0 + highlight) + 60 * highlight))
                b = min(255, int(235 * (1.0 + highlight) + 30 * highlight))
                if tx < 0.04 or tx > 0.96 or ty < 0.04 or ty > 0.96:
                    r, g, b = int(r * 0.7), int(g * 0.7), int(b * 0.7)
                    
            elif 0.14 <= bx <= 0.52 and 0.56 <= by <= 0.86:
                tx = (bx - 0.14) / 0.38
                ty = (by - 0.56) / 0.30
                highlight = 0.3 * (1.0 - ty) + 0.2 * (1.0 - tx)
                r = min(255, int(35 * (1.0 + highlight) + 30 * highlight))
                g = min(255, int(134 * (1.0 + highlight) + 60 * highlight))
                b = min(255, int(54 * (1.0 + highlight) + 30 * highlight))
                if tx < 0.04 or tx > 0.96 or ty < 0.05 or ty > 0.95:
                    r, g, b = int(r * 0.7), int(g * 0.7), int(b * 0.7)

            elif 0.56 <= bx <= 0.86 and 0.14 <= by <= 0.46:
                tx = (bx - 0.56) / 0.30
                ty = (by - 0.14) / 0.32
                highlight = 0.3 * (1.0 - ty) + 0.2 * (1.0 - tx)
                r = min(255, int(137 * (1.0 + highlight) + 60 * highlight))
                g = min(255, int(87 * (1.0 + highlight) + 40 * highlight))
                b = min(255, int(229 * (1.0 + highlight) + 40 * highlight))
                if tx < 0.05 or tx > 0.95 or ty < 0.05 or ty > 0.95:
                    r, g, b = int(r * 0.7), int(g * 0.7), int(b * 0.7)

            elif 0.56 <= bx <= 0.86 and 0.50 <= by <= 0.86:
                tx = (bx - 0.56) / 0.30
                ty = (by - 0.50) / 0.36
                highlight = 0.3 * (1.0 - ty) + 0.2 * (1.0 - tx)
                r = min(255, int(210 * (1.0 + highlight) + 50 * highlight))
                g = min(255, int(153 * (1.0 + highlight) + 40 * highlight))
                b = min(255, int(34 * (1.0 + highlight) + 20 * highlight))
                if tx < 0.05 or tx > 0.95 or ty < 0.05 or ty > 0.95:
                    r, g, b = int(r * 0.7), int(g * 0.7), int(b * 0.7)

            if dist > corner_r - 0.03 or bx < 0.02 or bx > 0.98 or by < 0.02 or by > 0.98:
                r, g, b = 48, 54, 61
                
            rgba[idx] = r
            rgba[idx+1] = g
            rgba[idx+2] = b
            rgba[idx+3] = a
            
    final_rgba = bytearray(size * size * 4)
    final_bgra = bytearray(size * size * 4)
    
    for y in range(size):
        for x in range(size):
            r_acc, g_acc, b_acc, a_acc = 0, 0, 0, 0
            for sy in range(2):
                for sx in range(2):
                    src_idx = ((y * 2 + sy) * sw + (x * 2 + sx)) * 4
                    r_acc += rgba[src_idx]
                    g_acc += rgba[src_idx+1]
                    b_acc += rgba[src_idx+2]
                    a_acc += rgba[src_idx+3]
                    
            f_idx = (y * size + x) * 4
            final_rgba[f_idx] = r_acc // 4
            final_rgba[f_idx+1] = g_acc // 4
            final_rgba[f_idx+2] = b_acc // 4
            final_rgba[f_idx+3] = a_acc // 4
            
            dib_idx = ((size - 1 - y) * size + x) * 4
            final_bgra[dib_idx] = b_acc // 4
            final_bgra[dib_idx+1] = g_acc // 4
            final_bgra[dib_idx+2] = r_acc // 4
            final_bgra[dib_idx+3] = a_acc // 4
            
    return bytes(final_rgba), bytes(final_bgra)

def build_ico(output_path):
    sizes = [16, 32, 48, 64, 128, 256]
    image_entries = []
    
    for s in sizes:
        rgba, bgra = render_icon(s)
        if s <= 48:
            data = create_dib_entry(s, bgra)
        else:
            data = create_png(s, s, rgba)
        image_entries.append((s, data))
        
    ico_header = struct.pack('<HHH', 0, 1, len(image_entries))
    offset = 6 + 16 * len(image_entries)
    
    dir_entries = []
    for s, data in image_entries:
        w_b = 0 if s == 256 else s
        h_b = 0 if s == 256 else s
        entry = struct.pack('<BBBBHHII', w_b, h_b, 0, 0, 1, 32, len(data), offset)
        dir_entries.append(entry)
        offset += len(data)
        
    with open(output_path, 'wb') as f:
        f.write(ico_header)
        for e in dir_entries:
            f.write(e)
        for _, data in image_entries:
            f.write(data)
            
    print(f"Successfully generated {output_path} with sizes: {sizes}")

if __name__ == '__main__':
    os.makedirs('resources', exist_ok=True)
    build_ico('resources/app.ico')
