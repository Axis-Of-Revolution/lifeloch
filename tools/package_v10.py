#!/usr/bin/env python3
"""Package current Lifeloch source and demo, without old executable/banner artifacts."""
from pathlib import Path
import hashlib,shutil,zipfile
root=Path(__file__).resolve().parents[1]
release=root.parent.parent/'release';release.mkdir(exist_ok=True)
files=[root/name for name in ['README.md','COPY_AND_BRANDING.md','LICENSE','Makefile','package.json','BUILD_STATUS.txt','RELEASE_INSTRUCTIONS.txt','dist/Lifeloch-v0.10-Demo.html']]
for folder in ['source','include','tests','assets']:
 files.extend(p for p in (root/folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.suffix not in ['.bnr','.o','.s'] and p.name not in ['test_v06.cpp','test_v07.cpp','test_v08.cpp','test_v09.cpp'])
for name in ['build_demo.sh','sync_demo_atlas.py','demo-v10.html','dice-tray.js','hit-audio.js','dice-physics.js','dice-meshes.json','generate_dice_meshes.py','test_host.sh','test_v3.sh','test_v4.sh','test_v10.sh','test_v10_demo.cjs','test_native_keyboard.sh','render_native_v10.cpp','make_assets.py','package_v10.py']:
 files.append(root/'tools'/name)
files=sorted(files)
checksums=''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(root))+'\n' for p in files)
output=release/'Lifeloch-v0.10.2-Source-and-Demo.zip'
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
 for p in files:z.write(p,'Lifeloch/'+str(p.relative_to(root)))
 z.writestr('Lifeloch/SHA256SUMS.txt',checksums)
with zipfile.ZipFile(output) as z:
 assert z.testzip() is None
 assert not any(n.endswith(('.cia','.3dsx','.elf','.bnr')) for n in z.namelist())
 assert z.read('Lifeloch/dist/Lifeloch-v0.10-Demo.html')==(root/'tools/demo-v10.html').read_bytes()
shutil.copy2(root/'dist/Lifeloch-v0.10-Demo.html',release/'Lifeloch-Demo-v0.10.2.html')
print(output.name,output.stat().st_size,'bytes;',len(files),'files; ZIP integrity and standalone demo identity verified.')
