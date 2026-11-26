"""Compare the compiled port's tables with a separate Flash decomp checkout.

python tests/verify_flash_rules.py /path/to/sim-regression /path/to/decomp
"""
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import sys
import zlib

binary, reference = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
text = (reference/'Games/age of war/The game/scripts/frame_13/DoAction.as').read_text()
def table(name):
    value = re.search(r'^'+name+r' = (\[.*\]);',text,re.M).group(1)
    return json.loads(value.replace("\\'", "'"))
en,es,tu,ts = (table(name) for name in ('EN','ES','TU','TS'))
swf = (reference/'Games/age of war/Age_Of_War.swf').read_bytes()
swf = swf[:8]+zlib.decompress(swf[8:]) if swf[:3]==b'CWS' else swf
offset = 8+(5+4*(swf[8]>>3)+7)//8
fps = struct.unpack_from('<H',swf,offset)[0]/256
rows = subprocess.check_output([str(binary),'--tables'],text=True).splitlines()
assert float(rows[0].split()[1])==fps==40
for row in rows[1:]:
    kind,index,*values=row.split()
    i=int(index)+1
    if kind=='U':
        expected=[en[i][1],*es[i][:3],es[i][3]/50,es[i][4]/50,en[i][2]/fps]
    else:
        expected=[tu[i][1],ts[i][0]/fps,ts[i][2],ts[i][3]/50,ts[i][1]]
    assert len(values)==len(expected)
    assert all(math.isclose(float(a),b,abs_tol=0.0006) for a,b in zip(values,expected)), (row,expected)
print('PASS: SWF frame rate, all 16 EN/ES unit rows and all 15 TU/TS turret rows')
