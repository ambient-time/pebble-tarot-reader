"""Match native card pixels to the pinned resources, including reversed cards."""
import hashlib
import json
import re
import struct
from pathlib import Path
from PIL import Image

root=Path(__file__).resolve().parent.parent
sha=hashlib.sha256((root/'build/tarot-reader.pbw').read_bytes()).hexdigest()
targets=json.loads((root/'package.json').read_text())['pebble']['targetPlatforms']
reverse_count=0
for platform in targets:
    report=json.loads((root/f'build/evidence/{platform}-report.json').read_text())
    assert report['passed'] and report['pbwSHA256']==sha
    for selected,mask,frame in [(0,1,'card'),(1,3,'second-card')]:
        log=next(line for line in report['logs'] if f'screen=2 menu=3 count=10 selected={selected} revealed={mask} ' in line)
        card,rev=map(int,re.search(r'card=(\d+) reverse=(\d+)',log).groups())
        suffix='~'+platform if platform in ['emery','gabbro','chalk'] else ''
        raw=(root/f'resources/data/art-{card}{suffix}.bin').read_bytes()
        w,h=struct.unpack('<HH',raw[:4])
        image=Image.open(root/f'build/evidence/{platform}-{frame}.png').convert('RGB')
        left=(image.width-w)//2;top=(image.height-h)//2+(0 if platform in ['chalk','gabbro'] else 2)
        for y in range(h):
            for x in range(w):
                sx,sy=(w-1-x,h-1-y) if rev else (x,y)
                white=bool(raw[4+sy*(w//8)+sx//8] & (0x80>>(sx%8)))
                expected=(255,255,255) if white else (0,0,0)
                assert image.getpixel((left+x,top+y))==expected,(platform,frame,x,y)
        reverse_count+=rev
    # The enlarged view must use the separately rasterized source, including
    # the bottom of reversed images, rather than stretching the small preview.
    log=next(line for line in report['logs'] if 'screen=5 menu=3 count=10 selected=0 revealed=1 ' in line)
    card,rev=map(int,re.search(r'card=(\d+) reverse=(\d+)',log).groups())
    suffix='~'+platform if platform in ['emery','gabbro'] else ''
    raw=(root/f'resources/data/detail-{card}{suffix}.bin').read_bytes()
    w,h=struct.unpack('<HH',raw[:4])
    for frame,bottom in [('detail',False),('detail-bottom',True)]:
        image=Image.open(root/f'build/evidence/{platform}-{frame}.png').convert('RGB')
        left=(image.width-w)//2
        reserved=72 if platform in ['chalk','gabbro'] else 60
        offset=max(0,h-(image.height-reserved)) if bottom else 0
        for y in range(32,image.height-(40 if platform in ['chalk','gabbro'] else 28)):
            for x in range(w):
                px=left+x
                if platform in ['chalk','gabbro'] and (px-image.width/2)**2+(y-image.height/2)**2 >= (image.width/2-1)**2: continue
                sx,sy=(w-1-x,h-1-(y-32+offset)) if rev else (x,y-32+offset)
                white=bool(raw[4+sy*(w//8)+sx//8] & (0x80>>(sx%8)))
                assert image.getpixel((px,y))==((255,255,255) if white else (0,0,0)),(platform,frame,x,y)
print(f'PASS: 12 enlarged crops and 12 native card images match resource pixels exactly; {reverse_count} reversed')
