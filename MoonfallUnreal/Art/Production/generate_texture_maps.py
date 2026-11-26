"""Generate original seamless material texture sources using Python stdlib.

Run from any directory. Outputs are deterministic, local PNG files, not
third-party textures. Existing different files are never overwritten.
"""
import argparse
import json
import math
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parent
NAMES = ("bronze", "leather", "steel", "stone", "moonstone")

def clamp(x):
    return max(0.0, min(1.0, x))

def srgb(x):
    x = clamp(x)
    return 12.92*x if x <= .0031308 else 1.055*x**(1/2.4)-.055

def byte(x):
    return round(clamp(x)*255)

def lattice(x,y,period,seed):
    h = ((x%period)*374761393 + (y%period)*668265263 + seed*1274126177) & 0xffffffff
    h = ((h ^ (h>>13))*1274126177) & 0xffffffff
    return (h ^ (h>>16))/4294967295

def noise(u,v,period,seed):
    x,y=u*period,v*period
    ix,iy=math.floor(x),math.floor(y)
    tx,ty=x-ix,y-iy
    tx=tx*tx*(3-2*tx)
    ty=ty*ty*(3-2*ty)
    a=lattice(ix,iy,period,seed)*(1-tx)+lattice(ix+1,iy,period,seed)*tx
    b=lattice(ix,iy+1,period,seed)*(1-tx)+lattice(ix+1,iy+1,period,seed)*tx
    return a*(1-ty)+b*ty

def height_at(name,u,v,seed):
    coarse=noise(u,v,8,seed)
    mid=noise(u,v,32,seed+1)
    fine=noise(u,v,128,seed+2)
    if name=="steel":
        return .56*noise(u,v,64,seed+3)+.24*mid+.20*fine
    if name=="bronze":
        return .40*coarse+.36*mid+.24*fine
    if name=="leather":
        return .28*coarse+.50*abs(2*mid-1)+.22*fine
    if name=="stone":
        return .42*coarse+.37*mid+.21*fine
    return .55*coarse+.30*mid+.15*fine

def chunk(kind,payload):
    return struct.pack(">I",len(payload))+kind+payload+struct.pack(">I",zlib.crc32(kind+payload)&0xffffffff)

def png(size,data):
    stride=size*3
    raw=b"".join(b"\0"+data[y*stride:(y+1)*stride] for y in range(size))
    return (b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",size,size,8,2,0,0,0))
            +chunk(b"IDAT",zlib.compress(raw,9))+chunk(b"IEND",b""))

def render(name,profile,size):
    seed=101+NAMES.index(name)*71
    heights=[height_at(name,x/size,y/size,seed) for y in range(size) for x in range(size)]
    base,normal,orm=bytearray(),bytearray(),bytearray()
    strength=profile["normal_strength"]
    for y in range(size):
        for x in range(size):
            i=y*size+x
            h=heights[i]
            dx=(heights[y*size+(x+1)%size]-heights[y*size+(x-1)%size])*strength*8
            dy=(heights[((y+1)%size)*size+x]-heights[((y-1)%size)*size+x])*strength*8
            length=math.sqrt(dx*dx+dy*dy+1)
            normal.extend((byte(.5-.5*dx/length),byte(.5-.5*dy/length),byte(.5+.5/length)))
            variation=.76+.48*h
            base.extend(byte(srgb(c*variation)) for c in profile["base_color"])
            rough=clamp(profile["roughness"]+(.5-h)*.24)
            ao=clamp(.84+.16*h)
            orm.extend((byte(ao),byte(rough),byte(profile["metallic"])))
    return {"BaseColor":png(size,base),"Normal":png(size,normal),"ORM":png(size,orm)}

def generate(size,output):
    if size not in (64,128,256,512,1024):
        raise ValueError("Texture size must be 64, 128, 256, 512 or 1024.")
    profiles=json.loads((ROOT/"materials.json").read_text(encoding="utf-8"))["materials"]
    output.mkdir(parents=True,exist_ok=True)
    for name in NAMES:
        for suffix,data in render(name,profiles[name],size).items():
            path=output/("T_MF_"+name.title()+"_"+suffix+".png")
            if path.exists() and path.read_bytes()!=data:
                raise FileExistsError("Refusing to overwrite different texture: "+str(path))
            if not path.exists():
                path.write_bytes(data)
    return 15

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--size",type=int,default=1024)
    parser.add_argument("--output",type=Path,default=ROOT/"Textures")
    args=parser.parse_args()
    print("Generated or verified",generate(args.size,args.output),"original texture maps.")
