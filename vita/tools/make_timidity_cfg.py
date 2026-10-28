"""Writes vita/timidity.cfg: the FreePats crude.cfg entries Captain Claw's music uses.

Usage: python3 -I make_timidity_cfg.py <freepats/crude.cfg> <out.cfg>

crude.cfg is in the FreePats 20060219 archive that vita/Freepats.cmake downloads
(build-vita/freepats-src/freepats/crude.cfg after a Vita build). Drum 82 is added by
hand afterwards, see the header of vita/timidity.cfg.
"""
import re
import sys

# From scanning all 50 XMI tracks in CLAW.REZ (programs on melodic channels, notes on channel 10)
USED_PROGRAMS = {0, 6, 8, 9, 12, 26, 32, 43, 44, 45, 46, 47, 48, 49, 52, 55, 56, 57, 58, 60, 61, 62,
                 68, 69, 70, 71, 73, 76, 113, 116, 117}
USED_DRUMS = {36, 38, 40, 49, 51, 52, 53, 54, 55, 57, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 70,
              73, 75, 82}

HEADER = """\
# Timidity configuration for OpenClaw on the PS Vita (SDL_mixer's built-in Timidity).
#
# Generated from FreePats 20060219 crude.cfg, which fills the gaps in the incomplete FreePats
# set with substitute patches. Only the programs and drum notes used by Captain Claw's music
# are kept (found by scanning all 50 XMI tracks in CLAW.REZ), so only those patches need to be
# shipped. Patch paths are relative to this file's folder.
#
# FreePats: Copyright (C) 2004 Free Software Foundation, GPL-2.0-or-later with an exception
# for compositions; see COPYING and README next to this file.

"""


def main():
    crude_path, out_path = sys.argv[1], sys.argv[2]
    section = None
    lines = []
    for raw in open(crude_path, encoding="latin-1"):
        line = raw.rstrip("\r\n")
        stripped = line.strip()
        if stripped.startswith("bank "):
            section = "bank"
            lines.append("")
            lines.append(stripped)
            continue
        if stripped.startswith("drumset "):
            section = "drumset"
            lines.append("")
            lines.append(stripped)
            continue
        match = re.match(r"^\s*(\d+)\s+(\S+\.pat)(.*)$", line)
        if not match or section is None:
            continue
        number = int(match.group(1))
        wanted = USED_PROGRAMS if section == "bank" else USED_DRUMS
        if number in wanted:
            lines.append("%3d  %s%s" % (number, match.group(2), match.group(3).rstrip()))

    with open(out_path, "w") as out:
        out.write(HEADER)
        out.write("\n".join(lines).lstrip("\n"))
        out.write("\n")

    patches = sorted({l.split()[1] for l in lines if l and l[0] in " 0123456789" and ".pat" in l})
    print("%d entries, %d distinct patches" % (sum(1 for l in lines if ".pat" in l), len(patches)))


if __name__ == "__main__":
    main()
