import hashlib
import json
import struct
import zipfile
from pathlib import Path

root=Path(__file__).resolve().parent.parent
cards=json.loads((root/'reference/cards.json').read_text())['cards']
assert len(cards)==len(set(c['name'] for c in cards))==78
sources=json.loads((root/'reference/sources.json').read_text())
for name,sha in sources['localArtSHA256'].items():
    assert hashlib.sha256((root/'reference/art'/name).read_bytes()).hexdigest()==sha
for i,card in enumerate(cards):
    text=(root/f'resources/data/text-{i}.bin').read_bytes().split(b'\0')
    assert text==[card['meaning_up'].encode(),card['meaning_rev'].encode(),b'']
    for kind,suffix,size in [('art','',(72,116)),('art','~chalk',(64,104)),('art','~emery',(104,168)),('art','~gabbro',(104,168)),('detail','',(128,208)),('detail','~emery',(184,296)),('detail','~gabbro',(184,296))]:
        b=(root/f'resources/data/{kind}-{i}{suffix}.bin').read_bytes()
        assert struct.unpack('<HH',b[:4])==size
        assert len(b)==4+size[0]*size[1]//8
        assert len(set(b[4:]))>10
package=json.loads((root/'package.json').read_text())
assert package['pebble']['watchapp']['watchface'] is False
with zipfile.ZipFile(root/'build/tarot-reader.pbw') as z:
    for platform in package['pebble']['targetPlatforms']:
        assert f'{platform}/pebble-app.bin' in z.namelist()
        assert f'{platform}/app_resources.pbpack' in z.namelist()
print('PASS: 78 source hashes, 156 exact text fields, 546 bitmaps, six packaged native targets')
