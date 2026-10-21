"""68000 decoding (capstone) and rendering to GNU as syntax that reassembles to the same bytes.

Rendering is checked, not trusted: tools/build.py assembles every rendered instruction and compares it
with the original bytes; any instruction that does not reproduce exactly is emitted as raw `.short`
words instead (and counted), so the build is always byte-identical while the readable share grows as
the renderer improves.
"""
from __future__ import annotations

import re
from dataclasses import dataclass, field

import capstone
from capstone import m68k as M

_MD = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_M68K_000)
_MD.detail = True

ROM_END = 0x400000
# Control-flow classes
RETURNS = {"rts", "rte", "rtr"}
UNCONDITIONAL = {"bra", "jmp"}
CALLS = {"bsr", "jsr"}


@dataclass
class Insn:
    address: int
    size: int
    raw: bytes
    mnemonic: str          # capstone mnemonic, e.g. "bne.b"
    op_str: str            # capstone operand text
    base: str              # mnemonic without size suffix, e.g. "bne"
    targets: list[int] = field(default_factory=list)   # absolute code/data addresses referenced
    flow: str = "normal"   # normal | branch | jump | call | return | computed_jump | computed_call | stop
    branch_target: int | None = None


def decode_one(rom: bytes, address: int) -> Insn | None:
    """Decode one instruction at `address` (or None if the bytes are not a valid 68000 instruction)."""
    window = rom[address:address + 10]
    for i in _MD.disasm(window, address, count=1):
        return _classify(i)
    return None


def _target_of_mem(i, op) -> int | None:
    am = op.address_mode
    if am in (M.M68K_AM_ABSOLUTE_DATA_LONG, M.M68K_AM_ABSOLUTE_DATA_SHORT):
        value = op.imm & 0xFFFFFFFF
        if am == M.M68K_AM_ABSOLUTE_DATA_SHORT and value & 0x8000:
            value |= 0xFFFF0000
        return value & 0xFFFFFF
    if am == M.M68K_AM_PCI_DISP:
        return (i.address + 2 + op.mem.disp) & 0xFFFFFF
    if am in (M.M68K_AM_PCI_INDEX_8_BIT_DISP, M.M68K_AM_PCI_INDEX_BASE_DISP):
        # capstone reports a brief extension word with a zero displacement as the 68020
        # base-displacement mode; on the 68000 both are d8(PC,Xn)
        return (i.address + 2 + op.mem.disp) & 0xFFFFFF
    return None


_DIRECT = (M.M68K_AM_ABSOLUTE_DATA_LONG, M.M68K_AM_ABSOLUTE_DATA_SHORT, M.M68K_AM_PCI_DISP)


def _classify(i) -> Insn:
    mnemonic = i.mnemonic
    base = mnemonic.split(".")[0]
    insn = Insn(i.address, i.size, bytes(i.bytes), mnemonic, i.op_str, base)
    direct = None
    for op in i.operands:
        if op.type == M.M68K_OP_BR_DISP:
            target = (i.address + 2 + op.br_disp.disp) & 0xFFFFFF
            insn.branch_target = target
            insn.targets.append(target)
        elif op.type == M.M68K_OP_MEM:
            target = _target_of_mem(i, op)
            if target is not None:
                insn.targets.append(target)
                if op.address_mode in _DIRECT:
                    direct = target
    if base in RETURNS:
        insn.flow = "return"
    elif base == "bra":
        insn.flow = "jump"
    elif base == "bsr":
        insn.flow = "call"
    elif base in ("jmp", "jsr"):
        # jmp/jsr through an index register (switch tables) or an address register is computed;
        # an indexed operand's base address is a data reference, not a code target.
        if direct is not None:
            insn.branch_target = direct
            insn.flow = "jump" if base == "jmp" else "call"
        else:
            insn.flow = "computed_jump" if base == "jmp" else "computed_call"
    elif base.startswith("db") or (base.startswith("b") and insn.branch_target is not None):
        insn.flow = "branch"
    elif base in ("stop", "illegal"):
        insn.flow = "stop"
    return insn


# ---------------------------------------------------------------------------------------------
# Rendering (capstone text -> GNU as, Motorola operand order with % registers)

_REG = re.compile(r"(?<![\w$%])(d[0-7]|a[0-7]|sp|pc|sr|ccr|usp)(?![\w])")
_HEX = re.compile(r"\$([0-9a-fA-F]+)")
_ABS = re.compile(r"(?<![#\w(])(-?0x[0-9a-f]+)\.([wl])(?![\w])")
_DISP = re.compile(r"(-?0x[0-9a-f]+)\((%[ad][0-7]|%sp|%pc)(,%[ad][0-7]\.[wl](?:\*[1248])?)?\)")
_BRANCH_SIZE = {"b": "s", "s": "s", "w": "w"}


def _signed_immediate(text: str, bits: int) -> str:
    """capstone prints moveq/link immediates unsigned (#$ff); gas wants the signed value."""
    def fix(match: re.Match) -> str:
        value = int(match.group(1), 16) & ((1 << bits) - 1)
        if value >= 1 << (bits - 1):
            value -= 1 << bits
        return f"#{value}"
    return re.sub(r"#0x([0-9a-f]+)", fix, text)


def _gas_operands(text: str) -> str:
    text = _HEX.sub(lambda m: "0x" + m.group(1).lower(), text)
    text = _REG.sub(lambda m: "%" + ("sp" if m.group(1) in ("a7", "sp") else m.group(1)), text)
    text = text.replace(", ", ",")
    text = _ABS.sub(lambda m: f"({m.group(1)}).{m.group(2)}", text)
    text = _DISP.sub(lambda m: f"({m.group(1)},{m.group(2)}{m.group(3) or ''})", text)
    return text


def render(insn: Insn, symbol) -> str | None:
    """GNU as text for `insn`, or None when this renderer has no safe spelling.

    `symbol(address, required)` names an absolute address. Branch and PC-relative targets are
    always symbolic (required=True) so the assembler recomputes the displacement; absolute
    addressing uses a symbol only for known code labels (required=False may return None).
    """
    m = insn.mnemonic
    base = insn.base
    ops = insn.op_str
    if insn.flow in ("branch", "jump", "call") and base not in ("jmp", "jsr"):
        # bcc/bra/bsr (byte/word) and dbcc: replace the absolute target with a label.
        target = symbol(insn.branch_target, True)
        if base.startswith("db"):
            reg = _gas_operands(ops.split(",")[0])
            return f"{base} {reg},{target}"
        size = m.split(".")[1] if "." in m else ""
        if size not in _BRANCH_SIZE:
            return None
        return f"{base}.{_BRANCH_SIZE[size]} {target}"
    text = _gas_operands(ops)

    def pcrel(match: re.Match) -> str:
        # capstone prints PC-relative operands with their absolute target
        return f"({symbol(int(match.group(1), 16), True)},%pc{match.group(2) or ''})"

    def absolute(match: re.Match) -> str:
        value = int(match.group(1), 16)
        name = symbol(value & 0xFFFFFF, False) if match.group(2) == "l" and 0 <= value < ROM_END else None
        return f"({name or match.group(0)[1:-3]}).{match.group(2)}"

    text = re.sub(r"\(0x([0-9a-f]+),%pc(,%[ad][0-7]\.[wl])?\)", pcrel, text)
    text = re.sub(r"\((0x[0-9a-f]+)\)\.([wl])", absolute, text)
    name = m
    if base in ("lea", "pea", "exg", "abcd", "sbcd", "nbcd"):
        name = base  # gas has no sized spelling of abcd; lea/pea/exg/BCD sizes are implied
    elif base in ("btst", "bchg", "bclr", "bset"):
        name = base  # size is implied by the destination (byte for memory, long for Dn)
    elif base == "move" and ("%usp" in text):
        name = "move.l"
    elif base == "link":
        name, text = "link", _signed_immediate(text, 16)
    elif base == "moveq":
        text = _signed_immediate(text, 8)
    if base in ("move", "ori", "andi", "eori") and text.endswith(("%sr", "%ccr")):
        name = base if base != "move" else "move.w"
    return f"{name} {text}".strip()


def words(raw: bytes) -> str:
    """Raw fallback: the exact instruction bytes as big-endian 16-bit words."""
    return ".short " + ",".join(f"0x{int.from_bytes(raw[k:k + 2], 'big'):04x}" for k in range(0, len(raw), 2))
