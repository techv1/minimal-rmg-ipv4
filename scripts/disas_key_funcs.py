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
md.detail = True

def disas_full(name, length=256):
    if name not in symbols:
        print(f"Symbol {name} not found")
        return
    addr = symbols[name]
    code = get_code(addr, length)
    print(f"\n=======================================================")
    print(f" Disassembly of {name} @ {hex(addr)}")
    print(f"=======================================================")
    insns = list(md.disasm(code, addr))
    for insn in insns:
        print(f"  {hex(insn.address)}:  {insn.mnemonic:10s} {insn.op_str}")

disas_full("commit_creds", 180)
disas_full("prepare_kernel_cred", 140)
disas_full("rt_mutex_init_waiter", 80)
disas_full("vfs_read", 200)
disas_full("vfs_write", 200)
