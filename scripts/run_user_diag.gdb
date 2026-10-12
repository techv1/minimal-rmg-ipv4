file /root/s/vmlinux.elf
target remote localhost:1234

# 1. restore_fpsimd_context entry (Active from start, enables rb_erase and walk once hit)
hbreak *0xffffffc0102cf86c

# 2. walk entry (disabled initially)
hbreak *0xffffffc010375424
disable 2

# 3. rb_erase entry (disabled initially to allow fast boot without CFS/timer spam)
hbreak *0xffffffc011638ae8
disable 3

# 4. panic breakpoint
break panic
commands 4
silent
printf "\n=== PANIC REACHED ===\n"
quit
end

# Log restore_fpsimd_context: struct base and where kernel writes
commands 1
silent
printf "=== RESTORE-FPSIMD ===\n"
printf "struct base (x0) = %p\n", $x0
printf "mem at x0+0x148  = %p %p\n", *(void**)($x0+0x148), *(void**)($x0+0x150)
printf "mem at x0+0x158  = %p %p\n", *(void**)($x0+0x158), *(void**)($x0+0x160)
printf "mem at x0+0x138  = %p %p\n", *(void**)($x0+0x138), *(void**)($x0+0x140)
enable 2
enable 3
continue
end

# Log walk entry: what waiter does walk see
commands 2
silent
printf "=== WALK ===\n"
printf "waiter x25 = %p\n", $x25
printf "x25+0x00 = %p\n", *(void**)$x25
printf "x25+0x08 = %p\n", *(void**)($x25+0x08)
printf "x25+0x10 = %p\n", *(void**)($x25+0x10)
printf "x25+0x30 = %p\n", *(void**)($x25+0x30)
printf "x25+0x38 = %p\n", *(void**)($x25+0x38)
continue
end

# Log rb_erase
commands 3
silent
printf "=== RB-ERASE ===\n"
printf "x0=%p x8=%p x9=%p\n", $x0, $x8, $x9
printf "node[0]=%p node[8]=%p node[16]=%p\n", *(void**)$x0, *(void**)($x0+8), *(void**)($x0+16)
continue
end

continue
