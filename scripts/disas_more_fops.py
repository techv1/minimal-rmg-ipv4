import elftools.elf.elffile as elffile
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

def load_vmlinux():
    with open('/root/s/vmlinux.elf', 'rb') as f:
        elf = elffile.ELFFile(f)
        symtab = elf.get_section_by_name('.symtab')
        symbols = {sym.name: sym['st_value'] for sym in symtab.iter_symbols() if sym.name}
        sec = elf.get_section_by_name('.kernel')
        return symbols, sec['sh_addr'], sec.data()

symbols, sec_addr, sec_data = load_vmlinux()

def get_code(addr, length):
    offset = addr - sec_addr
    return sec_data[offset:offset+length]

md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)

for sym in ['do_vfs_ioctl', 'vfs_ioctl', 'mmap_region', 'do_dentry_open', 'vfs_open']:
    if sym in symbols:
        print(f"\n=== {sym} @ {hex(symbols[sym])} ===")
        addr = symbols[sym]
        for insn in md.disasm(get_code(addr, 200), addr):
            if any(k in insn.op_str for k in ['[x', 'blr', 'add', 'ldr', 'str']):
                print(f"  {hex(insn.address)}:  {insn.mnemonic:10s} {insn.op_str}")
