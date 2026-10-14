"""Extract the actual marked native begin provider for its focused fixture."""
import sys
from pathlib import Path

source = Path(sys.argv[1]).read_text()
start = source.index('// BEGIN BASELINE_RENDER_PROVIDER')
start = source.index('\n', start) + 1
end = source.index('// END BASELINE_RENDER_PROVIDER', start)
assert source.count('// BEGIN BASELINE_RENDER_PROVIDER') == 1
assert source.count('// END BASELINE_RENDER_PROVIDER') == 1
Path(sys.argv[2]).write_text(source[start:end], newline='\n')
