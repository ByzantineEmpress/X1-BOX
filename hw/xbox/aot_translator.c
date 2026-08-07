#include "qemu/osdep.h"
#include "exec/translation-block.h"
#include "accel/tcg/tb-cpu-state.h"
#include "exec/mmap-lock.h"
#include "accel/tcg/internal-common.h"
#include "aot_cache.h"

extern void xemu_aot_translate_block(uint32_t start_pc, uint32_t end_pc, const uint8_t* block_buffer) {
    (void)end_pc;
    (void)block_buffer;

    CPUState *cpu = current_cpu;
    if (!cpu) {
        cpu = first_cpu;
    }
    if (!cpu) return;

    TCGTBCPUState s = {
        .pc      = (vaddr)start_pc,
        .cs_base = 0,
        .flags   = 0,
        .cflags  = 0,
    };

    mmap_lock();
    TranslationBlock *tb = tb_gen_code(cpu, s);
    mmap_unlock();
    (void)tb;
}
