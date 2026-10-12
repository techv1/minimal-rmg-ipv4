import elftools.elf.elffile as elffile
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

def load_vmlinux():
    with open('/root/s/vmlinux.elf', 'rb') as f:
        elf = elffile.ELFFile(f)
        symtab = elf.get_section_by_name('.symtab')
        symbols = {}
        for sym in symtab.iter_symbols():
            if sym.name:
                symbols[sym.name] = sym['st_value']
                
        sec = elf.get_section_by_name('.kernel')
        sec_addr = sec['sh_addr']
        sec_data = sec.data()
        return symbols, sec_addr, sec_data

symbols, sec_addr, sec_data = load_vmlinux()

def get_code(addr, length):
    offset = addr - sec_addr
    return sec_data[offset:offset+length]

md = Cs(CS_ARCH_ARM64, CS_MODE_ARM)

def analyze_commit_creds():
    print("=== commit_creds Analysis ===")
    addr = symbols["commit_creds"]
    code = get_code(addr, 0x120)
    for insn in md.disasm(code, addr):
        print(f"  {hex(insn.address)}:  {insn.mnemonic:10s} {insn.op_str}")

def analyze_vfs_read_write():
    print("\n=== vfs_read fops Analysis ===")
    addr = symbols["vfs_read"]
    code = get_code(addr, 0x150)
    for insn in md.disasm(code, addr):
        print(f"  {hex(insn.address)}:  {insn.mnemonic:10s} {insn.op_str}")

analyze_commit_creds()
analyze_vfs_read_write()
