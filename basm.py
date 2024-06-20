Import("env")

import sys
from itertools import chain
import re
from pathlib import Path

def extract_regs(data, path = None):
    RE_DEFS = r'#(define|undef)[ \t]+([a-z_][a-z0-9_]*)(?:[ \t]+([a-z])(\[(\d+)-(\d+)\]|(\d+)))?'

    line_starts = [m.start() for m in re.finditer('\n', data)]

    # map register prefix to owned register values
    taken: dict[str, set[int]] = {}
    # map symbol to register prefix and value
    owners: dict[str, tuple[str, int]] = {}

    def sub_register(match: re.Match) -> str:
        directive, sym, reg, constraint, lo, hi, val = match.groups()

        match directive.lower():
            case 'define': 
                # take register
                options = None
                if lo is not None and hi is not None:
                    lo, hi = int(lo), int(hi)
                    # take any register from lo to hi
                    options = set( range(lo, hi+1) )
                if val is not None:
                    val = int(val)
                    # take only val
                    options = { val }

                taken_set = taken.setdefault(reg, set())

                # take the lowest available register
                took: int = min(options - taken_set, default=None)
                
                if took is None: 
                    start = match.start()
                    line = next(line for line, line_start in enumerate(line_starts) if line_start > start) + 1
                    print(f"[basm] no available registers for {sym} in {str(path)}:{line}")
                    raise Exception()
                owners[sym] = (reg, took)
                taken_set.add(took)

                match_start, match_end = match.span(0)
                constraint_start, constraint_end = match.span(4)

                prefix, suffix = match.string[match_start:constraint_start], match.string[constraint_end:match_end]

                return f"{prefix}{took}{suffix} // basm: take {reg}{took}" 
            case 'undef':
                reg, took = owners.pop(sym)
                taken[reg].remove(took)
                return f"{match.group()} // basm: free {reg}{took}"
    
    retval = re.sub(RE_DEFS, sub_register, data, flags=re.IGNORECASE)

    for sym, (reg, took) in owners.items():
        print(f"\twarning: {sym} is not freed")
    
    return retval


def process(path: Path):
    print(path.with_suffix(".S"))
    path.with_suffix(".S").write_text(
        extract_regs(path.read_text(), path))

# def basm_rebuild(source, target, env):
print("basm: processing...", Path.cwd())

paths = (
    *env["LIBSOURCE_DIRS"],
    env["PROJECT_SRC_DIR"]
)

cwd = Path.cwd()
for path in chain(*(Path(p).glob("**/*.basm") for p in paths)):
    process(path)
    

#for arg in sys.argv[1:]:
#    process(Path(arg))
    