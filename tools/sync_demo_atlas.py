#!/usr/bin/env python3
"""Sync the shared raster atlas into the standalone browser demo, then package it.
The C++ atlas is authoritative; editing glyphs never requires a font download.
"""
from pathlib import Path
import re,base64
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'include/medieval_atlas.hpp').read_text()
widths=list(map(int,re.search(r'widths\[95\]=\{([^}]+)',s)[1].split(',')))
rows=[[int(v,16) for v in t.split(',')] for t in re.findall(r'\{(0x[^{}]+)\}',s)]
p=ROOT/'tools/demo-v10.html';html=p.read_text()
html=re.sub(r'const atlasWidths=\[[^]]+\]', 'const atlasWidths='+str(widths).replace(' ',''),html)
for i in range(95):
 paths=[]
 for y,r in enumerate(rows[i]):
  x=0
  while x<widths[i]:
   if not r>>x&1:x+=1;continue
   start=x
   while x<widths[i] and r>>x&1:x+=1
   paths.append(f'M{start},{y}h{x-start}v1h-{x-start}z')
 html=re.sub(r'<symbol id="md'+str(i+32)+r'".*?</symbol>', '<symbol id="md'+str(i+32)+'" overflow="visible"><path d="'+' '.join(paths)+'"/></symbol>',html)
mesh=(ROOT/'tools/dice-meshes.json').read_text();tray=(ROOT/'tools/dice-physics.js').read_text()+'\n'+(ROOT/'tools/dice-tray.js').read_text();html=re.sub(r'// BEGIN_DICE_TRAY[\s\S]*?// END_DICE_TRAY',lambda _: '// BEGIN_DICE_TRAY\nconst diceMeshes='+mesh+';\n'+tray+'\n// END_DICE_TRAY',html)
html=re.sub(r'// BEGIN_HIT_AUDIO[\s\S]*?// END_HIT_AUDIO',lambda _: '// BEGIN_HIT_AUDIO\n'+(ROOT/'tools/hit-audio.js').read_text()+'// END_HIT_AUDIO',html)
art='data:image/png;base64,'+base64.b64encode((ROOT/'assets/home.png').read_bytes()).decode()
html=re.sub(r"const homeArt='[^']*';",lambda _: "const homeArt='"+art+"';",html)
p.write_text(html)
(ROOT/'dist/Lifeloch-v0.10-Demo.html').write_text(html)
print('Synced C++ atlas and packaged v0.10 demo')
