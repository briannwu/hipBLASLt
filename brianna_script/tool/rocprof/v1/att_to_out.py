import numpy as np
import sys
BYTE_MAP = [str(k) for k in range(10)] + ['a', 'b', 'c', 'd', 'e', 'f']
def map8(c):
	return BYTE_MAP[(c//16)%16]+BYTE_MAP[c%16]
def map16(c):
	return map8(c>>8)+map8(c)
in_filename = sys.argv[1]
out_filename = in_filename.split('.att')[0]+'.out'
in_bytes = np.fromfile(in_filename, dtype=np.uint16)
out_bytes = [map16(c)+'\n' for c in in_bytes]
with open(out_filename, 'w') as f:
	[f.write(b) for b in out_bytes]
