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

def disas(name, length=256):
    if name not in symbols:
        return []
    addr = symbols[name]
    code = get_code(addr, length)
    return list(md.disasm(code, addr))

print("================================================================================")
print("1. TASK_STRUCT & CRED OFFSETS VERIFICATION via commit_creds & prepare_kernel_cred")
print("================================================================================")
for insn in disas("commit_creds", 200):
    print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- prepare_kernel_cred ---")
for insn in disas("prepare_kernel_cred", 200):
    print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- selinux_cred_free & selinux_cred_prepare ---")
if "selinux_cred_free" in symbols:
    for insn in disas("selinux_cred_free", 80):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")
if "selinux_cred_prepare" in symbols:
    print("\n--- selinux_cred_prepare ---")
    for insn in disas("selinux_cred_prepare", 120):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n================================================================================")
print("2. RT_MUTEX_WAITER & TASK_STRUCT PI FIELDS via rt_mutex_init_waiter & rt_mutex_adjust_prio_chain")
print("================================================================================")
print("--- rt_mutex_init_waiter ---")
for insn in disas("rt_mutex_init_waiter", 160):
    print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- rt_mutex_adjust_prio_chain (first 50 instructions) ---")
for insn in disas("rt_mutex_adjust_prio_chain", 300)[:50]:
    print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- remove_waiter (first 40 instructions) ---")
for insn in disas("remove_waiter", 200)[:40]:
    print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n================================================================================")
print("3. FILE_OPERATIONS OFFSETS via vfs_llseek, vfs_read, vfs_write, do_vfs_ioctl, vfs_poll, vfs_mmap")
print("================================================================================")
print("--- vfs_llseek ---")
for insn in disas("vfs_llseek", 80):
    print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- vfs_read (instructions around f_op access) ---")
for insn in disas("vfs_read", 250):
    if any(k in insn.op_str for k in ["#0x10", "#0x20", "#0x28", "blr"]):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- vfs_write (instructions around f_op access) ---")
for insn in disas("vfs_write", 250):
    if any(k in insn.op_str for k in ["#0x18", "#0x28", "blr"]):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

print("\n--- do_vfs_ioctl ---")
for insn in disas("do_vfs_ioctl", 400):
    if any(k in insn.op_str for k in ["#0x48", "#0x50", "#0x28", "blr"]):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

if "vfs_poll" in symbols:
    print("\n--- vfs_poll ---")
    for insn in disas("vfs_poll", 80):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")

if "vfs_mmap" in symbols:
    print("\n--- vfs_mmap ---")
    for insn in disas("vfs_mmap", 80):
        print(f"  {hex(insn.address)}: {insn.mnemonic:10s} {insn.op_str}")
