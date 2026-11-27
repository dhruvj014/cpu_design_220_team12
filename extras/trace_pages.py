"""trace_pages.py - Turns a ./cpu trace into captioned PDF pages for the report. Owner: Tanisha

Not part of the simulator (it is a report tool and is removed with extras/ before zipping).
Usage: python3 extras/trace_pages.py <trace.txt> <program name> <out.pdf>
Needs: pip install reportlab
"""
import sys

from reportlab.lib.pagesizes import letter
from reportlab.pdfgen import canvas

FONT = "Courier"
SIZE = 9          # body font size in points
LEAD = 10.5       # line height in points
PER_PAGE = 2      # snapshots per page
RULE = "=" * 66


def read_snapshots(path):
    """Returns (snapshots, result_line). Each snapshot is its list of lines, from one ===== rule to the next."""
    with open(path, encoding="utf-8") as f:
        lines = f.read().split("\n")
    snaps, current, result = [], None, ""
    for line in lines:
        if line == RULE:
            if current is None:
                current = [line]                 # opening rule of a snapshot
            else:
                current.append(line)             # closing rule
                snaps.append(current)
                current = None
        elif current is not None:
            current.append(line)
        elif line.startswith("RESULT:") or line.startswith("FAULT"):
            result = line
    return snaps, result


def caption(index, total, program, snap):
    """Caption for one snapshot, e.g. 'Snapshot 9 of 20 (pow): step 8, ST R3, [R0+0x34]'."""
    head = snap[1]
    if "initial memory" in head:
        return "Snapshot %d of %d (%s): initial memory, before the first instruction" % (index, total, program)
    step = int(head.split()[1])
    instruction = head.split("   ")[-1].strip()   # disassembly is the last field of the STEP line
    return "Snapshot %d of %d (%s): step %d, %s" % (index, total, program, step, instruction)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 1
    path, program, out = sys.argv[1], sys.argv[2], sys.argv[3]
    snaps, result = read_snapshots(path)
    if not snaps:
        print("no snapshots found in", path)
        return 1

    pdf = canvas.Canvas(out, pagesize=letter)
    width, height = letter
    left, top = 72, height - 60
    block = (max(len(snap) for snap in snaps) + 4) * LEAD   # room for the longest snapshot and its caption

    for i, snap in enumerate(snaps):
        slot = i % PER_PAGE
        y = top - slot * block
        pdf.setFont(FONT + "-Bold", SIZE + 1)
        pdf.drawString(left, y, caption(i + 1, len(snaps), program, snap))
        pdf.setFont(FONT, SIZE)
        y -= 1.6 * LEAD
        for line in snap:
            pdf.drawString(left, y, line)
            y -= LEAD
        last = i == len(snaps) - 1
        if last and result:
            pdf.setFont(FONT + "-Bold", SIZE)
            pdf.drawString(left, y - 0.6 * LEAD, result)
        if slot == PER_PAGE - 1 or last:
            pdf.showPage()
    pdf.save()
    print("%s: %d snapshots on %d pages -> %s" % (program, len(snaps), (len(snaps) + PER_PAGE - 1) // PER_PAGE, out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
