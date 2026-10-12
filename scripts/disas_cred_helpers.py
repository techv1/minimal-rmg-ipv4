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

def disas_named(name, length=128):
    if name not in symbols:
        print(f"[-] {name} not found")
        return
    print(f"\n=== {name} @ {hex(symbols[name])} ===")
    addr = symbols[name]
    for insn in md.disasm(get_code(addr, length), addr):
        print(f"  {hex(insn.address)}:  {insn.mnemonic:10s} {insn.op_str}")

disas_named("prepare_creds", 160)
disas_named("override_creds", 80)
disas_named("revert_creds", 80)
disas_named("set_security_override_from_ctx", 80)
disas_named("selinux_cred_prepare", 100)
disas_named("selinux_cred_free", 80)
