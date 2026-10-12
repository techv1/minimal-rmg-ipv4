import sys
import elftools.elf.elffile as elffile
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM

def load_vmlinux():
    with open('/root/s/vmlinux.elf', 'rb') as f:
        elf = elffile.ELFFile(f)
        
        # Build symbol map
        symtab = elf.get_section_by_name('.symtab')
        symbols = {}
        for sym in symtab.iter_symbols():
            if sym.name:
                symbols[sym.name] = sym['st_value']
                
        # Find executable section (.kernel)
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

def disas_func(name, length=128):
    if name not in symbols:
        print(f"[-] Symbol {name} not found!")
        return []
    addr = symbols[name]
    code = get_code(addr, length)
    insns = list(md.disasm(code, addr))
    return insns

def print_disas(name, length=128, max_insns=30):
    addr = symbols.get(name, 0)
    print(f"\n=======================================================")
    print(f" Disassembly of {name} @ {hex(addr)}")
    print(f"=======================================================")
    insns = disas_func(name, length)
    for i, insn in enumerate(insns[:max_insns]):
        print(f"  {hex(insn.address)}:  {insn.mnemonic:10s} {insn.op_str}")

# Run disassemblies for target verification
print_disas("commit_creds", 160)
print_disas("prepare_kernel_cred", 160)
print_disas("rt_mutex_init_waiter", 120)
print_disas("rt_mutex_adjust_prio_chain", 300, 60)
print_disas("remove_waiter", 200, 40)
print_disas("vfs_read", 160)
print_disas("vfs_write", 160)
print_disas("vfs_llseek", 160)
print_disas("do_vfs_ioctl", 160)
