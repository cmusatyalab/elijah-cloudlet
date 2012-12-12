# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/vmtest_headers.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../include/linux/kconfig.h" 1



# 1 "include/generated/autoconf.h" 1
# 5 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../include/linux/kconfig.h" 2
# 1 "<command-line>" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/vmtest_headers.c"
# 1 "include/linux/mmzone.h" 1






# 1 "include/linux/spinlock.h" 1
# 49 "include/linux/spinlock.h"
# 1 "include/linux/typecheck.h" 1
# 50 "include/linux/spinlock.h" 2
# 1 "include/linux/preempt.h" 1
# 9 "include/linux/preempt.h"
# 1 "include/linux/thread_info.h" 1
# 10 "include/linux/thread_info.h"
# 1 "include/linux/types.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/types.h" 1



# 1 "include/asm-generic/types.h" 1






# 1 "include/asm-generic/int-ll64.h" 1
# 11 "include/asm-generic/int-ll64.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitsperlong.h" 1
# 10 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitsperlong.h"
# 1 "include/asm-generic/bitsperlong.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitsperlong.h" 2
# 12 "include/asm-generic/int-ll64.h" 2







typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;


__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
# 42 "include/asm-generic/int-ll64.h"
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;
# 8 "include/asm-generic/types.h" 2



typedef unsigned short umode_t;
# 5 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/types.h" 2
# 5 "include/linux/types.h" 2
# 17 "include/linux/types.h"
# 1 "include/linux/posix_types.h" 1



# 1 "include/linux/stddef.h" 1



# 1 "include/linux/compiler.h" 1
# 48 "include/linux/compiler.h"
# 1 "include/linux/compiler-gcc.h" 1
# 94 "include/linux/compiler-gcc.h"
# 1 "include/linux/compiler-gcc4.h" 1
# 95 "include/linux/compiler-gcc.h" 2
# 49 "include/linux/compiler.h" 2
# 66 "include/linux/compiler.h"
struct ftrace_branch_data {
 const char *func;
 const char *file;
 unsigned line;
 union {
  struct {
   unsigned long correct;
   unsigned long incorrect;
  };
  struct {
   unsigned long miss;
   unsigned long hit;
  };
  unsigned long miss_hit[2];
 };
};
# 5 "include/linux/stddef.h" 2
# 15 "include/linux/stddef.h"
enum {
 false = 0,
 true = 1
};
# 5 "include/linux/posix_types.h" 2
# 36 "include/linux/posix_types.h"
typedef struct {
 unsigned long fds_bits [(1024/(8 * sizeof(unsigned long)))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/posix_types.h" 1


# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/posix_types_32.h" 1
# 10 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/posix_types_32.h"
typedef unsigned long __kernel_ino_t;
typedef unsigned short __kernel_mode_t;
typedef unsigned short __kernel_nlink_t;
typedef long __kernel_off_t;
typedef int __kernel_pid_t;
typedef unsigned short __kernel_ipc_pid_t;
typedef unsigned short __kernel_uid_t;
typedef unsigned short __kernel_gid_t;
typedef unsigned int __kernel_size_t;
typedef int __kernel_ssize_t;
typedef int __kernel_ptrdiff_t;
typedef long __kernel_time_t;
typedef long __kernel_suseconds_t;
typedef long __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef int __kernel_daddr_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;

typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;
typedef unsigned short __kernel_old_dev_t;


typedef long long __kernel_loff_t;


typedef struct {
 int val[2];
} __kernel_fsid_t;
# 4 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/posix_types.h" 2
# 48 "include/linux/posix_types.h" 2
# 18 "include/linux/types.h" 2



typedef __u32 __kernel_dev_t;

typedef __kernel_fd_set fd_set;
typedef __kernel_dev_t dev_t;
typedef __kernel_ino_t ino_t;
typedef __kernel_mode_t mode_t;
typedef __kernel_nlink_t nlink_t;
typedef __kernel_off_t off_t;
typedef __kernel_pid_t pid_t;
typedef __kernel_daddr_t daddr_t;
typedef __kernel_key_t key_t;
typedef __kernel_suseconds_t suseconds_t;
typedef __kernel_timer_t timer_t;
typedef __kernel_clockid_t clockid_t;
typedef __kernel_mqd_t mqd_t;

typedef _Bool bool;

typedef __kernel_uid32_t uid_t;
typedef __kernel_gid32_t gid_t;
typedef __kernel_uid16_t uid16_t;
typedef __kernel_gid16_t gid16_t;

typedef unsigned long uintptr_t;



typedef __kernel_old_uid_t old_uid_t;
typedef __kernel_old_gid_t old_gid_t;



typedef __kernel_loff_t loff_t;
# 62 "include/linux/types.h"
typedef __kernel_size_t size_t;




typedef __kernel_ssize_t ssize_t;




typedef __kernel_ptrdiff_t ptrdiff_t;




typedef __kernel_time_t time_t;




typedef __kernel_clock_t clock_t;




typedef __kernel_caddr_t caddr_t;



typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;


typedef unsigned char unchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;




typedef __u8 u_int8_t;
typedef __s8 int8_t;
typedef __u16 u_int16_t;
typedef __s16 int16_t;
typedef __u32 u_int32_t;
typedef __s32 int32_t;



typedef __u8 uint8_t;
typedef __u16 uint16_t;
typedef __u32 uint32_t;


typedef __u64 uint64_t;
typedef __u64 u_int64_t;
typedef __s64 int64_t;
# 138 "include/linux/types.h"
typedef u64 sector_t;
typedef u64 blkcnt_t;
# 154 "include/linux/types.h"
typedef u64 dma_addr_t;
# 177 "include/linux/types.h"
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;
# 201 "include/linux/types.h"
typedef unsigned gfp_t;
typedef unsigned fmode_t;


typedef u64 phys_addr_t;




typedef phys_addr_t resource_size_t;

typedef struct {
 int counter;
} atomic_t;







struct list_head {
 struct list_head *next, *prev;
};

struct hlist_head {
 struct hlist_node *first;
};

struct hlist_node {
 struct hlist_node *next, **pprev;
};

struct ustat {
 __kernel_daddr_t f_tfree;
 __kernel_ino_t f_tinode;
 char f_fname[6];
 char f_fpack[6];
};






struct rcu_head {
 struct rcu_head *next;
 void (*func)(struct rcu_head *head);
};
# 11 "include/linux/thread_info.h" 2

struct timespec;
struct compat_timespec;




struct restart_block {
 long (*fn)(struct restart_block *);
 union {

  struct {
   u32 *uaddr;
   u32 val;
   u32 flags;
   u32 bitset;
   u64 time;
   u32 *uaddr2;
  } futex;

  struct {
   clockid_t clockid;
   struct timespec *rmtp;



   u64 expires;
  } nanosleep;

  struct {
   struct pollfd *ufds;
   int nfds;
   int has_timeout;
   unsigned long tv_sec;
   unsigned long tv_nsec;
  } poll;
 };
};

extern long do_no_restart_syscall(struct restart_block *parm);

# 1 "include/linux/bitops.h" 1
# 13 "include/linux/bitops.h"
extern unsigned int __sw_hweight8(unsigned int w);
extern unsigned int __sw_hweight16(unsigned int w);
extern unsigned int __sw_hweight32(unsigned int w);
extern unsigned long __sw_hweight64(__u64 w);





# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 1
# 16 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h" 1





# 1 "include/linux/stringify.h" 1
# 7 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/asm.h" 1
# 8 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h" 2
# 45 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h"
struct alt_instr {
 s32 instr_offset;
 s32 repl_offset;
 u16 cpuid;
 u8 instrlen;
 u8 replacementlen;
};

extern void alternative_instructions(void);
extern void apply_alternatives(struct alt_instr *start, struct alt_instr *end);

struct module;


extern void alternatives_smp_module_add(struct module *mod, char *name,
     void *locks, void *locks_end,
     void *text, void *text_end);
extern void alternatives_smp_module_del(struct module *mod);
extern void alternatives_smp_switch(int smp);
extern int alternatives_text_reserved(void *start, void *end);
extern bool skip_smp_alternatives;
# 100 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cpufeature.h" 1






# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/required-features.h" 1
# 8 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cpufeature.h" 2
# 210 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cpufeature.h"
# 1 "include/linux/bitops.h" 1
# 211 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cpufeature.h" 2

extern const char * const x86_cap_flags[10*32];
extern const char * const x86_power_flags[32];
# 101 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h" 2
# 148 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h"
struct paravirt_patch_site;

void apply_paravirt(struct paravirt_patch_site *start,
      struct paravirt_patch_site *end);
# 160 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h"
extern void *text_poke_early(void *addr, const void *opcode, size_t len);
# 179 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/alternative.h"
struct text_poke_param {
 void *addr;
 const void *opcode;
 size_t len;
};

extern void *text_poke(void *addr, const void *opcode, size_t len);
extern void *text_poke_smp(void *addr, const void *opcode, size_t len);
extern void text_poke_smp_batch(struct text_poke_param *params, int n);
# 17 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2
# 59 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline __attribute__((always_inline)) void
set_bit(unsigned int nr, volatile unsigned long *addr)
{
 if ((__builtin_constant_p(nr))) {
  asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "orb %1,%0"
   : "+m" (*(volatile long *) ((void *)(addr) + ((nr)>>3)))
   : "iq" ((u8)(1 << ((nr) & 7)))
   : "memory");
 } else {
  asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "bts %1,%0"
   : "+m" (*(volatile long *) (addr)) : "Ir" (nr) : "memory");
 }
}
# 82 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline void __set_bit(int nr, volatile unsigned long *addr)
{
 asm volatile("bts %1,%0" : "+m" (*(volatile long *) (addr)) : "Ir" (nr) : "memory");
}
# 97 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline __attribute__((always_inline)) void
clear_bit(int nr, volatile unsigned long *addr)
{
 if ((__builtin_constant_p(nr))) {
  asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "andb %1,%0"
   : "+m" (*(volatile long *) ((void *)(addr) + ((nr)>>3)))
   : "iq" ((u8)~(1 << ((nr) & 7))));
 } else {
  asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "btr %1,%0"
   : "+m" (*(volatile long *) (addr))
   : "Ir" (nr));
 }
}
# 119 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline void clear_bit_unlock(unsigned nr, volatile unsigned long *addr)
{
 __asm__ __volatile__("": : :"memory");
 clear_bit(nr, addr);
}

static inline void __clear_bit(int nr, volatile unsigned long *addr)
{
 asm volatile("btr %1,%0" : "+m" (*(volatile long *) (addr)) : "Ir" (nr));
}
# 142 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline void __clear_bit_unlock(unsigned nr, volatile unsigned long *addr)
{
 __asm__ __volatile__("": : :"memory");
 __clear_bit(nr, addr);
}
# 160 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline void __change_bit(int nr, volatile unsigned long *addr)
{
 asm volatile("btc %1,%0" : "+m" (*(volatile long *) (addr)) : "Ir" (nr));
}
# 174 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline void change_bit(int nr, volatile unsigned long *addr)
{
 if ((__builtin_constant_p(nr))) {
  asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xorb %1,%0"
   : "+m" (*(volatile long *) ((void *)(addr) + ((nr)>>3)))
   : "iq" ((u8)(1 << ((nr) & 7))));
 } else {
  asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "btc %1,%0"
   : "+m" (*(volatile long *) (addr))
   : "Ir" (nr));
 }
}
# 195 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
 int oldbit;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "bts %2,%1\n\t"
       "sbb %0,%0" : "=r" (oldbit), "+m" (*(volatile long *) (addr)) : "Ir" (nr) : "memory");

 return oldbit;
}
# 212 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline __attribute__((always_inline)) int
test_and_set_bit_lock(int nr, volatile unsigned long *addr)
{
 return test_and_set_bit(nr, addr);
}
# 227 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int __test_and_set_bit(int nr, volatile unsigned long *addr)
{
 int oldbit;

 asm("bts %2,%1\n\t"
     "sbb %0,%0"
     : "=r" (oldbit), "+m" (*(volatile long *) (addr))
     : "Ir" (nr));
 return oldbit;
}
# 246 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
 int oldbit;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "btr %2,%1\n\t"
       "sbb %0,%0"
       : "=r" (oldbit), "+m" (*(volatile long *) (addr)) : "Ir" (nr) : "memory");

 return oldbit;
}
# 266 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int __test_and_clear_bit(int nr, volatile unsigned long *addr)
{
 int oldbit;

 asm volatile("btr %2,%1\n\t"
       "sbb %0,%0"
       : "=r" (oldbit), "+m" (*(volatile long *) (addr))
       : "Ir" (nr));
 return oldbit;
}


static inline int __test_and_change_bit(int nr, volatile unsigned long *addr)
{
 int oldbit;

 asm volatile("btc %2,%1\n\t"
       "sbb %0,%0"
       : "=r" (oldbit), "+m" (*(volatile long *) (addr))
       : "Ir" (nr) : "memory");

 return oldbit;
}
# 298 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int test_and_change_bit(int nr, volatile unsigned long *addr)
{
 int oldbit;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "btc %2,%1\n\t"
       "sbb %0,%0"
       : "=r" (oldbit), "+m" (*(volatile long *) (addr)) : "Ir" (nr) : "memory");

 return oldbit;
}

static inline __attribute__((always_inline)) int constant_test_bit(unsigned int nr, const volatile unsigned long *addr)
{
 return ((1UL << (nr % 32)) &
  (addr[nr / 32])) != 0;
}

static inline int variable_test_bit(int nr, volatile const unsigned long *addr)
{
 int oldbit;

 asm volatile("bt %2,%1\n\t"
       "sbb %0,%0"
       : "=r" (oldbit)
       : "m" (*(unsigned long *)addr), "Ir" (nr));

 return oldbit;
}
# 347 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline unsigned long __ffs(unsigned long word)
{
 asm("bsf %1,%0"
  : "=r" (word)
  : "rm" (word));
 return word;
}







static inline unsigned long ffz(unsigned long word)
{
 asm("bsf %1,%0"
  : "=r" (word)
  : "r" (~word));
 return word;
}







static inline unsigned long __fls(unsigned long word)
{
 asm("bsr %1,%0"
     : "=r" (word)
     : "rm" (word));
 return word;
}
# 395 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int ffs(int x)
{
 int r;

 asm("bsfl %1,%0\n\t"
     "cmovzl %2,%0"
     : "=r" (r) : "rm" (x), "r" (-1));






 return r + 1;
}
# 422 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h"
static inline int fls(int x)
{
 int r;

 asm("bsrl %1,%0\n\t"
     "cmovzl %2,%0"
     : "=&r" (r) : "rm" (x), "rm" (-1));






 return r + 1;
}






# 1 "include/asm-generic/bitops/find.h" 1
# 11 "include/asm-generic/bitops/find.h"
extern unsigned long find_next_bit(const unsigned long *addr, unsigned long
  size, unsigned long offset);
# 22 "include/asm-generic/bitops/find.h"
extern unsigned long find_next_zero_bit(const unsigned long *addr, unsigned
  long size, unsigned long offset);
# 35 "include/asm-generic/bitops/find.h"
extern unsigned long find_first_bit(const unsigned long *addr,
        unsigned long size);
# 45 "include/asm-generic/bitops/find.h"
extern unsigned long find_first_zero_bit(const unsigned long *addr,
      unsigned long size);
# 444 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2

# 1 "include/asm-generic/bitops/sched.h" 1
# 12 "include/asm-generic/bitops/sched.h"
static inline int sched_find_first_bit(const unsigned long *b)
{





 if (b[0])
  return __ffs(b[0]);
 if (b[1])
  return __ffs(b[1]) + 32;
 if (b[2])
  return __ffs(b[2]) + 64;
 return __ffs(b[3]) + 96;



}
# 446 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/arch_hweight.h" 1
# 24 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/arch_hweight.h"
static inline unsigned int __arch_hweight32(unsigned int w)
{
 unsigned int res = 0;

 asm ("661:\n\t" "call __sw_hweight32" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(4*32+23)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" ".byte 0xf3,0x0f,0xb8,0xc0" "\n664:\n" ".previous"
       : "=""a" (res)
       : "a" (w));

 return res;
}

static inline unsigned int __arch_hweight16(unsigned int w)
{
 return __arch_hweight32(w & 0xffff);
}

static inline unsigned int __arch_hweight8(unsigned int w)
{
 return __arch_hweight32(w & 0xff);
}

static inline unsigned long __arch_hweight64(__u64 w)
{
 unsigned long res = 0;


 return __arch_hweight32((u32)w) +
  __arch_hweight32((u32)(w >> 32));






 return res;
}
# 450 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2

# 1 "include/asm-generic/bitops/const_hweight.h" 1
# 452 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2



# 1 "include/asm-generic/bitops/fls64.h" 1
# 18 "include/asm-generic/bitops/fls64.h"
static inline __attribute__((always_inline)) int fls64(__u64 x)
{
 __u32 h = x >> 32;
 if (h)
  return fls(h) + 32;
 return fls(x);
}
# 456 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2



# 1 "include/asm-generic/bitops/le.h" 1




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/byteorder.h" 1



# 1 "include/linux/byteorder/little_endian.h" 1
# 12 "include/linux/byteorder/little_endian.h"
# 1 "include/linux/swab.h" 1





# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/swab.h" 1






static inline __attribute__((__const__)) __u32 __arch_swab32(__u32 val)
{


 asm("bswap %0" : "=r" (val) : "0" (val));
# 25 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/swab.h"
 return val;
}


static inline __attribute__((__const__)) __u64 __arch_swab64(__u64 val)
{

 union {
  struct {
   __u32 a;
   __u32 b;
  } s;
  __u64 u;
 } v;
 v.u = val;

 asm("bswapl %0 ; bswapl %1 ; xchgl %0,%1"
     : "=r" (v.s.a), "=r" (v.s.b)
     : "0" (v.s.a), "1" (v.s.b));







 return v.u;






}
# 7 "include/linux/swab.h" 2
# 46 "include/linux/swab.h"
static inline __attribute__((__const__)) __u16 __fswab16(__u16 val)
{



 return ((__u16)( (((__u16)(val) & (__u16)0x00ffU) << 8) | (((__u16)(val) & (__u16)0xff00U) >> 8)));

}

static inline __attribute__((__const__)) __u32 __fswab32(__u32 val)
{

 return __arch_swab32(val);



}

static inline __attribute__((__const__)) __u64 __fswab64(__u64 val)
{

 return __arch_swab64(val);







}

static inline __attribute__((__const__)) __u32 __fswahw32(__u32 val)
{



 return ((__u32)( (((__u32)(val) & (__u32)0x0000ffffUL) << 16) | (((__u32)(val) & (__u32)0xffff0000UL) >> 16)));

}

static inline __attribute__((__const__)) __u32 __fswahb32(__u32 val)
{



 return ((__u32)( (((__u32)(val) & (__u32)0x00ff00ffUL) << 8) | (((__u32)(val) & (__u32)0xff00ff00UL) >> 8)));

}
# 148 "include/linux/swab.h"
static inline __u16 __swab16p(const __u16 *p)
{



 return (__builtin_constant_p((__u16)(*p)) ? ((__u16)( (((__u16)(*p) & (__u16)0x00ffU) << 8) | (((__u16)(*p) & (__u16)0xff00U) >> 8))) : __fswab16(*p));

}





static inline __u32 __swab32p(const __u32 *p)
{



 return (__builtin_constant_p((__u32)(*p)) ? ((__u32)( (((__u32)(*p) & (__u32)0x000000ffUL) << 24) | (((__u32)(*p) & (__u32)0x0000ff00UL) << 8) | (((__u32)(*p) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(*p) & (__u32)0xff000000UL) >> 24))) : __fswab32(*p));

}





static inline __u64 __swab64p(const __u64 *p)
{



 return (__builtin_constant_p((__u64)(*p)) ? ((__u64)( (((__u64)(*p) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(*p) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(*p) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(*p) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(*p) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(*p) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(*p) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(*p) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(*p));

}







static inline __u32 __swahw32p(const __u32 *p)
{



 return (__builtin_constant_p((__u32)(*p)) ? ((__u32)( (((__u32)(*p) & (__u32)0x0000ffffUL) << 16) | (((__u32)(*p) & (__u32)0xffff0000UL) >> 16))) : __fswahw32(*p));

}







static inline __u32 __swahb32p(const __u32 *p)
{



 return (__builtin_constant_p((__u32)(*p)) ? ((__u32)( (((__u32)(*p) & (__u32)0x00ff00ffUL) << 8) | (((__u32)(*p) & (__u32)0xff00ff00UL) >> 8))) : __fswahb32(*p));

}





static inline void __swab16s(__u16 *p)
{



 *p = __swab16p(p);

}




static inline void __swab32s(__u32 *p)
{



 *p = __swab32p(p);

}





static inline void __swab64s(__u64 *p)
{



 *p = __swab64p(p);

}







static inline void __swahw32s(__u32 *p)
{



 *p = __swahw32p(p);

}







static inline void __swahb32s(__u32 *p)
{



 *p = __swahb32p(p);

}
# 13 "include/linux/byteorder/little_endian.h" 2
# 43 "include/linux/byteorder/little_endian.h"
static inline __le64 __cpu_to_le64p(const __u64 *p)
{
 return ( __le64)*p;
}
static inline __u64 __le64_to_cpup(const __le64 *p)
{
 return ( __u64)*p;
}
static inline __le32 __cpu_to_le32p(const __u32 *p)
{
 return ( __le32)*p;
}
static inline __u32 __le32_to_cpup(const __le32 *p)
{
 return ( __u32)*p;
}
static inline __le16 __cpu_to_le16p(const __u16 *p)
{
 return ( __le16)*p;
}
static inline __u16 __le16_to_cpup(const __le16 *p)
{
 return ( __u16)*p;
}
static inline __be64 __cpu_to_be64p(const __u64 *p)
{
 return ( __be64)__swab64p(p);
}
static inline __u64 __be64_to_cpup(const __be64 *p)
{
 return __swab64p((__u64 *)p);
}
static inline __be32 __cpu_to_be32p(const __u32 *p)
{
 return ( __be32)__swab32p(p);
}
static inline __u32 __be32_to_cpup(const __be32 *p)
{
 return __swab32p((__u32 *)p);
}
static inline __be16 __cpu_to_be16p(const __u16 *p)
{
 return ( __be16)__swab16p(p);
}
static inline __u16 __be16_to_cpup(const __be16 *p)
{
 return __swab16p((__u16 *)p);
}
# 105 "include/linux/byteorder/little_endian.h"
# 1 "include/linux/byteorder/generic.h" 1
# 143 "include/linux/byteorder/generic.h"
static inline void le16_add_cpu(__le16 *var, u16 val)
{
 *var = (( __le16)(__u16)((( __u16)(__le16)(*var)) + val));
}

static inline void le32_add_cpu(__le32 *var, u32 val)
{
 *var = (( __le32)(__u32)((( __u32)(__le32)(*var)) + val));
}

static inline void le64_add_cpu(__le64 *var, u64 val)
{
 *var = (( __le64)(__u64)((( __u64)(__le64)(*var)) + val));
}

static inline void be16_add_cpu(__be16 *var, u16 val)
{
 *var = (( __be16)(__builtin_constant_p((__u16)(((__builtin_constant_p((__u16)(( __u16)(__be16)(*var))) ? ((__u16)( (((__u16)(( __u16)(__be16)(*var)) & (__u16)0x00ffU) << 8) | (((__u16)(( __u16)(__be16)(*var)) & (__u16)0xff00U) >> 8))) : __fswab16(( __u16)(__be16)(*var))) + val))) ? ((__u16)( (((__u16)(((__builtin_constant_p((__u16)(( __u16)(__be16)(*var))) ? ((__u16)( (((__u16)(( __u16)(__be16)(*var)) & (__u16)0x00ffU) << 8) | (((__u16)(( __u16)(__be16)(*var)) & (__u16)0xff00U) >> 8))) : __fswab16(( __u16)(__be16)(*var))) + val)) & (__u16)0x00ffU) << 8) | (((__u16)(((__builtin_constant_p((__u16)(( __u16)(__be16)(*var))) ? ((__u16)( (((__u16)(( __u16)(__be16)(*var)) & (__u16)0x00ffU) << 8) | (((__u16)(( __u16)(__be16)(*var)) & (__u16)0xff00U) >> 8))) : __fswab16(( __u16)(__be16)(*var))) + val)) & (__u16)0xff00U) >> 8))) : __fswab16(((__builtin_constant_p((__u16)(( __u16)(__be16)(*var))) ? ((__u16)( (((__u16)(( __u16)(__be16)(*var)) & (__u16)0x00ffU) << 8) | (((__u16)(( __u16)(__be16)(*var)) & (__u16)0xff00U) >> 8))) : __fswab16(( __u16)(__be16)(*var))) + val))));
}

static inline void be32_add_cpu(__be32 *var, u32 val)
{
 *var = (( __be32)(__builtin_constant_p((__u32)(((__builtin_constant_p((__u32)(( __u32)(__be32)(*var))) ? ((__u32)( (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x000000ffUL) << 24) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0xff000000UL) >> 24))) : __fswab32(( __u32)(__be32)(*var))) + val))) ? ((__u32)( (((__u32)(((__builtin_constant_p((__u32)(( __u32)(__be32)(*var))) ? ((__u32)( (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x000000ffUL) << 24) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0xff000000UL) >> 24))) : __fswab32(( __u32)(__be32)(*var))) + val)) & (__u32)0x000000ffUL) << 24) | (((__u32)(((__builtin_constant_p((__u32)(( __u32)(__be32)(*var))) ? ((__u32)( (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x000000ffUL) << 24) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0xff000000UL) >> 24))) : __fswab32(( __u32)(__be32)(*var))) + val)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(((__builtin_constant_p((__u32)(( __u32)(__be32)(*var))) ? ((__u32)( (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x000000ffUL) << 24) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0xff000000UL) >> 24))) : __fswab32(( __u32)(__be32)(*var))) + val)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(((__builtin_constant_p((__u32)(( __u32)(__be32)(*var))) ? ((__u32)( (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x000000ffUL) << 24) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0xff000000UL) >> 24))) : __fswab32(( __u32)(__be32)(*var))) + val)) & (__u32)0xff000000UL) >> 24))) : __fswab32(((__builtin_constant_p((__u32)(( __u32)(__be32)(*var))) ? ((__u32)( (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x000000ffUL) << 24) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x0000ff00UL) << 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0x00ff0000UL) >> 8) | (((__u32)(( __u32)(__be32)(*var)) & (__u32)0xff000000UL) >> 24))) : __fswab32(( __u32)(__be32)(*var))) + val))));
}

static inline void be64_add_cpu(__be64 *var, u64 val)
{
 *var = (( __be64)(__builtin_constant_p((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val))) ? ((__u64)( (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(((__builtin_constant_p((__u64)(( __u64)(__be64)(*var))) ? ((__u64)( (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000000000ffULL) << 56) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000000000ff00ULL) << 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000000000ff0000ULL) << 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00000000ff000000ULL) << 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x000000ff00000000ULL) >> 8) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x0000ff0000000000ULL) >> 24) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0x00ff000000000000ULL) >> 40) | (((__u64)(( __u64)(__be64)(*var)) & (__u64)0xff00000000000000ULL) >> 56))) : __fswab64(( __u64)(__be64)(*var))) + val))));
}
# 106 "include/linux/byteorder/little_endian.h" 2
# 5 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/byteorder.h" 2
# 6 "include/asm-generic/bitops/le.h" 2





static inline unsigned long find_next_zero_bit_le(const void *addr,
  unsigned long size, unsigned long offset)
{
 return find_next_zero_bit(addr, size, offset);
}

static inline unsigned long find_next_bit_le(const void *addr,
  unsigned long size, unsigned long offset)
{
 return find_next_bit(addr, size, offset);
}

static inline unsigned long find_first_zero_bit_le(const void *addr,
  unsigned long size)
{
 return find_first_zero_bit(addr, size);
}
# 52 "include/asm-generic/bitops/le.h"
static inline int test_bit_le(int nr, const void *addr)
{
 return (__builtin_constant_p((nr ^ 0)) ? constant_test_bit((nr ^ 0), (addr)) : variable_test_bit((nr ^ 0), (addr)));
}

static inline void __set_bit_le(int nr, void *addr)
{
 __set_bit(nr ^ 0, addr);
}

static inline void __clear_bit_le(int nr, void *addr)
{
 __clear_bit(nr ^ 0, addr);
}

static inline int test_and_set_bit_le(int nr, void *addr)
{
 return test_and_set_bit(nr ^ 0, addr);
}

static inline int test_and_clear_bit_le(int nr, void *addr)
{
 return test_and_clear_bit(nr ^ 0, addr);
}

static inline int __test_and_set_bit_le(int nr, void *addr)
{
 return __test_and_set_bit(nr ^ 0, addr);
}

static inline int __test_and_clear_bit_le(int nr, void *addr)
{
 return __test_and_clear_bit(nr ^ 0, addr);
}
# 460 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2

# 1 "include/asm-generic/bitops/ext2-atomic-setbit.h" 1
# 462 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bitops.h" 2
# 23 "include/linux/bitops.h" 2






static __inline__ int get_bitmask_order(unsigned int count)
{
 int order;

 order = fls(count);
 return order;
}

static __inline__ int get_count_order(unsigned int count)
{
 int order;

 order = fls(count) - 1;
 if (count & (count - 1))
  order++;
 return order;
}

static inline unsigned long hweight_long(unsigned long w)
{
 return sizeof(w) == 4 ? (__builtin_constant_p(w) ? ((( (!!((w) & (1ULL << 0))) + (!!((w) & (1ULL << 1))) + (!!((w) & (1ULL << 2))) + (!!((w) & (1ULL << 3))) + (!!((w) & (1ULL << 4))) + (!!((w) & (1ULL << 5))) + (!!((w) & (1ULL << 6))) + (!!((w) & (1ULL << 7))) ) + ( (!!(((w) >> 8) & (1ULL << 0))) + (!!(((w) >> 8) & (1ULL << 1))) + (!!(((w) >> 8) & (1ULL << 2))) + (!!(((w) >> 8) & (1ULL << 3))) + (!!(((w) >> 8) & (1ULL << 4))) + (!!(((w) >> 8) & (1ULL << 5))) + (!!(((w) >> 8) & (1ULL << 6))) + (!!(((w) >> 8) & (1ULL << 7))) )) + (( (!!(((w) >> 16) & (1ULL << 0))) + (!!(((w) >> 16) & (1ULL << 1))) + (!!(((w) >> 16) & (1ULL << 2))) + (!!(((w) >> 16) & (1ULL << 3))) + (!!(((w) >> 16) & (1ULL << 4))) + (!!(((w) >> 16) & (1ULL << 5))) + (!!(((w) >> 16) & (1ULL << 6))) + (!!(((w) >> 16) & (1ULL << 7))) ) + ( (!!((((w) >> 16) >> 8) & (1ULL << 0))) + (!!((((w) >> 16) >> 8) & (1ULL << 1))) + (!!((((w) >> 16) >> 8) & (1ULL << 2))) + (!!((((w) >> 16) >> 8) & (1ULL << 3))) + (!!((((w) >> 16) >> 8) & (1ULL << 4))) + (!!((((w) >> 16) >> 8) & (1ULL << 5))) + (!!((((w) >> 16) >> 8) & (1ULL << 6))) + (!!((((w) >> 16) >> 8) & (1ULL << 7))) ))) : __arch_hweight32(w)) : (__builtin_constant_p(w) ? (((( (!!((w) & (1ULL << 0))) + (!!((w) & (1ULL << 1))) + (!!((w) & (1ULL << 2))) + (!!((w) & (1ULL << 3))) + (!!((w) & (1ULL << 4))) + (!!((w) & (1ULL << 5))) + (!!((w) & (1ULL << 6))) + (!!((w) & (1ULL << 7))) ) + ( (!!(((w) >> 8) & (1ULL << 0))) + (!!(((w) >> 8) & (1ULL << 1))) + (!!(((w) >> 8) & (1ULL << 2))) + (!!(((w) >> 8) & (1ULL << 3))) + (!!(((w) >> 8) & (1ULL << 4))) + (!!(((w) >> 8) & (1ULL << 5))) + (!!(((w) >> 8) & (1ULL << 6))) + (!!(((w) >> 8) & (1ULL << 7))) )) + (( (!!(((w) >> 16) & (1ULL << 0))) + (!!(((w) >> 16) & (1ULL << 1))) + (!!(((w) >> 16) & (1ULL << 2))) + (!!(((w) >> 16) & (1ULL << 3))) + (!!(((w) >> 16) & (1ULL << 4))) + (!!(((w) >> 16) & (1ULL << 5))) + (!!(((w) >> 16) & (1ULL << 6))) + (!!(((w) >> 16) & (1ULL << 7))) ) + ( (!!((((w) >> 16) >> 8) & (1ULL << 0))) + (!!((((w) >> 16) >> 8) & (1ULL << 1))) + (!!((((w) >> 16) >> 8) & (1ULL << 2))) + (!!((((w) >> 16) >> 8) & (1ULL << 3))) + (!!((((w) >> 16) >> 8) & (1ULL << 4))) + (!!((((w) >> 16) >> 8) & (1ULL << 5))) + (!!((((w) >> 16) >> 8) & (1ULL << 6))) + (!!((((w) >> 16) >> 8) & (1ULL << 7))) ))) + ((( (!!(((w) >> 32) & (1ULL << 0))) + (!!(((w) >> 32) & (1ULL << 1))) + (!!(((w) >> 32) & (1ULL << 2))) + (!!(((w) >> 32) & (1ULL << 3))) + (!!(((w) >> 32) & (1ULL << 4))) + (!!(((w) >> 32) & (1ULL << 5))) + (!!(((w) >> 32) & (1ULL << 6))) + (!!(((w) >> 32) & (1ULL << 7))) ) + ( (!!((((w) >> 32) >> 8) & (1ULL << 0))) + (!!((((w) >> 32) >> 8) & (1ULL << 1))) + (!!((((w) >> 32) >> 8) & (1ULL << 2))) + (!!((((w) >> 32) >> 8) & (1ULL << 3))) + (!!((((w) >> 32) >> 8) & (1ULL << 4))) + (!!((((w) >> 32) >> 8) & (1ULL << 5))) + (!!((((w) >> 32) >> 8) & (1ULL << 6))) + (!!((((w) >> 32) >> 8) & (1ULL << 7))) )) + (( (!!((((w) >> 32) >> 16) & (1ULL << 0))) + (!!((((w) >> 32) >> 16) & (1ULL << 1))) + (!!((((w) >> 32) >> 16) & (1ULL << 2))) + (!!((((w) >> 32) >> 16) & (1ULL << 3))) + (!!((((w) >> 32) >> 16) & (1ULL << 4))) + (!!((((w) >> 32) >> 16) & (1ULL << 5))) + (!!((((w) >> 32) >> 16) & (1ULL << 6))) + (!!((((w) >> 32) >> 16) & (1ULL << 7))) ) + ( (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 0))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 1))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 2))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 3))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 4))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 5))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 6))) + (!!(((((w) >> 32) >> 16) >> 8) & (1ULL << 7))) )))) : __arch_hweight64(w));
}






static inline __u64 rol64(__u64 word, unsigned int shift)
{
 return (word << shift) | (word >> (64 - shift));
}






static inline __u64 ror64(__u64 word, unsigned int shift)
{
 return (word >> shift) | (word << (64 - shift));
}






static inline __u32 rol32(__u32 word, unsigned int shift)
{
 return (word << shift) | (word >> (32 - shift));
}






static inline __u32 ror32(__u32 word, unsigned int shift)
{
 return (word >> shift) | (word << (32 - shift));
}






static inline __u16 rol16(__u16 word, unsigned int shift)
{
 return (word << shift) | (word >> (16 - shift));
}






static inline __u16 ror16(__u16 word, unsigned int shift)
{
 return (word >> shift) | (word << (16 - shift));
}






static inline __u8 rol8(__u8 word, unsigned int shift)
{
 return (word << shift) | (word >> (8 - shift));
}






static inline __u8 ror8(__u8 word, unsigned int shift)
{
 return (word >> shift) | (word << (8 - shift));
}






static inline __s32 sign_extend32(__u32 value, int index)
{
 __u8 shift = 31 - index;
 return (__s32)(value << shift) >> shift;
}

static inline unsigned fls_long(unsigned long l)
{
 if (sizeof(l) == 4)
  return fls(l);
 return fls64(l);
}
# 158 "include/linux/bitops.h"
static inline unsigned long __ffs64(u64 word)
{

 if (((u32)word) == 0UL)
  return __ffs((u32)(word >> 32)) + 32;



 return __ffs((unsigned long)word);
}
# 179 "include/linux/bitops.h"
extern unsigned long find_last_bit(const unsigned long *addr,
       unsigned long size);
# 53 "include/linux/thread_info.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page.h" 1







# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_types.h" 1



# 1 "include/linux/const.h" 1
# 5 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_types.h" 2
# 39 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_types.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_32_types.h" 1
# 49 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_32_types.h"
extern unsigned int __VMALLOC_RESERVE;
extern int sysctl_legacy_va_layout;

extern void find_low_pfn_range(void);
extern void setup_bootmem_allocator(void);
# 40 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_types.h" 2




extern int devmem_is_allowed(unsigned long pagenr);

extern unsigned long max_low_pfn_mapped;
extern unsigned long max_pfn_mapped;

static inline phys_addr_t get_max_mapped(void)
{
 return (phys_addr_t)max_pfn_mapped << 12;
}

extern unsigned long init_memory_mapping(unsigned long start,
      unsigned long end);

extern void initmem_init(void);
extern void free_initmem(void);
# 9 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page.h" 2




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_32.h" 1
# 37 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_32.h"
# 1 "include/linux/string.h" 1
# 13 "include/linux/string.h"
# 1 "/usr/lib/gcc/i686-linux-gnu/4.6/include/stdarg.h" 1 3 4
# 40 "/usr/lib/gcc/i686-linux-gnu/4.6/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 102 "/usr/lib/gcc/i686-linux-gnu/4.6/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 14 "include/linux/string.h" 2

extern char *strndup_user(const char *, long);
extern void *memdup_user(const void *, size_t);




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string.h" 1

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string_32.h" 1
# 9 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string_32.h"
extern char *strcpy(char *dest, const char *src);


extern char *strncpy(char *dest, const char *src, size_t count);


extern char *strcat(char *dest, const char *src);


extern char *strncat(char *dest, const char *src, size_t count);


extern int strcmp(const char *cs, const char *ct);


extern int strncmp(const char *cs, const char *ct, size_t count);


extern char *strchr(const char *s, int c);


extern size_t strlen(const char *s);

static inline __attribute__((always_inline)) void *__memcpy(void *to, const void *from, size_t n)
{
 int d0, d1, d2;
 asm volatile("rep ; movsl\n\t"
       "movl %4,%%ecx\n\t"
       "andl $3,%%ecx\n\t"
       "jz 1f\n\t"
       "rep ; movsb\n\t"
       "1:"
       : "=&c" (d0), "=&D" (d1), "=&S" (d2)
       : "0" (n / 4), "g" (n), "1" ((long)to), "2" ((long)from)
       : "memory");
 return to;
}





static inline __attribute__((always_inline)) void *__constant_memcpy(void *to, const void *from,
            size_t n)
{
 long esi, edi;
 if (!n)
  return to;

 switch (n) {
 case 1:
  *(char *)to = *(char *)from;
  return to;
 case 2:
  *(short *)to = *(short *)from;
  return to;
 case 4:
  *(int *)to = *(int *)from;
  return to;
 case 3:
  *(short *)to = *(short *)from;
  *((char *)to + 2) = *((char *)from + 2);
  return to;
 case 5:
  *(int *)to = *(int *)from;
  *((char *)to + 4) = *((char *)from + 4);
  return to;
 case 6:
  *(int *)to = *(int *)from;
  *((short *)to + 2) = *((short *)from + 2);
  return to;
 case 8:
  *(int *)to = *(int *)from;
  *((int *)to + 1) = *((int *)from + 1);
  return to;
 }

 esi = (long)from;
 edi = (long)to;
 if (n >= 5 * 4) {

  int ecx;
  asm volatile("rep ; movsl"
        : "=&c" (ecx), "=&D" (edi), "=&S" (esi)
        : "0" (n / 4), "1" (edi), "2" (esi)
        : "memory"
  );
 } else {

  if (n >= 4 * 4)
   asm volatile("movsl"
         : "=&D"(edi), "=&S"(esi)
         : "0"(edi), "1"(esi)
         : "memory");
  if (n >= 3 * 4)
   asm volatile("movsl"
         : "=&D"(edi), "=&S"(esi)
         : "0"(edi), "1"(esi)
         : "memory");
  if (n >= 2 * 4)
   asm volatile("movsl"
         : "=&D"(edi), "=&S"(esi)
         : "0"(edi), "1"(esi)
         : "memory");
  if (n >= 1 * 4)
   asm volatile("movsl"
         : "=&D"(edi), "=&S"(esi)
         : "0"(edi), "1"(esi)
         : "memory");
 }
 switch (n % 4) {

 case 0:
  return to;
 case 1:
  asm volatile("movsb"
        : "=&D"(edi), "=&S"(esi)
        : "0"(edi), "1"(esi)
        : "memory");
  return to;
 case 2:
  asm volatile("movsw"
        : "=&D"(edi), "=&S"(esi)
        : "0"(edi), "1"(esi)
        : "memory");
  return to;
 default:
  asm volatile("movsw\n\tmovsb"
        : "=&D"(edi), "=&S"(esi)
        : "0"(edi), "1"(esi)
        : "memory");
  return to;
 }
}
# 200 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string_32.h"
void *memmove(void *dest, const void *src, size_t n);




extern void *memchr(const void *cs, int c, size_t count);

static inline void *__memset_generic(void *s, char c, size_t count)
{
 int d0, d1;
 asm volatile("rep\n\t"
       "stosb"
       : "=&c" (d0), "=&D" (d1)
       : "a" (c), "1" (s), "0" (count)
       : "memory");
 return s;
}
# 226 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string_32.h"
static inline __attribute__((always_inline))
void *__constant_c_memset(void *s, unsigned long c, size_t count)
{
 int d0, d1;
 asm volatile("rep ; stosl\n\t"
       "testb $2,%b3\n\t"
       "je 1f\n\t"
       "stosw\n"
       "1:\ttestb $1,%b3\n\t"
       "je 2f\n\t"
       "stosb\n"
       "2:"
       : "=&c" (d0), "=&D" (d1)
       : "a" (c), "q" (count), "0" (count/4), "1" ((long)s)
       : "memory");
 return s;
}



extern size_t strnlen(const char *s, size_t count);



extern char *strstr(const char *cs, const char *ct);





static inline __attribute__((always_inline))
void *__constant_c_and_count_memset(void *s, unsigned long pattern,
        size_t count)
{
 switch (count) {
 case 0:
  return s;
 case 1:
  *(unsigned char *)s = pattern & 0xff;
  return s;
 case 2:
  *(unsigned short *)s = pattern & 0xffff;
  return s;
 case 3:
  *(unsigned short *)s = pattern & 0xffff;
  *((unsigned char *)s + 2) = pattern & 0xff;
  return s;
 case 4:
  *(unsigned long *)s = pattern;
  return s;
 }
# 285 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string_32.h"
 {
  int d0, d1;




  unsigned long eax = pattern;


  switch (count % 4) {
  case 0:
   asm volatile("rep ; stosl" "" : "=&c" (d0), "=&D" (d1) : "a" (eax), "0" (count/4), "1" ((long)s) : "memory");
   return s;
  case 1:
   asm volatile("rep ; stosl" "\n\tstosb" : "=&c" (d0), "=&D" (d1) : "a" (eax), "0" (count/4), "1" ((long)s) : "memory");
   return s;
  case 2:
   asm volatile("rep ; stosl" "\n\tstosw" : "=&c" (d0), "=&D" (d1) : "a" (eax), "0" (count/4), "1" ((long)s) : "memory");
   return s;
  default:
   asm volatile("rep ; stosl" "\n\tstosw\n\tstosb" : "=&c" (d0), "=&D" (d1) : "a" (eax), "0" (count/4), "1" ((long)s) : "memory");
   return s;
  }
 }


}
# 338 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string_32.h"
extern void *memscan(void *addr, int c, size_t size);
# 3 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/string.h" 2
# 22 "include/linux/string.h" 2
# 30 "include/linux/string.h"
size_t strlcpy(char *, const char *, size_t);
# 39 "include/linux/string.h"
extern size_t strlcat(char *, const char *, __kernel_size_t);
# 48 "include/linux/string.h"
extern int strnicmp(const char *, const char *, __kernel_size_t);


extern int strcasecmp(const char *s1, const char *s2);


extern int strncasecmp(const char *s1, const char *s2, size_t n);





extern char * strnchr(const char *, size_t, int);


extern char * strrchr(const char *,int);

extern char * skip_spaces(const char *);

extern char *strim(char *);

static inline char *strstrip(char *str)
{
 return strim(str);
}





extern char * strnstr(const char *, const char *, size_t);
# 87 "include/linux/string.h"
extern char * strpbrk(const char *,const char *);


extern char * strsep(char **,const char *);


extern __kernel_size_t strspn(const char *,const char *);


extern __kernel_size_t strcspn(const char *,const char *);
# 112 "include/linux/string.h"
extern int __builtin_memcmp(const void *,const void *,__kernel_size_t);




void *memchr_inv(const void *s, int c, size_t n);

extern char *kstrdup(const char *s, gfp_t gfp);
extern char *kstrndup(const char *s, size_t len, gfp_t gfp);
extern void *kmemdup(const void *src, size_t len, gfp_t gfp);

extern char **argv_split(gfp_t gfp, const char *str, int *argcp);
extern void argv_free(char **argv);

extern bool sysfs_streq(const char *s1, const char *s2);
extern int strtobool(const char *s, bool *res);


int vbin_printf(u32 *bin_buf, size_t size, const char *fmt, va_list args);
int bstr_printf(char *buf, size_t size, const char *fmt, const u32 *bin_buf);
int bprintf(u32 *bin_buf, size_t size, const char *fmt, ...) __attribute__((format(printf,3,4)));


extern ssize_t memory_read_from_buffer(void *to, size_t count, loff_t *ppos,
   const void *from, size_t available);






static inline bool strstarts(const char *str, const char *prefix)
{
 return strncmp(str, prefix, strlen(prefix)) == 0;
}
# 38 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page_32.h" 2

static inline void clear_page(void *page)
{
 __builtin_memset(page, 0, ((1UL) << 12));
}

static inline void copy_page(void *to, void *from)
{
 __builtin_memcpy(to, from, ((1UL) << 12));
}
# 14 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page.h" 2




struct page;

static inline void clear_user_page(void *page, unsigned long vaddr,
       struct page *pg)
{
 clear_page(page);
}

static inline void copy_user_page(void *to, void *from, unsigned long vaddr,
      struct page *topage)
{
 copy_page(to, from);
}
# 60 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page.h"
extern bool __virt_addr_valid(unsigned long kaddr);




# 1 "include/asm-generic/memory_model.h" 1
# 66 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page.h" 2
# 1 "include/asm-generic/getorder.h" 1
# 9 "include/asm-generic/getorder.h"
static inline __attribute__((__const__)) int get_order(unsigned long size)
{
 int order;

 size = (size - 1) >> (12 - 1);
 order = -1;
 do {
  size >>= 1;
  order++;
 } while (size);
 return order;
}
# 67 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/page.h" 2
# 12 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h" 2
# 20 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h"
struct task_struct;
struct exec_domain;
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor-flags.h" 1
# 5 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2


struct task_struct;
struct mm_struct;

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/vm86.h" 1
# 62 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/vm86.h"
struct vm86_regs {



 long ebx;
 long ecx;
 long edx;
 long esi;
 long edi;
 long ebp;
 long eax;
 long __null_ds;
 long __null_es;
 long __null_fs;
 long __null_gs;
 long orig_eax;
 long eip;
 unsigned short cs, __csh;
 long eflags;
 long esp;
 unsigned short ss, __ssh;



 unsigned short es, __esh;
 unsigned short ds, __dsh;
 unsigned short fs, __fsh;
 unsigned short gs, __gsh;
};

struct revectored_struct {
 unsigned long __map[8];
};

struct vm86_struct {
 struct vm86_regs regs;
 unsigned long flags;
 unsigned long screen_bitmap;
 unsigned long cpu_type;
 struct revectored_struct int_revectored;
 struct revectored_struct int21_revectored;
};






struct vm86plus_info_struct {
 unsigned long force_return_for_pic:1;
 unsigned long vm86dbg_active:1;
 unsigned long vm86dbg_TFpendig:1;
 unsigned long unused:28;
 unsigned long is_vm86pus:1;
 unsigned char vm86dbg_intxxtab[32];
};
struct vm86plus_struct {
 struct vm86_regs regs;
 unsigned long flags;
 unsigned long screen_bitmap;
 unsigned long cpu_type;
 struct revectored_struct int_revectored;
 struct revectored_struct int21_revectored;
 struct vm86plus_info_struct vm86plus;
};



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h" 1




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace-abi.h" 1
# 6 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h" 2



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/segment.h" 1
# 214 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/segment.h"
extern const char early_idt_handlers[32][10];
# 10 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h" 2
# 43 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
struct pt_regs {
 unsigned long bx;
 unsigned long cx;
 unsigned long dx;
 unsigned long si;
 unsigned long di;
 unsigned long bp;
 unsigned long ax;
 unsigned long ds;
 unsigned long es;
 unsigned long fs;
 unsigned long gs;
 unsigned long orig_ax;
 unsigned long ip;
 unsigned long cs;
 unsigned long flags;
 unsigned long sp;
 unsigned long ss;
};
# 133 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
# 1 "include/linux/init.h" 1
# 138 "include/linux/init.h"
typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

extern initcall_t __con_initcall_start[], __con_initcall_end[];
extern initcall_t __security_initcall_start[], __security_initcall_end[];


typedef void (*ctor_fn_t)(void);


extern int do_one_initcall(initcall_t fn);
extern char __attribute__ ((__section__(".init.data"))) boot_command_line[];
extern char *saved_command_line;
extern unsigned int reset_devices;


void setup_arch(char **);
void prepare_namespace(void);

extern void (*late_time_init)(void);

extern int initcall_debug;
# 134 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h" 2

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h" 1
# 42 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/desc_defs.h" 1
# 22 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/desc_defs.h"
struct desc_struct {
 union {
  struct {
   unsigned int a;
   unsigned int b;
  };
  struct {
   u16 limit0;
   u16 base0;
   unsigned base1: 8, type: 4, s: 1, dpl: 2, p: 1;
   unsigned limit: 4, avl: 1, l: 1, d: 1, g: 1, base2: 8;
  };
 };
} __attribute__((packed));







enum {
 GATE_INTERRUPT = 0xE,
 GATE_TRAP = 0xF,
 GATE_CALL = 0xC,
 GATE_TASK = 0x5,
};


struct gate_struct64 {
 u16 offset_low;
 u16 segment;
 unsigned ist : 3, zero0 : 5, type : 5, dpl : 2, p : 1;
 u16 offset_middle;
 u32 offset_high;
 u32 zero1;
} __attribute__((packed));





enum {
 DESC_TSS = 0x9,
 DESC_LDT = 0x2,
 DESCTYPE_S = 0x10,
};


struct ldttss_desc64 {
 u16 limit0;
 u16 base0;
 unsigned base1 : 8, type : 5, dpl : 2, p : 1;
 unsigned limit1 : 4, zero0 : 3, g : 1, base2 : 8;
 u32 base3;
 u32 zero1;
} __attribute__((packed));
# 87 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/desc_defs.h"
typedef struct desc_struct gate_desc;
typedef struct desc_struct ldt_desc;
typedef struct desc_struct tss_desc;




struct desc_ptr {
 unsigned short size;
 unsigned long address;
} __attribute__((packed)) ;
# 43 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/kmap_types.h" 1







# 1 "include/asm-generic/kmap_types.h" 1
# 10 "include/asm-generic/kmap_types.h"
enum km_type {
 KM_BOUNCE_READ,
 KM_SKB_SUNRPC_DATA,
 KM_SKB_DATA_SOFTIRQ,
 KM_USER0,
 KM_USER1,
 KM_BIO_SRC_IRQ,
 KM_BIO_DST_IRQ,
 KM_PTE0,
 KM_PTE1,
 KM_IRQ0,
 KM_IRQ1,
 KM_SOFTIRQ0,
 KM_SOFTIRQ1,
 KM_SYNC_ICACHE,
 KM_SYNC_DCACHE,

 KM_UML_USERCOPY,
 KM_IRQ_PTE,
 KM_NMI,
 KM_NMI_PTE,
 KM_KDB,



 KM_TYPE_NR
};
# 9 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/kmap_types.h" 2
# 44 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h" 1
# 177 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_32_types.h" 1
# 10 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_32_types.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable-3level_types.h" 1






typedef u64 pteval_t;
typedef u64 pmdval_t;
typedef u64 pudval_t;
typedef u64 pgdval_t;
typedef u64 pgprotval_t;

typedef union {
 struct {
  unsigned long pte_low, pte_high;
 };
 pteval_t pte;
} pte_t;
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_32_types.h" 2
# 30 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_32_types.h"
extern bool __vmalloc_start_set;
# 178 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h" 2
# 192 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h"
typedef struct pgprot { pgprotval_t pgprot; } pgprot_t;

typedef struct { pgdval_t pgd; } pgd_t;

static inline pgd_t native_make_pgd(pgdval_t val)
{
 return (pgd_t) { val };
}

static inline pgdval_t native_pgd_val(pgd_t pgd)
{
 return pgd.pgd;
}

static inline pgdval_t pgd_flags(pgd_t pgd)
{
 return native_pgd_val(pgd) & (~((pteval_t)(((signed long)(~(((1UL) << 12)-1))) & ((phys_addr_t)((1ULL << 44) - 1)))));
}
# 224 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h"
# 1 "include/asm-generic/pgtable-nopud.h" 1
# 13 "include/asm-generic/pgtable-nopud.h"
typedef struct { pgd_t pgd; } pud_t;
# 25 "include/asm-generic/pgtable-nopud.h"
static inline int pgd_none(pgd_t pgd) { return 0; }
static inline int pgd_bad(pgd_t pgd) { return 0; }
static inline int pgd_present(pgd_t pgd) { return 1; }
static inline void pgd_clear(pgd_t *pgd) { }
# 38 "include/asm-generic/pgtable-nopud.h"
static inline pud_t * pud_offset(pgd_t * pgd, unsigned long address)
{
 return (pud_t *)pgd;
}
# 225 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h" 2

static inline pudval_t native_pud_val(pud_t pud)
{
 return native_pgd_val(pud.pgd);
}



typedef struct { pmdval_t pmd; } pmd_t;

static inline pmd_t native_make_pmd(pmdval_t val)
{
 return (pmd_t) { val };
}

static inline pmdval_t native_pmd_val(pmd_t pmd)
{
 return pmd.pmd;
}
# 253 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h"
static inline pudval_t pud_flags(pud_t pud)
{
 return native_pud_val(pud) & (~((pteval_t)(((signed long)(~(((1UL) << 12)-1))) & ((phys_addr_t)((1ULL << 44) - 1)))));
}

static inline pmdval_t pmd_flags(pmd_t pmd)
{
 return native_pmd_val(pmd) & (~((pteval_t)(((signed long)(~(((1UL) << 12)-1))) & ((phys_addr_t)((1ULL << 44) - 1)))));
}

static inline pte_t native_make_pte(pteval_t val)
{
 return (pte_t) { .pte = val };
}

static inline pteval_t native_pte_val(pte_t pte)
{
 return pte.pte;
}

static inline pteval_t pte_flags(pte_t pte)
{
 return native_pte_val(pte) & (~((pteval_t)(((signed long)(~(((1UL) << 12)-1))) & ((phys_addr_t)((1ULL << 44) - 1)))));
}





typedef struct page *pgtable_t;

extern pteval_t __supported_pte_mask;
extern void set_nx(void);
extern int nx_enabled;


extern pgprot_t pgprot_writecombine(pgprot_t prot);





struct file;
pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
                              unsigned long size, pgprot_t vma_prot);
int phys_mem_access_prot_allowed(struct file *file, unsigned long pfn,
                              unsigned long size, pgprot_t *vma_prot);


void set_pte_vaddr(unsigned long vaddr, pte_t pte);

extern void native_pagetable_reserve(u64 start, u64 end);

extern void native_pagetable_setup_start(pgd_t *base);
extern void native_pagetable_setup_done(pgd_t *base);





struct seq_file;
extern void arch_report_meminfo(struct seq_file *m);

enum {
 PG_LEVEL_NONE,
 PG_LEVEL_4K,
 PG_LEVEL_2M,
 PG_LEVEL_1G,
 PG_LEVEL_NUM
};


extern void update_page_count(int level, unsigned long pages);
# 336 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/pgtable_types.h"
extern pte_t *lookup_address(unsigned long address, unsigned int *level);
# 45 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h" 2

struct page;
struct thread_struct;
struct desc_ptr;
struct tss_struct;
struct mm_struct;
struct desc_struct;
struct task_struct;
struct cpumask;





struct paravirt_callee_save {
 void *func;
};


struct pv_info {
 unsigned int kernel_rpl;
 int shared_kernel_pmd;





 int paravirt_enabled;
 const char *name;
};

struct pv_init_ops {
# 85 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h"
 unsigned (*patch)(u8 type, u16 clobber, void *insnbuf,
     unsigned long addr, unsigned len);
};


struct pv_lazy_ops {

 void (*enter)(void);
 void (*leave)(void);
};

struct pv_time_ops {
 unsigned long long (*sched_clock)(void);
 unsigned long long (*steal_clock)(int cpu);
 unsigned long (*get_tsc_khz)(void);
};

struct pv_cpu_ops {

 unsigned long (*get_debugreg)(int regno);
 void (*set_debugreg)(int regno, unsigned long value);

 void (*clts)(void);

 unsigned long (*read_cr0)(void);
 void (*write_cr0)(unsigned long);

 unsigned long (*read_cr4_safe)(void);
 unsigned long (*read_cr4)(void);
 void (*write_cr4)(unsigned long);







 void (*load_tr_desc)(void);
 void (*load_gdt)(const struct desc_ptr *);
 void (*load_idt)(const struct desc_ptr *);
 void (*store_gdt)(struct desc_ptr *);
 void (*store_idt)(struct desc_ptr *);
 void (*set_ldt)(const void *desc, unsigned entries);

 void (*load_user_cs_desc)(int cpu, struct mm_struct *mm);

 unsigned long (*store_tr)(void);
 void (*load_tls)(struct thread_struct *t, unsigned int cpu);



 void (*write_ldt_entry)(struct desc_struct *ldt, int entrynum,
    const void *desc);
 void (*write_gdt_entry)(struct desc_struct *,
    int entrynum, const void *desc, int size);
 void (*write_idt_entry)(gate_desc *,
    int entrynum, const gate_desc *gate);
 void (*alloc_ldt)(struct desc_struct *ldt, unsigned entries);
 void (*free_ldt)(struct desc_struct *ldt, unsigned entries);

 void (*load_sp0)(struct tss_struct *tss, struct thread_struct *t);

 void (*set_iopl_mask)(unsigned mask);

 void (*wbinvd)(void);
 void (*io_delay)(void);


 void (*cpuid)(unsigned int *eax, unsigned int *ebx,
        unsigned int *ecx, unsigned int *edx);



 u64 (*read_msr)(unsigned int msr, int *err);
 int (*rdmsr_regs)(u32 *regs);
 int (*write_msr)(unsigned int msr, unsigned low, unsigned high);
 int (*wrmsr_regs)(u32 *regs);

 u64 (*read_tsc)(void);
 u64 (*read_pmc)(int counter);
 unsigned long long (*read_tscp)(unsigned int *aux);







 void (*irq_enable_sysexit)(void);







 void (*usergs_sysret64)(void);







 void (*usergs_sysret32)(void);



 void (*iret)(void);

 void (*swapgs)(void);

 void (*start_context_switch)(struct task_struct *prev);
 void (*end_context_switch)(struct task_struct *next);
};

struct pv_irq_ops {
# 211 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h"
 struct paravirt_callee_save save_fl;
 struct paravirt_callee_save restore_fl;
 struct paravirt_callee_save irq_disable;
 struct paravirt_callee_save irq_enable;

 void (*safe_halt)(void);
 void (*halt)(void);




};

struct pv_apic_ops {

 void (*startup_ipi_hook)(int phys_apicid,
     unsigned long start_eip,
     unsigned long start_esp);

};

struct pv_mmu_ops {
 unsigned long (*read_cr2)(void);
 void (*write_cr2)(unsigned long);

 unsigned long (*read_cr3)(void);
 void (*write_cr3)(unsigned long);





 void (*activate_mm)(struct mm_struct *prev,
       struct mm_struct *next);
 void (*dup_mmap)(struct mm_struct *oldmm,
    struct mm_struct *mm);
 void (*exit_mmap)(struct mm_struct *mm);



 void (*flush_tlb_user)(void);
 void (*flush_tlb_kernel)(void);
 void (*flush_tlb_single)(unsigned long addr);
 void (*flush_tlb_others)(const struct cpumask *cpus,
     struct mm_struct *mm,
     unsigned long va);


 int (*pgd_alloc)(struct mm_struct *mm);
 void (*pgd_free)(struct mm_struct *mm, pgd_t *pgd);





 void (*alloc_pte)(struct mm_struct *mm, unsigned long pfn);
 void (*alloc_pmd)(struct mm_struct *mm, unsigned long pfn);
 void (*alloc_pud)(struct mm_struct *mm, unsigned long pfn);
 void (*release_pte)(unsigned long pfn);
 void (*release_pmd)(unsigned long pfn);
 void (*release_pud)(unsigned long pfn);


 void (*set_pte)(pte_t *ptep, pte_t pteval);
 void (*set_pte_at)(struct mm_struct *mm, unsigned long addr,
      pte_t *ptep, pte_t pteval);
 void (*set_pmd)(pmd_t *pmdp, pmd_t pmdval);
 void (*set_pmd_at)(struct mm_struct *mm, unsigned long addr,
      pmd_t *pmdp, pmd_t pmdval);
 void (*pte_update)(struct mm_struct *mm, unsigned long addr,
      pte_t *ptep);
 void (*pte_update_defer)(struct mm_struct *mm,
     unsigned long addr, pte_t *ptep);
 void (*pmd_update)(struct mm_struct *mm, unsigned long addr,
      pmd_t *pmdp);
 void (*pmd_update_defer)(struct mm_struct *mm,
     unsigned long addr, pmd_t *pmdp);

 pte_t (*ptep_modify_prot_start)(struct mm_struct *mm, unsigned long addr,
     pte_t *ptep);
 void (*ptep_modify_prot_commit)(struct mm_struct *mm, unsigned long addr,
     pte_t *ptep, pte_t pte);

 struct paravirt_callee_save pte_val;
 struct paravirt_callee_save make_pte;

 struct paravirt_callee_save pgd_val;
 struct paravirt_callee_save make_pgd;



 void (*set_pte_atomic)(pte_t *ptep, pte_t pteval);
 void (*pte_clear)(struct mm_struct *mm, unsigned long addr,
     pte_t *ptep);
 void (*pmd_clear)(pmd_t *pmdp);



 void (*set_pud)(pud_t *pudp, pud_t pudval);

 struct paravirt_callee_save pmd_val;
 struct paravirt_callee_save make_pmd;
# 322 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h"
 struct pv_lazy_ops lazy_mode;





 void (*set_fixmap)(unsigned idx,
      phys_addr_t phys, pgprot_t flags);
};

struct arch_spinlock;
struct pv_lock_ops {
 int (*spin_is_locked)(struct arch_spinlock *lock);
 int (*spin_is_contended)(struct arch_spinlock *lock);
 void (*spin_lock)(struct arch_spinlock *lock);
 void (*spin_lock_flags)(struct arch_spinlock *lock, unsigned long flags);
 int (*spin_trylock)(struct arch_spinlock *lock);
 void (*spin_unlock)(struct arch_spinlock *lock);
};




struct paravirt_patch_template {
 struct pv_init_ops pv_init_ops;
 struct pv_time_ops pv_time_ops;
 struct pv_cpu_ops pv_cpu_ops;
 struct pv_irq_ops pv_irq_ops;
 struct pv_apic_ops pv_apic_ops;
 struct pv_mmu_ops pv_mmu_ops;
 struct pv_lock_ops pv_lock_ops;
};

extern struct pv_info pv_info;
extern struct pv_init_ops pv_init_ops;
extern struct pv_time_ops pv_time_ops;
extern struct pv_cpu_ops pv_cpu_ops;
extern struct pv_irq_ops pv_irq_ops;
extern struct pv_apic_ops pv_apic_ops;
extern struct pv_mmu_ops pv_mmu_ops;
extern struct pv_lock_ops pv_lock_ops;
# 396 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h"
unsigned paravirt_patch_nop(void);
unsigned paravirt_patch_ident_32(void *insnbuf, unsigned len);
unsigned paravirt_patch_ident_64(void *insnbuf, unsigned len);
unsigned paravirt_patch_ignore(unsigned len);
unsigned paravirt_patch_call(void *insnbuf,
        const void *target, u16 tgt_clobbers,
        unsigned long addr, u16 site_clobbers,
        unsigned len);
unsigned paravirt_patch_jmp(void *insnbuf, const void *target,
       unsigned long addr, unsigned len);
unsigned paravirt_patch_default(u8 type, u16 clobbers, void *insnbuf,
    unsigned long addr, unsigned len);

unsigned paravirt_patch_insns(void *insnbuf, unsigned len,
         const char *start, const char *end);

unsigned native_patch(u8 type, u16 clobbers, void *ibuf,
        unsigned long addr, unsigned len);

int paravirt_disable_iospace(void);
# 674 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt_types.h"
enum paravirt_lazy_mode {
 PARAVIRT_LAZY_NONE,
 PARAVIRT_LAZY_MMU,
 PARAVIRT_LAZY_CPU,
};

enum paravirt_lazy_mode paravirt_get_lazy_mode(void);
void paravirt_start_context_switch(struct task_struct *prev);
void paravirt_end_context_switch(struct task_struct *next);

void paravirt_enter_lazy_mmu(void);
void paravirt_leave_lazy_mmu(void);

void _paravirt_nop(void);
u32 _paravirt_ident_32(u32);
u64 _paravirt_ident_64(u64);




struct paravirt_patch_site {
 u8 *instr;
 u8 instrtype;
 u8 len;
 u16 clobbers;
};

extern struct paravirt_patch_site __parainstructions[],
 __parainstructions_end[];
# 136 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h" 2


struct cpuinfo_x86;
struct task_struct;

extern unsigned long profile_pc(struct pt_regs *regs);


extern unsigned long
convert_ip_to_linear(struct task_struct *child, struct pt_regs *regs);
extern void send_sigtrap(struct task_struct *tsk, struct pt_regs *regs,
    int error_code, int si_code);
void signal_fault(struct pt_regs *regs, void *frame, char *where);

extern long syscall_trace_enter(struct pt_regs *);
extern void syscall_trace_leave(struct pt_regs *);

static inline unsigned long regs_return_value(struct pt_regs *regs)
{
 return regs->ax;
}
# 165 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
static inline int user_mode(struct pt_regs *regs)
{

 return (regs->cs & 0x3) == 0x3;



}

static inline int user_mode_vm(struct pt_regs *regs)
{

 return ((regs->cs & 0x3) | (regs->flags & 0x00020000)) >=
  0x3;



}

static inline int v8086_mode(struct pt_regs *regs)
{

 return (regs->flags & 0x00020000);



}
# 216 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
static inline unsigned long kernel_stack_pointer(struct pt_regs *regs)
{

 return (unsigned long)(&regs->sp);



}





# 1 "include/asm-generic/ptrace.h" 1
# 22 "include/asm-generic/ptrace.h"
static inline unsigned long instruction_pointer(struct pt_regs *regs)
{
 return ((regs)->ip);
}
static inline void instruction_pointer_set(struct pt_regs *regs,
                                           unsigned long val)
{
 (((regs)->ip) = (val));
}
# 44 "include/asm-generic/ptrace.h"
static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
 return ((regs)->sp);
}
static inline void user_stack_pointer_set(struct pt_regs *regs,
                                          unsigned long val)
{
 (((regs)->sp) = (val));
}
# 62 "include/asm-generic/ptrace.h"
static inline unsigned long frame_pointer(struct pt_regs *regs)
{
 return ((regs)->bp);
}
static inline void frame_pointer_set(struct pt_regs *regs,
                                     unsigned long val)
{
 (((regs)->bp) = (val));
}
# 230 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h" 2


extern int regs_query_register_offset(const char *name);
extern const char *regs_query_register_name(unsigned int offset);
# 245 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
static inline unsigned long regs_get_register(struct pt_regs *regs,
           unsigned int offset)
{
 if (__builtin_expect(!!(offset > (__builtin_offsetof(struct pt_regs,ss))), 0))
  return 0;
 return *(unsigned long *)((unsigned long)regs + offset);
}
# 261 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
static inline int regs_within_kernel_stack(struct pt_regs *regs,
        unsigned long addr)
{
 return ((addr & ~((((1UL) << 12) << 1) - 1)) ==
  (kernel_stack_pointer(regs) & ~((((1UL) << 12) << 1) - 1)));
}
# 277 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
static inline unsigned long regs_get_kernel_stack_nth(struct pt_regs *regs,
            unsigned int n)
{
 unsigned long *addr = (unsigned long *)kernel_stack_pointer(regs);
 addr += n;
 if (regs_within_kernel_stack(regs, (unsigned long)addr))
  return *addr;
 else
  return 0;
}
# 297 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ptrace.h"
struct user_desc;
extern int do_get_thread_area(struct task_struct *p, int idx,
         struct user_desc *info);
extern int do_set_thread_area(struct task_struct *p, int idx,
         struct user_desc *info, int can_allocate);
# 131 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/vm86.h" 2
# 141 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/vm86.h"
struct kernel_vm86_regs {



 struct pt_regs pt;



 unsigned short es, __esh;
 unsigned short ds, __dsh;
 unsigned short fs, __fsh;
 unsigned short gs, __gsh;
};

struct kernel_vm86_struct {
 struct kernel_vm86_regs regs;
# 166 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/vm86.h"
 unsigned long flags;
 unsigned long screen_bitmap;
 unsigned long cpu_type;
 struct revectored_struct int_revectored;
 struct revectored_struct int21_revectored;
 struct vm86plus_info_struct vm86plus;
 struct pt_regs *regs32;
# 183 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/vm86.h"
};



void handle_vm86_fault(struct kernel_vm86_regs *, long);
int handle_vm86_trap(struct kernel_vm86_regs *, long, int);
struct pt_regs *save_v86_state(struct kernel_vm86_regs *);

struct task_struct;
void release_vm86_irqs(struct task_struct *);
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/math_emu.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/math_emu.h"
struct math_emu_info {
 long ___orig_eip;
 union {
  struct pt_regs *regs;
  struct kernel_vm86_regs *vm86;
 };
};
# 12 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2


# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/sigcontext.h" 1
# 23 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/sigcontext.h"
struct _fpx_sw_bytes {
 __u32 magic1;
 __u32 extended_size;


 __u64 xstate_bv;




 __u32 xstate_size;




 __u32 padding[7];
};
# 56 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/sigcontext.h"
struct _fpreg {
 unsigned short significand[4];
 unsigned short exponent;
};

struct _fpxreg {
 unsigned short significand[4];
 unsigned short exponent;
 unsigned short padding[3];
};

struct _xmmreg {
 unsigned long element[4];
};

struct _fpstate {

 unsigned long cw;
 unsigned long sw;
 unsigned long tag;
 unsigned long ipoff;
 unsigned long cssel;
 unsigned long dataoff;
 unsigned long datasel;
 struct _fpreg _st[8];
 unsigned short status;
 unsigned short magic;


 unsigned long _fxsr_env[6];
 unsigned long mxcsr;
 unsigned long reserved;
 struct _fpxreg _fxsr_st[8];
 struct _xmmreg _xmm[8];
 unsigned long padding1[44];

 union {
  unsigned long padding2[12];
  struct _fpx_sw_bytes sw_reserved;

 };
};




struct sigcontext {
 unsigned short gs, __gsh;
 unsigned short fs, __fsh;
 unsigned short es, __esh;
 unsigned short ds, __dsh;
 unsigned long di;
 unsigned long si;
 unsigned long bp;
 unsigned long sp;
 unsigned long bx;
 unsigned long dx;
 unsigned long cx;
 unsigned long ax;
 unsigned long trapno;
 unsigned long err;
 unsigned long ip;
 unsigned short cs, __csh;
 unsigned long flags;
 unsigned long sp_at_signal;
 unsigned short ss, __ssh;
# 130 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/sigcontext.h"
 void *fpstate;
 unsigned long oldmask;
 unsigned long cr2;
};
# 266 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/sigcontext.h"
struct _xsave_hdr {
 __u64 xstate_bv;
 __u64 reserved1[2];
 __u64 reserved2[5];
};

struct _ymmh_state {

 __u32 ymmh_space[64];
};







struct _xstate {
 struct _fpstate fpstate;
 struct _xsave_hdr xstate_hdr;
 struct _ymmh_state ymmh;

};
# 15 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/current.h" 1




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/percpu.h" 1
# 44 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/percpu.h"
# 1 "include/linux/kernel.h" 1
# 13 "include/linux/kernel.h"
# 1 "include/linux/linkage.h" 1




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/linkage.h" 1
# 6 "include/linux/linkage.h" 2
# 14 "include/linux/kernel.h" 2




# 1 "include/linux/log2.h" 1
# 21 "include/linux/log2.h"
extern __attribute__((const, noreturn))
int ____ilog2_NaN(void);
# 31 "include/linux/log2.h"
static inline __attribute__((const))
int __ilog2_u32(u32 n)
{
 return fls(n) - 1;
}



static inline __attribute__((const))
int __ilog2_u64(u64 n)
{
 return fls64(n) - 1;
}







static inline __attribute__((const))
bool is_power_of_2(unsigned long n)
{
 return (n != 0 && ((n & (n - 1)) == 0));
}




static inline __attribute__((const))
unsigned long __roundup_pow_of_two(unsigned long n)
{
 return 1UL << fls_long(n - 1);
}




static inline __attribute__((const))
unsigned long __rounddown_pow_of_two(unsigned long n)
{
 return 1UL << (fls_long(n) - 1);
}
# 19 "include/linux/kernel.h" 2

# 1 "include/linux/printk.h" 1





extern const char linux_banner[];
extern const char linux_proc_banner[];
# 27 "include/linux/printk.h"
extern int console_printk[];






static inline void console_silent(void)
{
 (console_printk[0]) = 0;
}

static inline void console_verbose(void)
{
 if ((console_printk[0]))
  (console_printk[0]) = 15;
}

struct va_format {
 const char *fmt;
 va_list *va;
};
# 85 "include/linux/printk.h"
static inline __attribute__((format(printf,1,2)))
int no_printk(const char *fmt, ...)
{
 return 0;
}

extern __attribute__((regparm(0))) __attribute__((format(printf,1,2)))
void early_printk(const char *fmt, ...);

extern int printk_needs_cpu(int cpu);
extern void printk_tick(void);


 __attribute__((regparm(0))) __attribute__((format(printf,1,0)))
int vprintk(const char *fmt, va_list args);
 __attribute__((regparm(0))) __attribute__((format(printf,1,2))) __attribute__((__cold__))
int printk(const char *fmt, ...);






extern int __printk_ratelimit(const char *func);

extern bool printk_timed_ratelimit(unsigned long *caller_jiffies,
       unsigned int interval_msec);

extern int printk_delay_msec;
extern int dmesg_restrict;
extern int kptr_restrict;

void log_buf_kexec_setup(void);
void __attribute__ ((__section__(".init.text"))) __attribute__((__cold__)) __attribute__((no_instrument_function)) setup_log_buf(int early);
# 149 "include/linux/printk.h"
extern void dump_stack(void) __attribute__((__cold__));
# 282 "include/linux/printk.h"
enum {
 DUMP_PREFIX_NONE,
 DUMP_PREFIX_ADDRESS,
 DUMP_PREFIX_OFFSET
};
extern void hex_dump_to_buffer(const void *buf, size_t len,
          int rowsize, int groupsize,
          char *linebuf, size_t linebuflen, bool ascii);

extern void print_hex_dump(const char *level, const char *prefix_str,
      int prefix_type, int rowsize, int groupsize,
      const void *buf, size_t len, bool ascii);
extern void print_hex_dump_bytes(const char *prefix_str, int prefix_type,
     const void *buf, size_t len);
# 21 "include/linux/kernel.h" 2
# 1 "include/linux/dynamic_debug.h" 1
# 9 "include/linux/dynamic_debug.h"
struct _ddebug {




 const char *modname;
 const char *function;
 const char *filename;
 const char *format;
 unsigned int lineno:24;
# 30 "include/linux/dynamic_debug.h"
 unsigned int flags:8;
 char enabled;
} __attribute__((aligned(8)));


int ddebug_add_module(struct _ddebug *tab, unsigned int n,
    const char *modname);
# 94 "include/linux/dynamic_debug.h"
static inline int ddebug_remove_module(const char *mod)
{
 return 0;
}
# 22 "include/linux/kernel.h" 2

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bug.h" 1
# 38 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bug.h"
# 1 "include/asm-generic/bug.h" 1
# 10 "include/asm-generic/bug.h"
struct bug_entry {

 unsigned long bug_addr;





 const char *file;



 unsigned short line;

 unsigned short flags;
};
# 64 "include/asm-generic/bug.h"
extern __attribute__((format(printf,3,4)))
void warn_slowpath_fmt(const char *file, const int line,
         const char *fmt, ...);
extern __attribute__((format(printf,4,5)))
void warn_slowpath_fmt_taint(const char *file, const int line, unsigned taint,
        const char *fmt, ...);
extern void warn_slowpath_null(const char *file, const int line);
# 39 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/bug.h" 2
# 24 "include/linux/kernel.h" 2
# 105 "include/linux/kernel.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/div64.h" 1
# 36 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/div64.h"
static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
 union {
  u64 v64;
  u32 v32[2];
 } d = { dividend };
 u32 upper;

 upper = d.v32[1];
 d.v32[1] = 0;
 if (upper >= divisor) {
  d.v32[1] = upper / divisor;
  upper %= divisor;
 }
 asm ("divl %2" : "=a" (d.v32[0]), "=d" (*remainder) :
  "rm" (divisor), "0" (d.v32[0]), "1" (upper));
 return d.v64;
}
# 106 "include/linux/kernel.h" 2
# 134 "include/linux/kernel.h"
struct completion;
struct pt_regs;
struct user;


extern int _cond_resched(void);
# 160 "include/linux/kernel.h"
  static inline void __might_sleep(const char *file, int line,
       int preempt_offset) { }
# 193 "include/linux/kernel.h"
static inline void might_fault(void)
{
 do { _cond_resched(); } while (0);
}


extern struct atomic_notifier_head panic_notifier_list;
extern long (*panic_blink)(int state);
 void panic(const char * fmt, ...)
 __attribute__ ((noreturn, format (printf, 1, 2))) __attribute__((__cold__));
extern void oops_enter(void);
extern void oops_exit(void);
void print_oops_end_marker(void);
extern int oops_may_print(void);
 void do_exit(long error_code)
 __attribute__((noreturn));
 void complete_and_exit(struct completion *, long)
 __attribute__((noreturn));


int _kstrtoul(const char *s, unsigned int base, unsigned long *res);
int _kstrtol(const char *s, unsigned int base, long *res);

int kstrtoull(const char *s, unsigned int base, unsigned long long *res);
int kstrtoll(const char *s, unsigned int base, long long *res);
static inline int kstrtoul(const char *s, unsigned int base, unsigned long *res)
{




 if (sizeof(unsigned long) == sizeof(unsigned long long) &&
     __alignof__(unsigned long) == __alignof__(unsigned long long))
  return kstrtoull(s, base, (unsigned long long *)res);
 else
  return _kstrtoul(s, base, res);
}

static inline int kstrtol(const char *s, unsigned int base, long *res)
{




 if (sizeof(long) == sizeof(long long) &&
     __alignof__(long) == __alignof__(long long))
  return kstrtoll(s, base, (long long *)res);
 else
  return _kstrtol(s, base, res);
}

int kstrtouint(const char *s, unsigned int base, unsigned int *res);
int kstrtoint(const char *s, unsigned int base, int *res);

static inline int kstrtou64(const char *s, unsigned int base, u64 *res)
{
 return kstrtoull(s, base, res);
}

static inline int kstrtos64(const char *s, unsigned int base, s64 *res)
{
 return kstrtoll(s, base, res);
}

static inline int kstrtou32(const char *s, unsigned int base, u32 *res)
{
 return kstrtouint(s, base, res);
}

static inline int kstrtos32(const char *s, unsigned int base, s32 *res)
{
 return kstrtoint(s, base, res);
}

int kstrtou16(const char *s, unsigned int base, u16 *res);
int kstrtos16(const char *s, unsigned int base, s16 *res);
int kstrtou8(const char *s, unsigned int base, u8 *res);
int kstrtos8(const char *s, unsigned int base, s8 *res);

int kstrtoull_from_user(const char *s, size_t count, unsigned int base, unsigned long long *res);
int kstrtoll_from_user(const char *s, size_t count, unsigned int base, long long *res);
int kstrtoul_from_user(const char *s, size_t count, unsigned int base, unsigned long *res);
int kstrtol_from_user(const char *s, size_t count, unsigned int base, long *res);
int kstrtouint_from_user(const char *s, size_t count, unsigned int base, unsigned int *res);
int kstrtoint_from_user(const char *s, size_t count, unsigned int base, int *res);
int kstrtou16_from_user(const char *s, size_t count, unsigned int base, u16 *res);
int kstrtos16_from_user(const char *s, size_t count, unsigned int base, s16 *res);
int kstrtou8_from_user(const char *s, size_t count, unsigned int base, u8 *res);
int kstrtos8_from_user(const char *s, size_t count, unsigned int base, s8 *res);

static inline int kstrtou64_from_user(const char *s, size_t count, unsigned int base, u64 *res)
{
 return kstrtoull_from_user(s, count, base, res);
}

static inline int kstrtos64_from_user(const char *s, size_t count, unsigned int base, s64 *res)
{
 return kstrtoll_from_user(s, count, base, res);
}

static inline int kstrtou32_from_user(const char *s, size_t count, unsigned int base, u32 *res)
{
 return kstrtouint_from_user(s, count, base, res);
}

static inline int kstrtos32_from_user(const char *s, size_t count, unsigned int base, s32 *res)
{
 return kstrtoint_from_user(s, count, base, res);
}



extern unsigned long simple_strtoul(const char *,char **,unsigned int);
extern long simple_strtol(const char *,char **,unsigned int);
extern unsigned long long simple_strtoull(const char *,char **,unsigned int);
extern long long simple_strtoll(const char *,char **,unsigned int);







extern __attribute__((format(printf,2,3))) int sprintf(char *buf, const char * fmt, ...);
extern __attribute__((format(printf,2,0))) int vsprintf(char *buf, const char *, va_list);
extern __attribute__((format(printf,3,4)))
int snprintf(char *buf, size_t size, const char *fmt, ...);
extern __attribute__((format(printf,3,0)))
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern __attribute__((format(printf,3,4)))
int scnprintf(char *buf, size_t size, const char *fmt, ...);
extern __attribute__((format(printf,3,0)))
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern __attribute__((format(printf,2,3)))
char *kasprintf(gfp_t gfp, const char *fmt, ...);
extern char *kvasprintf(gfp_t gfp, const char *fmt, va_list args);

extern int sscanf(const char *, const char *, ...)
 __attribute__ ((format (scanf, 2, 3)));
extern int vsscanf(const char *, const char *, va_list)
 __attribute__ ((format (scanf, 2, 0)));

extern int get_option(char **str, int *pint);
extern char *get_options(const char *str, int nints, int *ints);
extern unsigned long long memparse(const char *ptr, char **retptr);

extern int core_kernel_text(unsigned long addr);
extern int core_kernel_data(unsigned long addr);
extern int __kernel_text_address(unsigned long addr);
extern int kernel_text_address(unsigned long addr);
extern int func_ptr_is_kernel_text(void *ptr);

struct pid;
extern struct pid *session_of_pgrp(struct pid *pgrp);

unsigned long int_sqrt(unsigned long);

extern void bust_spinlocks(int yes);
extern void wake_up_klogd(void);
extern int oops_in_progress;
extern int panic_timeout;
extern int panic_on_oops;
extern int panic_on_unrecovered_nmi;
extern int panic_on_io_nmi;
extern const char *print_tainted(void);
extern void add_taint(unsigned flag);
extern int test_taint(unsigned flag);
extern unsigned long get_taint(void);
extern int root_mountflags;

extern bool early_boot_irqs_disabled;


extern enum system_states {
 SYSTEM_BOOTING,
 SYSTEM_RUNNING,
 SYSTEM_HALT,
 SYSTEM_POWER_OFF,
 SYSTEM_RESTART,
 SYSTEM_SUSPEND_DISK,
} system_state;
# 389 "include/linux/kernel.h"
extern const char hex_asc[];



static inline char *hex_byte_pack(char *buf, u8 byte)
{
 *buf++ = hex_asc[((byte) & 0xf0) >> 4];
 *buf++ = hex_asc[((byte) & 0x0f)];
 return buf;
}

static inline char * pack_hex_byte(char *buf, u8 byte)
{
 return hex_byte_pack(buf, byte);
}

extern int hex_to_bin(char ch);
extern int hex2bin(u8 *dst, const char *src, size_t count);
# 428 "include/linux/kernel.h"
void tracing_on(void);
void tracing_off(void);

void tracing_off_permanent(void);
int tracing_is_on(void);







enum ftrace_dump_mode {
 DUMP_NONE,
 DUMP_ALL,
 DUMP_ORIG,
};


extern void tracing_start(void);
extern void tracing_stop(void);
extern void ftrace_off_permanent(void);

static inline __attribute__((format(printf,1,2)))
void ____trace_printk_check_format(const char *fmt, ...)
{
}
# 491 "include/linux/kernel.h"
extern __attribute__((format(printf,2,3)))
int __trace_bprintk(unsigned long ip, const char *fmt, ...);

extern __attribute__((format(printf,2,3)))
int __trace_printk(unsigned long ip, const char *fmt, ...);

extern void trace_dump_stack(void);
# 516 "include/linux/kernel.h"
extern int
__ftrace_vbprintk(unsigned long ip, const char *fmt, va_list ap);

extern int
__ftrace_vprintk(unsigned long ip, const char *fmt, va_list ap);

extern void ftrace_dump(enum ftrace_dump_mode oops_dump_mode);
# 712 "include/linux/kernel.h"
extern int __build_bug_on_failed;
# 743 "include/linux/kernel.h"
struct sysinfo;
extern int do_sysinfo(struct sysinfo *info);




struct sysinfo {
 long uptime;
 unsigned long loads[3];
 unsigned long totalram;
 unsigned long freeram;
 unsigned long sharedram;
 unsigned long bufferram;
 unsigned long totalswap;
 unsigned long freeswap;
 unsigned short procs;
 unsigned short pad;
 unsigned long totalhigh;
 unsigned long freehigh;
 unsigned int mem_unit;
 char _f[20-2*sizeof(long)-sizeof(int)];
};
# 45 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/percpu.h" 2
# 88 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/percpu.h"
extern void __bad_percpu_size(void);
# 549 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/percpu.h"
static inline __attribute__((always_inline)) int x86_this_cpu_constant_test_bit(unsigned int nr,
                        const unsigned long *addr)
{
 unsigned long *a = (unsigned long *)addr + nr / 32;

 return ((1UL << (nr % 32)) & ({ typeof(*a) pfo_ret__; switch (sizeof(*a)) { case 1: asm("mov" "b ""%%""fs"":" "%P" "1"",%0" : "=q" (pfo_ret__) : "m" (*a)); break; case 2: asm("mov" "w ""%%""fs"":" "%P" "1"",%0" : "=r" (pfo_ret__) : "m" (*a)); break; case 4: asm("mov" "l ""%%""fs"":" "%P" "1"",%0" : "=r" (pfo_ret__) : "m" (*a)); break; case 8: asm("mov" "q ""%%""fs"":" "%P" "1"",%0" : "=r" (pfo_ret__) : "m" (*a)); break; default: __bad_percpu_size(); } pfo_ret__; })) != 0;
}

static inline int x86_this_cpu_variable_test_bit(int nr,
                        const unsigned long *addr)
{
 int oldbit;

 asm volatile("bt ""%%""fs"":" "%P" "2"",%1\n\t"
   "sbb %0,%0"
   : "=r" (oldbit)
   : "m" (*(unsigned long *)addr), "Ir" (nr));

 return oldbit;
}







# 1 "include/asm-generic/percpu.h" 1




# 1 "include/linux/threads.h" 1
# 6 "include/asm-generic/percpu.h" 2
# 1 "include/linux/percpu-defs.h" 1
# 7 "include/asm-generic/percpu.h" 2
# 18 "include/asm-generic/percpu.h"
extern unsigned long __per_cpu_offset[8];
# 72 "include/asm-generic/percpu.h"
extern void setup_per_cpu_areas(void);
# 577 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/percpu.h" 2


extern __attribute__((section(".data..percpu" ""))) __typeof__(unsigned long) this_cpu_off;
# 6 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/current.h" 2


struct task_struct;

extern __attribute__((section(".data..percpu" ""))) __typeof__(struct task_struct *) current_task;

static inline __attribute__((always_inline)) struct task_struct *get_current(void)
{
 return ({ typeof(current_task) pfo_ret__; switch (sizeof(current_task)) { case 1: asm("mov" "b ""%%""fs"":" "%P" "1"",%0" : "=q" (pfo_ret__) : "p" (&(current_task))); break; case 2: asm("mov" "w ""%%""fs"":" "%P" "1"",%0" : "=r" (pfo_ret__) : "p" (&(current_task))); break; case 4: asm("mov" "l ""%%""fs"":" "%P" "1"",%0" : "=r" (pfo_ret__) : "p" (&(current_task))); break; case 8: asm("mov" "q ""%%""fs"":" "%P" "1"",%0" : "=r" (pfo_ret__) : "p" (&(current_task))); break; default: __bad_percpu_size(); } pfo_ret__; });
}
# 16 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h" 1






# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg.h"
extern void __xchg_wrong_size(void)
 __attribute__((error("Bad argument size for xchg")));
extern void __cmpxchg_wrong_size(void)
 __attribute__((error("Bad argument size for cmpxchg")));
extern void __xadd_wrong_size(void)
 __attribute__((error("Bad argument size for xadd")));
# 152 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg_32.h" 1
# 23 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg_32.h"
static inline void set_64bit(volatile u64 *ptr, u64 value)
{
 u32 low = value;
 u32 high = value >> 32;
 u64 prev = *ptr;

 asm volatile("\n1:\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchg8b %0\n\t"
       "jnz 1b"
       : "=m" (*ptr), "+A" (prev)
       : "b" (low), "c" (high)
       : "memory");
}
# 50 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg_32.h"
static inline u64 __cmpxchg64(volatile u64 *ptr, u64 old, u64 new)
{
 u64 prev;
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchg8b %1"
       : "=A" (prev),
         "+m" (*ptr)
       : "b" ((u32)new),
         "c" ((u32)(new >> 32)),
         "0" (old)
       : "memory");
 return prev;
}

static inline u64 __cmpxchg64_local(volatile u64 *ptr, u64 old, u64 new)
{
 u64 prev;
 asm volatile("cmpxchg8b %1"
       : "=A" (prev),
         "+m" (*ptr)
       : "b" ((u32)new),
         "c" ((u32)(new >> 32)),
         "0" (old)
       : "memory");
 return prev;
}
# 153 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cmpxchg.h" 2
# 8 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/nops.h" 1
# 138 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/nops.h"
extern const unsigned char * const *ideal_nops;
extern void arch_init_ideal_nops(void);
# 9 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h" 2


# 1 "include/linux/irqflags.h" 1
# 15 "include/linux/irqflags.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/irqflags.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/irqflags.h"
static inline unsigned long native_save_fl(void)
{
 unsigned long flags;






 asm volatile("# __raw_save_flags\n\t"
       "pushf ; pop %0"
       : "=rm" (flags)
       :
       : "memory");

 return flags;
}

static inline void native_restore_fl(unsigned long flags)
{
 asm volatile("push %0 ; popf"
       :
       :"g" (flags)
       :"memory", "cc");
}

static inline void native_irq_disable(void)
{
 asm volatile("cli": : :"memory");
}

static inline void native_irq_enable(void)
{
 asm volatile("sti": : :"memory");
}

static inline void native_safe_halt(void)
{
 asm volatile("sti; hlt": : :"memory");
}

static inline void native_halt(void)
{
 asm volatile("hlt": : :"memory");
}




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h" 1
# 14 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
# 1 "include/linux/cpumask.h" 1
# 11 "include/linux/cpumask.h"
# 1 "include/linux/bitmap.h" 1
# 91 "include/linux/bitmap.h"
extern int __bitmap_empty(const unsigned long *bitmap, int bits);
extern int __bitmap_full(const unsigned long *bitmap, int bits);
extern int __bitmap_equal(const unsigned long *bitmap1,
                 const unsigned long *bitmap2, int bits);
extern void __bitmap_complement(unsigned long *dst, const unsigned long *src,
   int bits);
extern void __bitmap_shift_right(unsigned long *dst,
                        const unsigned long *src, int shift, int bits);
extern void __bitmap_shift_left(unsigned long *dst,
                        const unsigned long *src, int shift, int bits);
extern int __bitmap_and(unsigned long *dst, const unsigned long *bitmap1,
   const unsigned long *bitmap2, int bits);
extern void __bitmap_or(unsigned long *dst, const unsigned long *bitmap1,
   const unsigned long *bitmap2, int bits);
extern void __bitmap_xor(unsigned long *dst, const unsigned long *bitmap1,
   const unsigned long *bitmap2, int bits);
extern int __bitmap_andnot(unsigned long *dst, const unsigned long *bitmap1,
   const unsigned long *bitmap2, int bits);
extern int __bitmap_intersects(const unsigned long *bitmap1,
   const unsigned long *bitmap2, int bits);
extern int __bitmap_subset(const unsigned long *bitmap1,
   const unsigned long *bitmap2, int bits);
extern int __bitmap_weight(const unsigned long *bitmap, int bits);

extern void bitmap_set(unsigned long *map, int i, int len);
extern void bitmap_clear(unsigned long *map, int start, int nr);
extern unsigned long bitmap_find_next_zero_area(unsigned long *map,
      unsigned long size,
      unsigned long start,
      unsigned int nr,
      unsigned long align_mask);

extern int bitmap_scnprintf(char *buf, unsigned int len,
   const unsigned long *src, int nbits);
extern int __bitmap_parse(const char *buf, unsigned int buflen, int is_user,
   unsigned long *dst, int nbits);
extern int bitmap_parse_user(const char *ubuf, unsigned int ulen,
   unsigned long *dst, int nbits);
extern int bitmap_scnlistprintf(char *buf, unsigned int len,
   const unsigned long *src, int nbits);
extern int bitmap_parselist(const char *buf, unsigned long *maskp,
   int nmaskbits);
extern int bitmap_parselist_user(const char *ubuf, unsigned int ulen,
   unsigned long *dst, int nbits);
extern void bitmap_remap(unsigned long *dst, const unsigned long *src,
  const unsigned long *old, const unsigned long *new, int bits);
extern int bitmap_bitremap(int oldbit,
  const unsigned long *old, const unsigned long *new, int bits);
extern void bitmap_onto(unsigned long *dst, const unsigned long *orig,
  const unsigned long *relmap, int bits);
extern void bitmap_fold(unsigned long *dst, const unsigned long *orig,
  int sz, int bits);
extern int bitmap_find_free_region(unsigned long *bitmap, int bits, int order);
extern void bitmap_release_region(unsigned long *bitmap, int pos, int order);
extern int bitmap_allocate_region(unsigned long *bitmap, int pos, int order);
extern void bitmap_copy_le(void *dst, const unsigned long *src, int nbits);
extern int bitmap_ord_to_pos(const unsigned long *bitmap, int n, int bits);
# 159 "include/linux/bitmap.h"
static inline void bitmap_zero(unsigned long *dst, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = 0UL;
 else {
  int len = (((nbits) + (8 * sizeof(long)) - 1) / (8 * sizeof(long))) * sizeof(unsigned long);
  __builtin_memset(dst, 0, len);
 }
}

static inline void bitmap_fill(unsigned long *dst, int nbits)
{
 size_t nlongs = (((nbits) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)));
 if (!(__builtin_constant_p(nbits) && (nbits) <= 32)) {
  int len = (nlongs - 1) * sizeof(unsigned long);
  __builtin_memset(dst, 0xff, len);
 }
 dst[nlongs - 1] = ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL );
}

static inline void bitmap_copy(unsigned long *dst, const unsigned long *src,
   int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = *src;
 else {
  int len = (((nbits) + (8 * sizeof(long)) - 1) / (8 * sizeof(long))) * sizeof(unsigned long);
  __builtin_memcpy(dst, src, len);
 }
}

static inline int bitmap_and(unsigned long *dst, const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return (*dst = *src1 & *src2) != 0;
 return __bitmap_and(dst, src1, src2, nbits);
}

static inline void bitmap_or(unsigned long *dst, const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = *src1 | *src2;
 else
  __bitmap_or(dst, src1, src2, nbits);
}

static inline void bitmap_xor(unsigned long *dst, const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = *src1 ^ *src2;
 else
  __bitmap_xor(dst, src1, src2, nbits);
}

static inline int bitmap_andnot(unsigned long *dst, const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return (*dst = *src1 & ~(*src2)) != 0;
 return __bitmap_andnot(dst, src1, src2, nbits);
}

static inline void bitmap_complement(unsigned long *dst, const unsigned long *src,
   int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = ~(*src) & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL );
 else
  __bitmap_complement(dst, src, nbits);
}

static inline int bitmap_equal(const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return ! ((*src1 ^ *src2) & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL ));
 else
  return __bitmap_equal(src1, src2, nbits);
}

static inline int bitmap_intersects(const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return ((*src1 & *src2) & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL )) != 0;
 else
  return __bitmap_intersects(src1, src2, nbits);
}

static inline int bitmap_subset(const unsigned long *src1,
   const unsigned long *src2, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return ! ((*src1 & ~(*src2)) & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL ));
 else
  return __bitmap_subset(src1, src2, nbits);
}

static inline int bitmap_empty(const unsigned long *src, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return ! (*src & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL ));
 else
  return __bitmap_empty(src, nbits);
}

static inline int bitmap_full(const unsigned long *src, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return ! (~(*src) & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL ));
 else
  return __bitmap_full(src, nbits);
}

static inline int bitmap_weight(const unsigned long *src, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  return hweight_long(*src & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL ));
 return __bitmap_weight(src, nbits);
}

static inline void bitmap_shift_right(unsigned long *dst,
   const unsigned long *src, int n, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = *src >> n;
 else
  __bitmap_shift_right(dst, src, n, nbits);
}

static inline void bitmap_shift_left(unsigned long *dst,
   const unsigned long *src, int n, int nbits)
{
 if ((__builtin_constant_p(nbits) && (nbits) <= 32))
  *dst = (*src << n) & ( ((nbits) % 32) ? (1UL<<((nbits) % 32))-1 : ~0UL );
 else
  __bitmap_shift_left(dst, src, n, nbits);
}

static inline int bitmap_parse(const char *buf, unsigned int buflen,
   unsigned long *maskp, int nmaskbits)
{
 return __bitmap_parse(buf, buflen, 0, maskp, nmaskbits);
}
# 12 "include/linux/cpumask.h" 2

typedef struct cpumask { unsigned long bits[(((8) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)))]; } cpumask_t;
# 27 "include/linux/cpumask.h"
extern int nr_cpu_ids;
# 78 "include/linux/cpumask.h"
extern const struct cpumask *const cpu_possible_mask;
extern const struct cpumask *const cpu_online_mask;
extern const struct cpumask *const cpu_present_mask;
extern const struct cpumask *const cpu_active_mask;
# 104 "include/linux/cpumask.h"
static inline unsigned int cpumask_check(unsigned int cpu)
{



 return cpu;
}
# 157 "include/linux/cpumask.h"
static inline unsigned int cpumask_first(const struct cpumask *srcp)
{
 return find_first_bit(((srcp)->bits), 8);
}
# 169 "include/linux/cpumask.h"
static inline unsigned int cpumask_next(int n, const struct cpumask *srcp)
{

 if (n != -1)
  cpumask_check(n);
 return find_next_bit(((srcp)->bits), 8, n+1);
}
# 184 "include/linux/cpumask.h"
static inline unsigned int cpumask_next_zero(int n, const struct cpumask *srcp)
{

 if (n != -1)
  cpumask_check(n);
 return find_next_zero_bit(((srcp)->bits), 8, n+1);
}

int cpumask_next_and(int n, const struct cpumask *, const struct cpumask *);
int cpumask_any_but(const struct cpumask *mask, unsigned int cpu);
# 254 "include/linux/cpumask.h"
static inline void cpumask_set_cpu(unsigned int cpu, struct cpumask *dstp)
{
 set_bit(cpumask_check(cpu), ((dstp)->bits));
}






static inline void cpumask_clear_cpu(int cpu, struct cpumask *dstp)
{
 clear_bit(cpumask_check(cpu), ((dstp)->bits));
}
# 286 "include/linux/cpumask.h"
static inline int cpumask_test_and_set_cpu(int cpu, struct cpumask *cpumask)
{
 return test_and_set_bit(cpumask_check(cpu), ((cpumask)->bits));
}
# 298 "include/linux/cpumask.h"
static inline int cpumask_test_and_clear_cpu(int cpu, struct cpumask *cpumask)
{
 return test_and_clear_bit(cpumask_check(cpu), ((cpumask)->bits));
}





static inline void cpumask_setall(struct cpumask *dstp)
{
 bitmap_fill(((dstp)->bits), 8);
}





static inline void cpumask_clear(struct cpumask *dstp)
{
 bitmap_zero(((dstp)->bits), 8);
}







static inline int cpumask_and(struct cpumask *dstp,
          const struct cpumask *src1p,
          const struct cpumask *src2p)
{
 return bitmap_and(((dstp)->bits), ((src1p)->bits),
           ((src2p)->bits), 8);
}







static inline void cpumask_or(struct cpumask *dstp, const struct cpumask *src1p,
         const struct cpumask *src2p)
{
 bitmap_or(((dstp)->bits), ((src1p)->bits),
          ((src2p)->bits), 8);
}







static inline void cpumask_xor(struct cpumask *dstp,
          const struct cpumask *src1p,
          const struct cpumask *src2p)
{
 bitmap_xor(((dstp)->bits), ((src1p)->bits),
           ((src2p)->bits), 8);
}







static inline int cpumask_andnot(struct cpumask *dstp,
      const struct cpumask *src1p,
      const struct cpumask *src2p)
{
 return bitmap_andnot(((dstp)->bits), ((src1p)->bits),
       ((src2p)->bits), 8);
}






static inline void cpumask_complement(struct cpumask *dstp,
          const struct cpumask *srcp)
{
 bitmap_complement(((dstp)->bits), ((srcp)->bits),
           8);
}






static inline bool cpumask_equal(const struct cpumask *src1p,
    const struct cpumask *src2p)
{
 return bitmap_equal(((src1p)->bits), ((src2p)->bits),
       8);
}






static inline bool cpumask_intersects(const struct cpumask *src1p,
         const struct cpumask *src2p)
{
 return bitmap_intersects(((src1p)->bits), ((src2p)->bits),
            8);
}






static inline int cpumask_subset(const struct cpumask *src1p,
     const struct cpumask *src2p)
{
 return bitmap_subset(((src1p)->bits), ((src2p)->bits),
        8);
}





static inline bool cpumask_empty(const struct cpumask *srcp)
{
 return bitmap_empty(((srcp)->bits), 8);
}





static inline bool cpumask_full(const struct cpumask *srcp)
{
 return bitmap_full(((srcp)->bits), 8);
}





static inline unsigned int cpumask_weight(const struct cpumask *srcp)
{
 return bitmap_weight(((srcp)->bits), 8);
}







static inline void cpumask_shift_right(struct cpumask *dstp,
           const struct cpumask *srcp, int n)
{
 bitmap_shift_right(((dstp)->bits), ((srcp)->bits), n,
            8);
}







static inline void cpumask_shift_left(struct cpumask *dstp,
          const struct cpumask *srcp, int n)
{
 bitmap_shift_left(((dstp)->bits), ((srcp)->bits), n,
           8);
}






static inline void cpumask_copy(struct cpumask *dstp,
    const struct cpumask *srcp)
{
 bitmap_copy(((dstp)->bits), ((srcp)->bits), 8);
}
# 529 "include/linux/cpumask.h"
static inline int cpumask_scnprintf(char *buf, int len,
        const struct cpumask *srcp)
{
 return bitmap_scnprintf(buf, len, ((srcp)->bits), 8);
}
# 543 "include/linux/cpumask.h"
static inline int cpumask_parse_user(const char *buf, int len,
         struct cpumask *dstp)
{
 return bitmap_parse_user(buf, len, ((dstp)->bits), 8);
}
# 557 "include/linux/cpumask.h"
static inline int cpumask_parselist_user(const char *buf, int len,
         struct cpumask *dstp)
{
 return bitmap_parselist_user(buf, len, ((dstp)->bits),
       8);
}
# 573 "include/linux/cpumask.h"
static inline int cpulist_scnprintf(char *buf, int len,
        const struct cpumask *srcp)
{
 return bitmap_scnlistprintf(buf, len, ((srcp)->bits),
        8);
}
# 588 "include/linux/cpumask.h"
static inline int cpulist_parse(const char *buf, struct cpumask *dstp)
{
 return bitmap_parselist(buf, ((dstp)->bits), 8);
}






static inline size_t cpumask_size(void)
{


 return (((8) + (8 * sizeof(long)) - 1) / (8 * sizeof(long))) * sizeof(long);
}
# 647 "include/linux/cpumask.h"
typedef struct cpumask cpumask_var_t[1];

static inline bool alloc_cpumask_var(cpumask_var_t *mask, gfp_t flags)
{
 return true;
}

static inline bool alloc_cpumask_var_node(cpumask_var_t *mask, gfp_t flags,
       int node)
{
 return true;
}

static inline bool zalloc_cpumask_var(cpumask_var_t *mask, gfp_t flags)
{
 cpumask_clear(*mask);
 return true;
}

static inline bool zalloc_cpumask_var_node(cpumask_var_t *mask, gfp_t flags,
       int node)
{
 cpumask_clear(*mask);
 return true;
}

static inline void alloc_bootmem_cpumask_var(cpumask_var_t *mask)
{
}

static inline void free_cpumask_var(cpumask_var_t mask)
{
}

static inline void free_bootmem_cpumask_var(cpumask_var_t mask)
{
}




extern const unsigned long cpu_all_bits[(((8) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)))];
# 699 "include/linux/cpumask.h"
void set_cpu_possible(unsigned int cpu, bool possible);
void set_cpu_present(unsigned int cpu, bool present);
void set_cpu_online(unsigned int cpu, bool online);
void set_cpu_active(unsigned int cpu, bool active);
void init_cpu_present(const struct cpumask *src);
void init_cpu_possible(const struct cpumask *src);
void init_cpu_online(const struct cpumask *src);
# 721 "include/linux/cpumask.h"
static inline int __check_is_bitmap(const unsigned long *bitmap)
{
 return 1;
}
# 733 "include/linux/cpumask.h"
extern const unsigned long
 cpu_bit_bitmap[32 +1][(((8) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)))];

static inline const struct cpumask *get_cpu_mask(unsigned int cpu)
{
 const unsigned long *p = cpu_bit_bitmap[1 + cpu % 32];
 p -= cpu / 32;
 return ((struct cpumask *)(1 ? (p) : (void *)sizeof(__check_is_bitmap(p))));
}
# 810 "include/linux/cpumask.h"
int __first_cpu(const cpumask_t *srcp);
int __next_cpu(int n, const cpumask_t *srcp);
int __any_online_cpu(const cpumask_t *mask);
# 840 "include/linux/cpumask.h"
static inline void __cpu_set(int cpu, volatile cpumask_t *dstp)
{
 set_bit(cpu, dstp->bits);
}


static inline void __cpu_clear(int cpu, volatile cpumask_t *dstp)
{
 clear_bit(cpu, dstp->bits);
}


static inline void __cpus_setall(cpumask_t *dstp, int nbits)
{
 bitmap_fill(dstp->bits, nbits);
}


static inline void __cpus_clear(cpumask_t *dstp, int nbits)
{
 bitmap_zero(dstp->bits, nbits);
}





static inline int __cpu_test_and_set(int cpu, cpumask_t *addr)
{
 return test_and_set_bit(cpu, addr->bits);
}


static inline int __cpus_and(cpumask_t *dstp, const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 return bitmap_and(dstp->bits, src1p->bits, src2p->bits, nbits);
}


static inline void __cpus_or(cpumask_t *dstp, const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 bitmap_or(dstp->bits, src1p->bits, src2p->bits, nbits);
}


static inline void __cpus_xor(cpumask_t *dstp, const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 bitmap_xor(dstp->bits, src1p->bits, src2p->bits, nbits);
}



static inline int __cpus_andnot(cpumask_t *dstp, const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 return bitmap_andnot(dstp->bits, src1p->bits, src2p->bits, nbits);
}


static inline int __cpus_equal(const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 return bitmap_equal(src1p->bits, src2p->bits, nbits);
}


static inline int __cpus_intersects(const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 return bitmap_intersects(src1p->bits, src2p->bits, nbits);
}


static inline int __cpus_subset(const cpumask_t *src1p,
     const cpumask_t *src2p, int nbits)
{
 return bitmap_subset(src1p->bits, src2p->bits, nbits);
}


static inline int __cpus_empty(const cpumask_t *srcp, int nbits)
{
 return bitmap_empty(srcp->bits, nbits);
}


static inline int __cpus_weight(const cpumask_t *srcp, int nbits)
{
 return bitmap_weight(srcp->bits, nbits);
}



static inline void __cpus_shift_left(cpumask_t *dstp,
     const cpumask_t *srcp, int n, int nbits)
{
 bitmap_shift_left(dstp->bits, srcp->bits, n, nbits);
}
# 15 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h" 2

static inline int paravirt_enabled(void)
{
 return pv_info.paravirt_enabled;
}

static inline void load_sp0(struct tss_struct *tss,
        struct thread_struct *thread)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.load_sp0); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.load_sp0) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.load_sp0)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(tss)), "d" ((unsigned long)(thread)) : "memory", "cc" ); });
}


static inline void __cpuid(unsigned int *eax, unsigned int *ebx,
      unsigned int *ecx, unsigned int *edx)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.cpuid); asm volatile("push %[_arg4];" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "lea 4(%%esp),%%esp;" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.cpuid) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.cpuid)), [paravirt_clobber] "i" (((1 << 4) - 1)), "0" ((u32)(eax)), "1" ((u32)(ebx)), "2" ((u32)(ecx)), [_arg4] "mr" ((u32)(edx)) : "memory", "cc" ); });
}




static inline unsigned long paravirt_get_debugreg(int reg)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.get_debugreg); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.get_debugreg) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.get_debugreg)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(reg)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.get_debugreg) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.get_debugreg)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(reg)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline void set_debugreg(unsigned long val, int reg)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.set_debugreg); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.set_debugreg) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.set_debugreg)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(reg)), "d" ((unsigned long)(val)) : "memory", "cc" ); });
}

static inline void clts(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.clts); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.clts) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.clts)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}

static inline unsigned long read_cr0(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_cr0); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_cr0) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_cr0)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_cr0) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_cr0)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline void write_cr0(unsigned long x)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.write_cr0); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_cr0) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_cr0)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(x)) : "memory", "cc" ); });
}

static inline unsigned long read_cr2(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.read_cr2); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.read_cr2) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.read_cr2)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.read_cr2) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.read_cr2)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline void write_cr2(unsigned long x)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.write_cr2); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.write_cr2) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.write_cr2)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(x)) : "memory", "cc" ); });
}

static inline unsigned long read_cr3(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.read_cr3); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.read_cr3) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.read_cr3)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.read_cr3) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.read_cr3)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline void write_cr3(unsigned long x)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.write_cr3); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.write_cr3) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.write_cr3)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(x)) : "memory", "cc" ); });
}

static inline unsigned long read_cr4(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_cr4); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_cr4) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_cr4)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_cr4) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_cr4)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}
static inline unsigned long read_cr4_safe(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_cr4_safe); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_cr4_safe) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_cr4_safe)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_cr4_safe) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_cr4_safe)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline void write_cr4(unsigned long x)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.write_cr4); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_cr4) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_cr4)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(x)) : "memory", "cc" ); });
}
# 108 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline void arch_safe_halt(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_irq_ops.safe_halt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.safe_halt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.safe_halt)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}

static inline void halt(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_irq_ops.halt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.halt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.halt)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}

static inline void wbinvd(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.wbinvd); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.wbinvd) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.wbinvd)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}



static inline u64 paravirt_read_msr(unsigned msr, int *err)
{
 return ({ u64 __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_msr); if (sizeof(u64) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_msr) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_msr)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(msr)), "d" ((unsigned long)(err)) : "memory", "cc" ); __ret = (u64)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_msr) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_msr)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(msr)), "d" ((unsigned long)(err)) : "memory", "cc" ); __ret = (u64)__eax; } __ret; });
}

static inline int paravirt_rdmsr_regs(u32 *regs)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.rdmsr_regs); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.rdmsr_regs) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.rdmsr_regs)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(regs)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.rdmsr_regs) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.rdmsr_regs)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(regs)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}

static inline int paravirt_write_msr(unsigned msr, unsigned low, unsigned high)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.write_msr); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_msr) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_msr)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(msr)), "d" ((unsigned long)(low)), "c" ((unsigned long)(high)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_msr) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_msr)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(msr)), "d" ((unsigned long)(low)), "c" ((unsigned long)(high)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}

static inline int paravirt_wrmsr_regs(u32 *regs)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.wrmsr_regs); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.wrmsr_regs) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.wrmsr_regs)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(regs)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.wrmsr_regs) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.wrmsr_regs)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(regs)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}
# 181 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline int rdmsrl_safe(unsigned msr, unsigned long long *p)
{
 int err;

 *p = paravirt_read_msr(msr, &err);
 return err;
}
static inline int rdmsrl_amd_safe(unsigned msr, unsigned long long *p)
{
 u32 gprs[8] = { 0 };
 int err;

 gprs[1] = msr;
 gprs[7] = 0x9c5a203a;

 err = paravirt_rdmsr_regs(gprs);

 *p = gprs[0] | ((u64)gprs[2] << 32);

 return err;
}

static inline int wrmsrl_amd_safe(unsigned msr, unsigned long long val)
{
 u32 gprs[8] = { 0 };

 gprs[0] = (u32)val;
 gprs[1] = msr;
 gprs[2] = val >> 32;
 gprs[7] = 0x9c5a203a;

 return paravirt_wrmsr_regs(gprs);
}

static inline u64 paravirt_read_tsc(void)
{
 return ({ u64 __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_tsc); if (sizeof(u64) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_tsc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_tsc)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (u64)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_tsc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_tsc)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (u64)__eax; } __ret; });
}
# 228 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline unsigned long long paravirt_sched_clock(void)
{
 return ({ unsigned long long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_time_ops.sched_clock); if (sizeof(unsigned long long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_time_ops.sched_clock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_time_ops.sched_clock)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_time_ops.sched_clock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_time_ops.sched_clock)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long long)__eax; } __ret; });
}

struct jump_label_key;
extern struct jump_label_key paravirt_steal_enabled;
extern struct jump_label_key paravirt_steal_rq_enabled;

static inline u64 paravirt_steal_clock(int cpu)
{
 return ({ u64 __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_time_ops.steal_clock); if (sizeof(u64) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_time_ops.steal_clock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_time_ops.steal_clock)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(cpu)) : "memory", "cc" ); __ret = (u64)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_time_ops.steal_clock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_time_ops.steal_clock)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(cpu)) : "memory", "cc" ); __ret = (u64)__eax; } __ret; });
}

static inline unsigned long long paravirt_read_pmc(int counter)
{
 return ({ u64 __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_pmc); if (sizeof(u64) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_pmc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_pmc)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(counter)) : "memory", "cc" ); __ret = (u64)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_pmc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_pmc)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(counter)) : "memory", "cc" ); __ret = (u64)__eax; } __ret; });
}
# 254 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline unsigned long long paravirt_rdtscp(unsigned int *aux)
{
 return ({ u64 __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.read_tscp); if (sizeof(u64) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_tscp) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_tscp)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(aux)) : "memory", "cc" ); __ret = (u64)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.read_tscp) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.read_tscp)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(aux)) : "memory", "cc" ); __ret = (u64)__eax; } __ret; });
}
# 275 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline void paravirt_alloc_ldt(struct desc_struct *ldt, unsigned entries)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.alloc_ldt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.alloc_ldt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.alloc_ldt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(ldt)), "d" ((unsigned long)(entries)) : "memory", "cc" ); });
}

static inline void paravirt_free_ldt(struct desc_struct *ldt, unsigned entries)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.free_ldt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.free_ldt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.free_ldt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(ldt)), "d" ((unsigned long)(entries)) : "memory", "cc" ); });
}

static inline void load_TR_desc(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.load_tr_desc); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.load_tr_desc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.load_tr_desc)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}
static inline void load_gdt(const struct desc_ptr *dtr)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.load_gdt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.load_gdt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.load_gdt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(dtr)) : "memory", "cc" ); });
}
static inline void load_idt(const struct desc_ptr *dtr)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.load_idt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.load_idt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.load_idt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(dtr)) : "memory", "cc" ); });
}
static inline void set_ldt(const void *addr, unsigned entries)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.set_ldt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.set_ldt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.set_ldt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(addr)), "d" ((unsigned long)(entries)) : "memory", "cc" ); });
}

static inline void load_user_cs_desc(unsigned int cpu, struct mm_struct *mm)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.load_user_cs_desc); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.load_user_cs_desc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.load_user_cs_desc)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(cpu)), "d" ((unsigned long)(mm)) : "memory", "cc" ); });
}

static inline void store_gdt(struct desc_ptr *dtr)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.store_gdt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.store_gdt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.store_gdt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(dtr)) : "memory", "cc" ); });
}
static inline void store_idt(struct desc_ptr *dtr)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.store_idt); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.store_idt) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.store_idt)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(dtr)) : "memory", "cc" ); });
}
static inline unsigned long paravirt_store_tr(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.store_tr); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.store_tr) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.store_tr)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.store_tr) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.store_tr)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline void load_TLS(struct thread_struct *t, unsigned cpu)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.load_tls); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.load_tls) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.load_tls)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(t)), "d" ((unsigned long)(cpu)) : "memory", "cc" ); });
}
# 332 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline void write_ldt_entry(struct desc_struct *dt, int entry,
       const void *desc)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.write_ldt_entry); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_ldt_entry) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_ldt_entry)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(dt)), "d" ((unsigned long)(entry)), "c" ((unsigned long)(desc)) : "memory", "cc" ); });
}

static inline void write_gdt_entry(struct desc_struct *dt, int entry,
       void *desc, int type)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.write_gdt_entry); asm volatile("push %[_arg4];" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "lea 4(%%esp),%%esp;" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_gdt_entry) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_gdt_entry)), [paravirt_clobber] "i" (((1 << 4) - 1)), "0" ((u32)(dt)), "1" ((u32)(entry)), "2" ((u32)(desc)), [_arg4] "mr" ((u32)(type)) : "memory", "cc" ); });
}

static inline void write_idt_entry(gate_desc *dt, int entry, const gate_desc *g)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.write_idt_entry); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.write_idt_entry) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.write_idt_entry)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(dt)), "d" ((unsigned long)(entry)), "c" ((unsigned long)(g)) : "memory", "cc" ); });
}
static inline void set_iopl_mask(unsigned mask)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.set_iopl_mask); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.set_iopl_mask) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.set_iopl_mask)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mask)) : "memory", "cc" ); });
}


static inline void slow_down_io(void)
{
 pv_cpu_ops.io_delay();





}


static inline void startup_ipi_hook(int phys_apicid, unsigned long start_eip,
        unsigned long start_esp)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_apic_ops.startup_ipi_hook); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_apic_ops.startup_ipi_hook) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_apic_ops.startup_ipi_hook)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(phys_apicid)), "d" ((unsigned long)(start_eip)), "c" ((unsigned long)(start_esp)) : "memory", "cc" ); })
                                        ;
}


static inline void paravirt_activate_mm(struct mm_struct *prev,
     struct mm_struct *next)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.activate_mm); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.activate_mm) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.activate_mm)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(prev)), "d" ((unsigned long)(next)) : "memory", "cc" ); });
}

static inline void arch_dup_mmap(struct mm_struct *oldmm,
     struct mm_struct *mm)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.dup_mmap); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.dup_mmap) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.dup_mmap)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(oldmm)), "d" ((unsigned long)(mm)) : "memory", "cc" ); });
}

static inline void arch_exit_mmap(struct mm_struct *mm)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.exit_mmap); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.exit_mmap) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.exit_mmap)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)) : "memory", "cc" ); });
}

static inline void __flush_tlb(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.flush_tlb_user); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.flush_tlb_user) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.flush_tlb_user)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}
static inline void __flush_tlb_global(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.flush_tlb_kernel); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.flush_tlb_kernel) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.flush_tlb_kernel)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}
static inline void __flush_tlb_single(unsigned long addr)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.flush_tlb_single); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.flush_tlb_single) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.flush_tlb_single)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(addr)) : "memory", "cc" ); });
}

static inline void flush_tlb_others(const struct cpumask *cpumask,
        struct mm_struct *mm,
        unsigned long va)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.flush_tlb_others); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.flush_tlb_others) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.flush_tlb_others)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(cpumask)), "d" ((unsigned long)(mm)), "c" ((unsigned long)(va)) : "memory", "cc" ); });
}

static inline int paravirt_pgd_alloc(struct mm_struct *mm)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pgd_alloc); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_alloc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_alloc)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_alloc) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_alloc)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}

static inline void paravirt_pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pgd_free); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_free) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_free)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(pgd)) : "memory", "cc" ); });
}

static inline void paravirt_alloc_pte(struct mm_struct *mm, unsigned long pfn)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.alloc_pte); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.alloc_pte) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.alloc_pte)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(pfn)) : "memory", "cc" ); });
}
static inline void paravirt_release_pte(unsigned long pfn)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.release_pte); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.release_pte) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.release_pte)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pfn)) : "memory", "cc" ); });
}

static inline void paravirt_alloc_pmd(struct mm_struct *mm, unsigned long pfn)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.alloc_pmd); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.alloc_pmd) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.alloc_pmd)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(pfn)) : "memory", "cc" ); });
}

static inline void paravirt_release_pmd(unsigned long pfn)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.release_pmd); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.release_pmd) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.release_pmd)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pfn)) : "memory", "cc" ); });
}

static inline void paravirt_alloc_pud(struct mm_struct *mm, unsigned long pfn)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.alloc_pud); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.alloc_pud) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.alloc_pud)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(pfn)) : "memory", "cc" ); });
}
static inline void paravirt_release_pud(unsigned long pfn)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.release_pud); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.release_pud) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.release_pud)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pfn)) : "memory", "cc" ); });
}

static inline void pte_update(struct mm_struct *mm, unsigned long addr,
         pte_t *ptep)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pte_update); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_update) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_update)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(ptep)) : "memory", "cc" ); });
}
static inline void pmd_update(struct mm_struct *mm, unsigned long addr,
         pmd_t *pmdp)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pmd_update); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_update) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_update)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(pmdp)) : "memory", "cc" ); });
}

static inline void pte_update_defer(struct mm_struct *mm, unsigned long addr,
        pte_t *ptep)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pte_update_defer); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_update_defer) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_update_defer)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(ptep)) : "memory", "cc" ); });
}

static inline void pmd_update_defer(struct mm_struct *mm, unsigned long addr,
        pmd_t *pmdp)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pmd_update_defer); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_update_defer) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_update_defer)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(pmdp)) : "memory", "cc" ); });
}

static inline pte_t __pte(pteval_t val)
{
 pteval_t ret;

 if (sizeof(pteval_t) > sizeof(long))
  ret = ({ pteval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.make_pte.func); if (sizeof(pteval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pte.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pte.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)), "d" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); __ret = (pteval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pte.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pte.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)), "d" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); __ret = (pteval_t)__eax; } __ret; })

                           ;
 else
  ret = ({ pteval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.make_pte.func); if (sizeof(pteval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pte.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pte.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)) : "memory", "cc" ); __ret = (pteval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pte.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pte.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)) : "memory", "cc" ); __ret = (pteval_t)__eax; } __ret; })

           ;

 return (pte_t) { .pte = ret };
}

static inline pteval_t pte_val(pte_t pte)
{
 pteval_t ret;

 if (sizeof(pteval_t) > sizeof(long))
  ret = ({ pteval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pte_val.func); if (sizeof(pteval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pte.pte)), "d" ((unsigned long)((u64)pte.pte >> 32)) : "memory", "cc" ); __ret = (pteval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pte.pte)), "d" ((unsigned long)((u64)pte.pte >> 32)) : "memory", "cc" ); __ret = (pteval_t)__eax; } __ret; })
                                   ;
 else
  ret = ({ pteval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pte_val.func); if (sizeof(pteval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pte.pte)) : "memory", "cc" ); __ret = (pteval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pte.pte)) : "memory", "cc" ); __ret = (pteval_t)__eax; } __ret; })
               ;

 return ret;
}

static inline pgd_t __pgd(pgdval_t val)
{
 pgdval_t ret;

 if (sizeof(pgdval_t) > sizeof(long))
  ret = ({ pgdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.make_pgd.func); if (sizeof(pgdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pgd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pgd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)), "d" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); __ret = (pgdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pgd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pgd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)), "d" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); __ret = (pgdval_t)__eax; } __ret; })
                           ;
 else
  ret = ({ pgdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.make_pgd.func); if (sizeof(pgdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pgd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pgd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)) : "memory", "cc" ); __ret = (pgdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pgd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pgd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)) : "memory", "cc" ); __ret = (pgdval_t)__eax; } __ret; })
           ;

 return (pgd_t) { ret };
}

static inline pgdval_t pgd_val(pgd_t pgd)
{
 pgdval_t ret;

 if (sizeof(pgdval_t) > sizeof(long))
  ret = ({ pgdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pgd_val.func); if (sizeof(pgdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pgd.pgd)), "d" ((unsigned long)((u64)pgd.pgd >> 32)) : "memory", "cc" ); __ret = (pgdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pgd.pgd)), "d" ((unsigned long)((u64)pgd.pgd >> 32)) : "memory", "cc" ); __ret = (pgdval_t)__eax; } __ret; })
                                    ;
 else
  ret = ({ pgdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pgd_val.func); if (sizeof(pgdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pgd.pgd)) : "memory", "cc" ); __ret = (pgdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pgd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pgd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pgd.pgd)) : "memory", "cc" ); __ret = (pgdval_t)__eax; } __ret; })
                ;

 return ret;
}


static inline pte_t ptep_modify_prot_start(struct mm_struct *mm, unsigned long addr,
        pte_t *ptep)
{
 pteval_t ret;

 ret = ({ pteval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.ptep_modify_prot_start); if (sizeof(pteval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.ptep_modify_prot_start) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.ptep_modify_prot_start)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(ptep)) : "memory", "cc" ); __ret = (pteval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.ptep_modify_prot_start) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.ptep_modify_prot_start)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(ptep)) : "memory", "cc" ); __ret = (pteval_t)__eax; } __ret; })
                   ;

 return (pte_t) { .pte = ret };
}

static inline void ptep_modify_prot_commit(struct mm_struct *mm, unsigned long addr,
        pte_t *ptep, pte_t pte)
{
 if (sizeof(pteval_t) > sizeof(long))

  pv_mmu_ops.ptep_modify_prot_commit(mm, addr, ptep, pte);
 else
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.ptep_modify_prot_commit); asm volatile("push %[_arg4];" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "lea 4(%%esp),%%esp;" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.ptep_modify_prot_commit) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.ptep_modify_prot_commit)), [paravirt_clobber] "i" (((1 << 4) - 1)), "0" ((u32)(mm)), "1" ((u32)(addr)), "2" ((u32)(ptep)), [_arg4] "mr" ((u32)(pte.pte)) : "memory", "cc" ); })
                               ;
}

static inline void set_pte(pte_t *ptep, pte_t pte)
{
 if (sizeof(pteval_t) > sizeof(long))
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pte); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pte) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pte)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(ptep)), "d" ((unsigned long)(pte.pte)), "c" ((unsigned long)((u64)pte.pte >> 32)) : "memory", "cc" ); })
                                   ;
 else
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pte); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pte) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pte)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(ptep)), "d" ((unsigned long)(pte.pte)) : "memory", "cc" ); })
               ;
}

static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
         pte_t *ptep, pte_t pte)
{
 if (sizeof(pteval_t) > sizeof(long))

  pv_mmu_ops.set_pte_at(mm, addr, ptep, pte);
 else
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pte_at); asm volatile("push %[_arg4];" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "lea 4(%%esp),%%esp;" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pte_at) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pte_at)), [paravirt_clobber] "i" (((1 << 4) - 1)), "0" ((u32)(mm)), "1" ((u32)(addr)), "2" ((u32)(ptep)), [_arg4] "mr" ((u32)(pte.pte)) : "memory", "cc" ); });
}


static inline void set_pmd_at(struct mm_struct *mm, unsigned long addr,
         pmd_t *pmdp, pmd_t pmd)
{
 if (sizeof(pmdval_t) > sizeof(long))

  pv_mmu_ops.set_pmd_at(mm, addr, pmdp, pmd);
 else
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pmd_at); asm volatile("push %[_arg4];" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "lea 4(%%esp),%%esp;" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pmd_at) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pmd_at)), [paravirt_clobber] "i" (((1 << 4) - 1)), "0" ((u32)(mm)), "1" ((u32)(addr)), "2" ((u32)(pmdp)), [_arg4] "mr" ((u32)(native_pmd_val(pmd))) : "memory", "cc" ); })
                           ;
}


static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
 pmdval_t val = native_pmd_val(pmd);

 if (sizeof(pmdval_t) > sizeof(long))
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pmd); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pmd) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pmd)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pmdp)), "d" ((unsigned long)(val)), "c" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); });
 else
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pmd); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pmd) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pmd)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pmdp)), "d" ((unsigned long)(val)) : "memory", "cc" ); });
}


static inline pmd_t __pmd(pmdval_t val)
{
 pmdval_t ret;

 if (sizeof(pmdval_t) > sizeof(long))
  ret = ({ pmdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.make_pmd.func); if (sizeof(pmdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pmd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pmd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)), "d" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); __ret = (pmdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pmd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pmd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)), "d" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); __ret = (pmdval_t)__eax; } __ret; })
                           ;
 else
  ret = ({ pmdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.make_pmd.func); if (sizeof(pmdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pmd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pmd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)) : "memory", "cc" ); __ret = (pmdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.make_pmd.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.make_pmd.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(val)) : "memory", "cc" ); __ret = (pmdval_t)__eax; } __ret; })
           ;

 return (pmd_t) { ret };
}

static inline pmdval_t pmd_val(pmd_t pmd)
{
 pmdval_t ret;

 if (sizeof(pmdval_t) > sizeof(long))
  ret = ({ pmdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pmd_val.func); if (sizeof(pmdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pmd.pmd)), "d" ((unsigned long)((u64)pmd.pmd >> 32)) : "memory", "cc" ); __ret = (pmdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pmd.pmd)), "d" ((unsigned long)((u64)pmd.pmd >> 32)) : "memory", "cc" ); __ret = (pmdval_t)__eax; } __ret; })
                                    ;
 else
  ret = ({ pmdval_t __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pmd_val.func); if (sizeof(pmdval_t) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pmd.pmd)) : "memory", "cc" ); __ret = (pmdval_t)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_val.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_val.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(pmd.pmd)) : "memory", "cc" ); __ret = (pmdval_t)__eax; } __ret; })
                ;

 return ret;
}

static inline void set_pud(pud_t *pudp, pud_t pud)
{
 pudval_t val = native_pud_val(pud);

 if (sizeof(pudval_t) > sizeof(long))
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pud); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pud) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pud)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pudp)), "d" ((unsigned long)(val)), "c" ((unsigned long)((u64)val >> 32)) : "memory", "cc" ); })
                           ;
 else
  ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pud); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pud) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pud)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pudp)), "d" ((unsigned long)(val)) : "memory", "cc" ); })
           ;
}
# 693 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline void set_pte_atomic(pte_t *ptep, pte_t pte)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.set_pte_atomic); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.set_pte_atomic) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.set_pte_atomic)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(ptep)), "d" ((unsigned long)(pte.pte)), "c" ((unsigned long)(pte.pte >> 32)) : "memory", "cc" ); })
                             ;
}

static inline void pte_clear(struct mm_struct *mm, unsigned long addr,
        pte_t *ptep)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pte_clear); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pte_clear) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pte_clear)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(mm)), "d" ((unsigned long)(addr)), "c" ((unsigned long)(ptep)) : "memory", "cc" ); });
}

static inline void pmd_clear(pmd_t *pmdp)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.pmd_clear); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.pmd_clear) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.pmd_clear)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(pmdp)) : "memory", "cc" ); });
}
# 728 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline void arch_start_context_switch(struct task_struct *prev)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.start_context_switch); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.start_context_switch) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.start_context_switch)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(prev)) : "memory", "cc" ); });
}

static inline void arch_end_context_switch(struct task_struct *next)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_cpu_ops.end_context_switch); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_cpu_ops.end_context_switch) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_cpu_ops.end_context_switch)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(next)) : "memory", "cc" ); });
}


static inline void arch_enter_lazy_mmu_mode(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.lazy_mode.enter); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.lazy_mode.enter) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.lazy_mode.enter)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}

static inline void arch_leave_lazy_mmu_mode(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_mmu_ops.lazy_mode.leave); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_mmu_ops.lazy_mode.leave) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_mmu_ops.lazy_mode.leave)), [paravirt_clobber] "i" (((1 << 4) - 1)) : "memory", "cc" ); });
}

void arch_flush_lazy_mmu_mode(void);

static inline void __set_fixmap(unsigned idx,
    phys_addr_t phys, pgprot_t flags)
{
 pv_mmu_ops.set_fixmap(idx, phys, flags);
}



static inline int arch_spin_is_locked(struct arch_spinlock *lock)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_lock_ops.spin_is_locked); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_is_locked) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_is_locked)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_is_locked) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_is_locked)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}

static inline int arch_spin_is_contended(struct arch_spinlock *lock)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_lock_ops.spin_is_contended); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_is_contended) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_is_contended)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_is_contended) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_is_contended)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}


static inline __attribute__((always_inline)) void arch_spin_lock(struct arch_spinlock *lock)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_lock_ops.spin_lock); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_lock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_lock)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); });
}

static inline __attribute__((always_inline)) void arch_spin_lock_flags(struct arch_spinlock *lock,
        unsigned long flags)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_lock_ops.spin_lock_flags); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_lock_flags) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_lock_flags)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)), "d" ((unsigned long)(flags)) : "memory", "cc" ); });
}

static inline __attribute__((always_inline)) int arch_spin_trylock(struct arch_spinlock *lock)
{
 return ({ int __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_lock_ops.spin_trylock); if (sizeof(int) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_trylock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_trylock)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); __ret = (int)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_trylock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_trylock)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); __ret = (int)__eax; } __ret; });
}

static inline __attribute__((always_inline)) void arch_spin_unlock(struct arch_spinlock *lock)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_lock_ops.spin_unlock); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx), "=c" (__ecx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_lock_ops.spin_unlock) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_lock_ops.spin_unlock)), [paravirt_clobber] "i" (((1 << 4) - 1)), "a" ((unsigned long)(lock)) : "memory", "cc" ); });
}
# 866 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
static inline __attribute__((no_instrument_function)) unsigned long arch_local_save_flags(void)
{
 return ({ unsigned long __ret; unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_irq_ops.save_fl.func); if (sizeof(unsigned long) > sizeof(unsigned long)) { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.save_fl.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.save_fl.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))) : "memory", "cc" ); __ret = (unsigned long)((((u64)__edx) << 32) | __eax); } else { asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.save_fl.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.save_fl.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))) : "memory", "cc" ); __ret = (unsigned long)__eax; } __ret; });
}

static inline __attribute__((no_instrument_function)) void arch_local_irq_restore(unsigned long f)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_irq_ops.restore_fl.func); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.restore_fl.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.restore_fl.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))), "a" ((unsigned long)(f)) : "memory", "cc" ); });
}

static inline __attribute__((no_instrument_function)) void arch_local_irq_disable(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_irq_ops.irq_disable.func); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.irq_disable.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.irq_disable.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))) : "memory", "cc" ); });
}

static inline __attribute__((no_instrument_function)) void arch_local_irq_enable(void)
{
 ({ unsigned long __eax = __eax, __edx = __edx, __ecx = __ecx; ((void)pv_irq_ops.irq_enable.func); asm volatile("" "771:\n\t" "call *%c[paravirt_opptr];" "\n" "772:\n" ".pushsection .parainstructions,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " " 771b\n" "  .byte " "%c[paravirt_typenum]" "\n" "  .byte 772b-771b\n" "  .short " "%c[paravirt_clobber]" "\n" ".popsection\n" "" : "=a" (__eax), "=d" (__edx) : [paravirt_typenum] "i" ((__builtin_offsetof(struct paravirt_patch_template,pv_irq_ops.irq_enable.func) / sizeof(void *))), [paravirt_opptr] "i" (&(pv_irq_ops.irq_enable.func)), [paravirt_clobber] "i" (((1 << 0) | (1 << 2))) : "memory", "cc" ); });
}

static inline __attribute__((no_instrument_function)) unsigned long arch_local_irq_save(void)
{
 unsigned long f;

 f = arch_local_save_flags();
 arch_local_irq_disable();
 return f;
}
# 911 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/paravirt.h"
extern void default_banner(void);
# 61 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/irqflags.h" 2
# 155 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/irqflags.h"
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
 return !(flags & 0x00000200);
}

static inline int arch_irqs_disabled(void)
{
 unsigned long flags = arch_local_save_flags();

 return arch_irqs_disabled_flags(flags);
}
# 16 "include/linux/irqflags.h" 2
# 12 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h" 2
# 20 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
struct task_struct;
struct task_struct *__switch_to(struct task_struct *prev,
    struct task_struct *next);
struct tss_struct;
void __switch_to_xtra(struct task_struct *prev_p, struct task_struct *next_p,
        struct tss_struct *tss);
extern void show_regs_common(void);
# 151 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
extern void native_load_gs_index(unsigned);
# 199 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
static inline unsigned long get_limit(unsigned long segment)
{
 unsigned long __limit;
 asm("lsll %1,%0" : "=r" (__limit) : "r" (segment));
 return __limit + 1;
}

static inline void native_clts(void)
{
 asm volatile("clts");
}
# 218 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
static unsigned long __force_order;

static inline unsigned long native_read_cr0(void)
{
 unsigned long val;
 asm volatile("mov %%cr0,%0\n\t" : "=r" (val), "=m" (__force_order));
 return val;
}

static inline void native_write_cr0(unsigned long val)
{
 asm volatile("mov %0,%%cr0": : "r" (val), "m" (__force_order));
}

static inline unsigned long native_read_cr2(void)
{
 unsigned long val;
 asm volatile("mov %%cr2,%0\n\t" : "=r" (val), "=m" (__force_order));
 return val;
}

static inline void native_write_cr2(unsigned long val)
{
 asm volatile("mov %0,%%cr2": : "r" (val), "m" (__force_order));
}

static inline unsigned long native_read_cr3(void)
{
 unsigned long val;
 asm volatile("mov %%cr3,%0\n\t" : "=r" (val), "=m" (__force_order));
 return val;
}

static inline void native_write_cr3(unsigned long val)
{
 asm volatile("mov %0,%%cr3": : "r" (val), "m" (__force_order));
}

static inline unsigned long native_read_cr4(void)
{
 unsigned long val;
 asm volatile("mov %%cr4,%0\n\t" : "=r" (val), "=m" (__force_order));
 return val;
}

static inline unsigned long native_read_cr4_safe(void)
{
 unsigned long val;



 asm volatile("1: mov %%cr4, %0\n"
       "2:\n"
       " .section __ex_table,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " "1b" "," "2b" "\n" " .previous\n"
       : "=r" (val), "=m" (__force_order) : "0" (0));



 return val;
}

static inline void native_write_cr4(unsigned long val)
{
 asm volatile("mov %0,%%cr4": : "r" (val), "m" (__force_order));
}
# 298 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
static inline void native_wbinvd(void)
{
 asm volatile("wbinvd": : :"memory");
}
# 388 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
static inline void clflush(volatile void *__p)
{
 asm volatile("clflush %0" : "+m" (*(volatile char *)__p));
}



void disable_hlt(void);
void enable_hlt(void);

void cpu_idle_wait(void);

extern unsigned long arch_align_stack(unsigned long sp);
extern void free_init_pages(char *what, unsigned long begin, unsigned long end);

void default_idle(void);
bool set_pm_idle_to_default(void);

void stop_this_cpu(void *dummy);
# 510 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/system.h"
static inline __attribute__((always_inline)) void rdtsc_barrier(void)
{
 asm volatile ("661:\n\t" ".byte " "0x8d,0x76,0x00" "\n" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(3*32+17)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "mfence" "\n664:\n" ".previous" : : : "memory");
 asm volatile ("661:\n\t" ".byte " "0x8d,0x76,0x00" "\n" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(3*32+18)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "lfence" "\n664:\n" ".previous" : : : "memory");
}
# 18 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr-index.h" 1
# 5 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h" 2




# 1 "include/linux/ioctl.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ioctl.h" 1
# 1 "include/asm-generic/ioctl.h" 1
# 73 "include/asm-generic/ioctl.h"
extern unsigned int __invalid_size_argument_for_IOC;
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ioctl.h" 2
# 5 "include/linux/ioctl.h" 2
# 10 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h" 2







# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/errno.h" 1
# 1 "include/asm-generic/errno.h" 1



# 1 "include/asm-generic/errno-base.h" 1
# 5 "include/asm-generic/errno.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/errno.h" 2
# 18 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cpumask.h" 1





extern cpumask_var_t cpu_callin_mask;
extern cpumask_var_t cpu_callout_mask;
extern cpumask_var_t cpu_initialized_mask;
extern cpumask_var_t cpu_sibling_setup_mask;

extern void setup_cpu_local_masks(void);
# 19 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h" 2

struct msr {
 union {
  struct {
   u32 l;
   u32 h;
  };
  u64 q;
 };
};

struct msr_info {
 u32 msr_no;
 struct msr reg;
 struct msr *msrs;
 int err;
};

struct msr_regs_info {
 u32 *regs;
 int err;
};

static inline unsigned long long native_read_tscp(unsigned int *aux)
{
 unsigned long low, high;
 asm volatile(".byte 0x0f,0x01,0xf9"
       : "=a" (low), "=d" (high), "=c" (*aux));
 return low | ((u64)high << 32);
}
# 68 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h"
static inline unsigned long long native_read_msr(unsigned int msr)
{
 unsigned long long val;

 asm volatile("rdmsr" : "=A" (val) : "c" (msr));
 return (val);
}

static inline unsigned long long native_read_msr_safe(unsigned int msr,
            int *err)
{
 unsigned long long val;

 asm volatile("2: rdmsr ; xor %[err],%[err]\n"
       "1:\n\t"
       ".section .fixup,\"ax\"\n\t"
       "3:  mov %[fault],%[err] ; jmp 1b\n\t"
       ".previous\n\t"
       " .section __ex_table,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " "2b" "," "3b" "\n" " .previous\n"
       : [err] "=r" (*err), "=A" (val)
       : "c" (msr), [fault] "i" (-5));
 return (val);
}

static inline void native_write_msr(unsigned int msr,
        unsigned low, unsigned high)
{
 asm volatile("wrmsr" : : "c" (msr), "a"(low), "d" (high) : "memory");
}


__attribute__((no_instrument_function)) static inline int native_write_msr_safe(unsigned int msr,
     unsigned low, unsigned high)
{
 int err;
 asm volatile("2: wrmsr ; xor %[err],%[err]\n"
       "1:\n\t"
       ".section .fixup,\"ax\"\n\t"
       "3:  mov %[fault],%[err] ; jmp 1b\n\t"
       ".previous\n\t"
       " .section __ex_table,\"a\"\n" " " ".balign 4" " " "\n" " " ".long" " " "2b" "," "3b" "\n" " .previous\n"
       : [err] "=a" (err)
       : "c" (msr), "0" (low), "d" (high),
         [fault] "i" (-5)
       : "memory");
 return err;
}

extern unsigned long long native_read_tsc(void);

extern int native_rdmsr_safe_regs(u32 regs[8]);
extern int native_wrmsr_safe_regs(u32 regs[8]);

static inline __attribute__((always_inline)) unsigned long long __native_read_tsc(void)
{
 unsigned long long val;

 asm volatile("rdtsc" : "=A" (val));

 return (val);
}

static inline unsigned long long native_read_pmc(int counter)
{
 unsigned long long val;

 asm volatile("rdpmc" : "=A" (val) : "c" (counter));
 return (val);
}
# 266 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/msr.h"
struct msr *msrs_alloc(void);
void msrs_free(struct msr *msrs);


int rdmsr_on_cpu(unsigned int cpu, u32 msr_no, u32 *l, u32 *h);
int wrmsr_on_cpu(unsigned int cpu, u32 msr_no, u32 l, u32 h);
void rdmsr_on_cpus(const struct cpumask *mask, u32 msr_no, struct msr *msrs);
void wrmsr_on_cpus(const struct cpumask *mask, u32 msr_no, struct msr *msrs);
int rdmsr_safe_on_cpu(unsigned int cpu, u32 msr_no, u32 *l, u32 *h);
int wrmsr_safe_on_cpu(unsigned int cpu, u32 msr_no, u32 l, u32 h);
int rdmsr_safe_regs_on_cpu(unsigned int cpu, u32 regs[8]);
int wrmsr_safe_regs_on_cpu(unsigned int cpu, u32 regs[8]);
# 22 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2



# 1 "include/linux/personality.h" 1
# 10 "include/linux/personality.h"
struct exec_domain;
struct pt_regs;

extern int register_exec_domain(struct exec_domain *);
extern int unregister_exec_domain(struct exec_domain *);
extern int __set_personality(unsigned int);
# 24 "include/linux/personality.h"
enum {
 UNAME26 = 0x0020000,
 ADDR_NO_RANDOMIZE = 0x0040000,
 FDPIC_FUNCPTRS = 0x0080000,


 MMAP_PAGE_ZERO = 0x0100000,
 ADDR_COMPAT_LAYOUT = 0x0200000,
 READ_IMPLIES_EXEC = 0x0400000,
 ADDR_LIMIT_32BIT = 0x0800000,
 SHORT_INODE = 0x1000000,
 WHOLE_SECONDS = 0x2000000,
 STICKY_TIMEOUTS = 0x4000000,
 ADDR_LIMIT_3GB = 0x8000000,
};
# 55 "include/linux/personality.h"
enum {
 PER_LINUX = 0x0000,
 PER_LINUX_32BIT = 0x0000 | ADDR_LIMIT_32BIT,
 PER_LINUX_FDPIC = 0x0000 | FDPIC_FUNCPTRS,
 PER_SVR4 = 0x0001 | STICKY_TIMEOUTS | MMAP_PAGE_ZERO,
 PER_SVR3 = 0x0002 | STICKY_TIMEOUTS | SHORT_INODE,
 PER_SCOSVR3 = 0x0003 | STICKY_TIMEOUTS |
      WHOLE_SECONDS | SHORT_INODE,
 PER_OSR5 = 0x0003 | STICKY_TIMEOUTS | WHOLE_SECONDS,
 PER_WYSEV386 = 0x0004 | STICKY_TIMEOUTS | SHORT_INODE,
 PER_ISCR4 = 0x0005 | STICKY_TIMEOUTS,
 PER_BSD = 0x0006,
 PER_SUNOS = 0x0006 | STICKY_TIMEOUTS,
 PER_XENIX = 0x0007 | STICKY_TIMEOUTS | SHORT_INODE,
 PER_LINUX32 = 0x0008,
 PER_LINUX32_3GB = 0x0008 | ADDR_LIMIT_3GB,
 PER_IRIX32 = 0x0009 | STICKY_TIMEOUTS,
 PER_IRIXN32 = 0x000a | STICKY_TIMEOUTS,
 PER_IRIX64 = 0x000b | STICKY_TIMEOUTS,
 PER_RISCOS = 0x000c,
 PER_SOLARIS = 0x000d | STICKY_TIMEOUTS,
 PER_UW7 = 0x000e | STICKY_TIMEOUTS | MMAP_PAGE_ZERO,
 PER_OSF4 = 0x000f,
 PER_HPUX = 0x0010,
 PER_MASK = 0x00ff,
};
# 90 "include/linux/personality.h"
typedef void (*handler_t)(int, struct pt_regs *);

struct exec_domain {
 const char *name;
 handler_t handler;
 unsigned char pers_low;
 unsigned char pers_high;
 unsigned long *signal_map;
 unsigned long *signal_invmap;
 struct map_segment *err_map;
 struct map_segment *socktype_map;
 struct map_segment *sockopt_map;
 struct map_segment *af_map;
 struct module *module;
 struct exec_domain *next;
};
# 26 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2

# 1 "include/linux/cache.h" 1




# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/cache.h" 1
# 6 "include/linux/cache.h" 2
# 28 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2

# 1 "include/linux/math64.h" 1
# 61 "include/linux/math64.h"
extern s64 div_s64_rem(s64 dividend, s32 divisor, s32 *remainder);



extern u64 div64_u64(u64 dividend, u64 divisor);



extern s64 div64_s64(s64 dividend, s64 divisor);
# 82 "include/linux/math64.h"
static inline u64 div_u64(u64 dividend, u32 divisor)
{
 u32 remainder;
 return div_u64_rem(dividend, divisor, &remainder);
}






static inline s64 div_s64(s64 dividend, s32 divisor)
{
 s32 remainder;
 return div_s64_rem(dividend, divisor, &remainder);
}


u32 iter_div_u64_rem(u64 dividend, u32 divisor, u64 *remainder);

static inline __attribute__((always_inline)) u32
__iter_div_u64_rem(u64 dividend, u32 divisor, u64 *remainder)
{
 u32 ret = 0;

 while (dividend >= divisor) {


  asm("" : "+rm"(dividend));

  dividend -= divisor;
  ret++;
 }

 *remainder = dividend;

 return ret;
}
# 30 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2

# 1 "include/linux/err.h" 1





# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/errno.h" 1
# 7 "include/linux/err.h" 2
# 22 "include/linux/err.h"
static inline void * ERR_PTR(long error)
{
 return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
 return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
 return __builtin_expect(!!(((unsigned long)ptr) >= (unsigned long)-4095), 0);
}

static inline long IS_ERR_OR_NULL(const void *ptr)
{
 return !ptr || __builtin_expect(!!(((unsigned long)ptr) >= (unsigned long)-4095), 0);
}
# 49 "include/linux/err.h"
static inline void * ERR_CAST(const void *ptr)
{

 return (void *) ptr;
}

static inline int PTR_RET(const void *ptr)
{
 if (IS_ERR(ptr))
  return PTR_ERR(ptr);
 else
  return 0;
}
# 32 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h" 2






static inline void *current_text_addr(void)
{
 void *pc;

 asm volatile("mov $1f, %0; 1:":"=r" (pc));

 return pc;
}
# 61 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
struct cpuinfo_x86 {
 __u8 x86;
 __u8 x86_vendor;
 __u8 x86_model;
 __u8 x86_mask;

 char wp_works_ok;


 char hlt_works_ok;
 char hard_math;
 char rfu;
 char fdiv_bug;
 char f00f_bug;
 char coma_bug;
 char pad0;




 __u8 x86_virt_bits;
 __u8 x86_phys_bits;

 __u8 x86_coreid_bits;

 __u32 extended_cpuid_level;

 int cpuid_level;
 __u32 x86_capability[10];
 char x86_vendor_id[16];
 char x86_model_id[64];

 int x86_cache_size;
 int x86_cache_alignment;
 int x86_power;
 unsigned long loops_per_jiffy;

 u16 x86_max_cores;
 u16 apicid;
 u16 initial_apicid;
 u16 x86_clflush_size;


 u16 booted_cores;

 u16 phys_proc_id;

 u16 cpu_core_id;

 u8 compute_unit_id;

 u16 cpu_index;

 u32 microcode;
} __attribute__((__aligned__((1 << (6)))));
# 131 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
extern struct cpuinfo_x86 boot_cpu_data;
extern struct cpuinfo_x86 new_cpu_data;

extern struct tss_struct doublefault_tss;
extern __u32 cpu_caps_cleared[10];
extern __u32 cpu_caps_set[10];


extern __attribute__((section(".data..percpu" ""))) __typeof__(struct cpuinfo_x86) cpu_info __attribute__((__aligned__((1 << (6)))));






extern const struct seq_operations cpuinfo_op;

static inline int hlt_works(int cpu)
{

 return (*({ do { const void *__vpp_verify = (typeof((&(cpu_info))))((void *)0); (void)__vpp_verify; } while (0); ({ unsigned long __ptr; __asm__ ("" : "=r"(__ptr) : "0"((typeof(*(&(cpu_info))) *)(&(cpu_info)))); (typeof((typeof(*(&(cpu_info))) *)(&(cpu_info)))) (__ptr + (((__per_cpu_offset[cpu])))); }); })).hlt_works_ok;



}



extern void cpu_detect(struct cpuinfo_x86 *c);

extern struct pt_regs *idle_regs(struct pt_regs *);

extern void early_cpu_init(void);
extern void identify_boot_cpu(void);
extern void identify_secondary_cpu(struct cpuinfo_x86 *);
extern void print_cpu_info(struct cpuinfo_x86 *);
extern void init_scattered_cpuid_features(struct cpuinfo_x86 *c);
extern unsigned int init_intel_cacheinfo(struct cpuinfo_x86 *c);
extern unsigned short num_cache_leaves;

extern void detect_extended_topology(struct cpuinfo_x86 *c);
extern void detect_ht(struct cpuinfo_x86 *c);

static inline void native_cpuid(unsigned int *eax, unsigned int *ebx,
    unsigned int *ecx, unsigned int *edx)
{

 asm volatile("cpuid"
     : "=a" (*eax),
       "=b" (*ebx),
       "=c" (*ecx),
       "=d" (*edx)
     : "0" (*eax), "2" (*ecx)
     : "memory");
}

static inline void load_cr3(pgd_t *pgdir)
{
 write_cr3((((unsigned long)(pgdir)) - ((unsigned long)(0xC0000000UL))));
}



struct x86_hw_tss {
 unsigned short back_link, __blh;
 unsigned long sp0;
 unsigned short ss0, __ss0h;
 unsigned long sp1;

 unsigned short ss1, __ss1h;
 unsigned long sp2;
 unsigned short ss2, __ss2h;
 unsigned long __cr3;
 unsigned long ip;
 unsigned long flags;
 unsigned long ax;
 unsigned long cx;
 unsigned long dx;
 unsigned long bx;
 unsigned long sp;
 unsigned long bp;
 unsigned long si;
 unsigned long di;
 unsigned short es, __esh;
 unsigned short cs, __csh;
 unsigned short ss, __ssh;
 unsigned short ds, __dsh;
 unsigned short fs, __fsh;
 unsigned short gs, __gsh;
 unsigned short ldt, __ldth;
 unsigned short trace;
 unsigned short io_bitmap_base;

} __attribute__((packed));
# 250 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
struct tss_struct {



 struct x86_hw_tss x86_tss;







 unsigned long io_bitmap[((65536/8)/sizeof(long)) + 1];




 unsigned long stack[64];

} __attribute__((__aligned__((1 << (6)))));

extern __attribute__((section(".data..percpu" ""))) __typeof__(struct tss_struct) init_tss __attribute__((__aligned__((1 << (6)))));




struct orig_ist {
 unsigned long ist[7];
};



struct i387_fsave_struct {
 u32 cwd;
 u32 swd;
 u32 twd;
 u32 fip;
 u32 fcs;
 u32 foo;
 u32 fos;


 u32 st_space[20];


 u32 status;
};

struct i387_fxsave_struct {
 u16 cwd;
 u16 swd;
 u16 twd;
 u16 fop;
 union {
  struct {
   u64 rip;
   u64 rdp;
  };
  struct {
   u32 fip;
   u32 fcs;
   u32 foo;
   u32 fos;
  };
 };
 u32 mxcsr;
 u32 mxcsr_mask;


 u32 st_space[32];


 u32 xmm_space[64];

 u32 padding[12];

 union {
  u32 padding1[12];
  u32 sw_reserved[12];
 };

} __attribute__((aligned(16)));

struct i387_soft_struct {
 u32 cwd;
 u32 swd;
 u32 twd;
 u32 fip;
 u32 fcs;
 u32 foo;
 u32 fos;

 u32 st_space[20];
 u8 ftop;
 u8 changed;
 u8 lookahead;
 u8 no_update;
 u8 rm;
 u8 alimit;
 struct math_emu_info *info;
 u32 entry_eip;
};

struct ymmh_struct {

 u32 ymmh_space[64];
};

struct xsave_hdr_struct {
 u64 xstate_bv;
 u64 reserved1[2];
 u64 reserved2[5];
} __attribute__((packed));

struct xsave_struct {
 struct i387_fxsave_struct i387;
 struct xsave_hdr_struct xsave_hdr;
 struct ymmh_struct ymmh;

} __attribute__ ((packed, aligned (64)));

union thread_xstate {
 struct i387_fsave_struct fsave;
 struct i387_fxsave_struct fxsave;
 struct i387_soft_struct soft;
 struct xsave_struct xsave;
};

struct fpu {
 union thread_xstate *state;
};
# 413 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
struct stack_canary {
 char __pad[20];
 unsigned long canary;
};
extern __attribute__((section(".data..percpu" ""))) __typeof__(struct stack_canary) stack_canary __attribute__((__aligned__((1 << (6)))));



extern unsigned int xstate_size;
extern void free_thread_xstate(struct task_struct *);
extern struct kmem_cache *task_xstate_cachep;

struct perf_event;

struct thread_struct {

 struct desc_struct tls_array[3];
 unsigned long sp0;
 unsigned long sp;

 unsigned long sysenter_cs;
# 442 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
 unsigned long ip;




 unsigned long gs;

 struct perf_event *ptrace_bps[4];

 unsigned long debugreg6;

 unsigned long ptrace_dr7;

 unsigned long cr2;
 unsigned long trap_no;
 unsigned long error_code;

 unsigned long has_fpu;
 struct fpu fpu;


 struct vm86_struct *vm86_info;
 unsigned long screen_bitmap;
 unsigned long v86flags;
 unsigned long v86mask;
 unsigned long saved_sp0;
 unsigned int saved_fs;
 unsigned int saved_gs;


 unsigned long *io_bitmap_ptr;
 unsigned long iopl;

 unsigned io_bitmap_max;
};

static inline unsigned long native_get_debugreg(int regno)
{
 unsigned long val = 0;

 switch (regno) {
 case 0:
  asm("mov %%db0, %0" :"=r" (val));
  break;
 case 1:
  asm("mov %%db1, %0" :"=r" (val));
  break;
 case 2:
  asm("mov %%db2, %0" :"=r" (val));
  break;
 case 3:
  asm("mov %%db3, %0" :"=r" (val));
  break;
 case 6:
  asm("mov %%db6, %0" :"=r" (val));
  break;
 case 7:
  asm("mov %%db7, %0" :"=r" (val));
  break;
 default:
  do { asm volatile("1:\tud2\n" ".pushsection __bug_table,\"a\"\n" "2:\t.long 1b, %c0\n" "\t.word %c1, 0\n" "\t.org 2b+%c2\n" ".popsection" : : "i" ("/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"), "i" (502), "i" (sizeof(struct bug_entry))); __builtin_unreachable(); } while (0);
 }
 return val;
}

static inline void native_set_debugreg(int regno, unsigned long value)
{
 switch (regno) {
 case 0:
  asm("mov %0, %%db0" ::"r" (value));
  break;
 case 1:
  asm("mov %0, %%db1" ::"r" (value));
  break;
 case 2:
  asm("mov %0, %%db2" ::"r" (value));
  break;
 case 3:
  asm("mov %0, %%db3" ::"r" (value));
  break;
 case 6:
  asm("mov %0, %%db6" ::"r" (value));
  break;
 case 7:
  asm("mov %0, %%db7" ::"r" (value));
  break;
 default:
  do { asm volatile("1:\tud2\n" ".pushsection __bug_table,\"a\"\n" "2:\t.long 1b, %c0\n" "\t.word %c1, 0\n" "\t.org 2b+%c2\n" ".popsection" : : "i" ("/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"), "i" (529), "i" (sizeof(struct bug_entry))); __builtin_unreachable(); } while (0);
 }
}




static inline void native_set_iopl_mask(unsigned mask)
{

 unsigned int reg;

 asm volatile ("pushfl;"
        "popl %0;"
        "andl %1, %0;"
        "orl %2, %0;"
        "pushl %0;"
        "popfl"
        : "=&r" (reg)
        : "i" (~0x00003000), "r" (mask));

}

static inline void
native_load_sp0(struct tss_struct *tss, struct thread_struct *thread)
{
 tss->x86_tss.sp0 = thread->sp0;


 if (__builtin_expect(!!(tss->x86_tss.ss1 != thread->sysenter_cs), 0)) {
  tss->x86_tss.ss1 = thread->sysenter_cs;
  do { paravirt_write_msr(0x00000174, thread->sysenter_cs, 0); } while (0);
 }

}

static inline void native_swapgs(void)
{



}
# 601 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
extern unsigned long mmu_cr4_features;

static inline void set_in_cr4(unsigned long mask)
{
 unsigned long cr4;

 mmu_cr4_features |= mask;
 cr4 = read_cr4();
 cr4 |= mask;
 write_cr4(cr4);
}

static inline void clear_in_cr4(unsigned long mask)
{
 unsigned long cr4;

 mmu_cr4_features &= ~mask;
 cr4 = read_cr4();
 cr4 &= ~mask;
 write_cr4(cr4);
}

typedef struct {
 unsigned long seg;
} mm_segment_t;





extern int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);


extern void release_thread(struct task_struct *);


extern void prepare_to_copy(struct task_struct *tsk);

unsigned long get_wchan(struct task_struct *p);






static inline void cpuid(unsigned int op,
    unsigned int *eax, unsigned int *ebx,
    unsigned int *ecx, unsigned int *edx)
{
 *eax = op;
 *ecx = 0;
 __cpuid(eax, ebx, ecx, edx);
}


static inline void cpuid_count(unsigned int op, int count,
          unsigned int *eax, unsigned int *ebx,
          unsigned int *ecx, unsigned int *edx)
{
 *eax = op;
 *ecx = count;
 __cpuid(eax, ebx, ecx, edx);
}




static inline unsigned int cpuid_eax(unsigned int op)
{
 unsigned int eax, ebx, ecx, edx;

 cpuid(op, &eax, &ebx, &ecx, &edx);

 return eax;
}

static inline unsigned int cpuid_ebx(unsigned int op)
{
 unsigned int eax, ebx, ecx, edx;

 cpuid(op, &eax, &ebx, &ecx, &edx);

 return ebx;
}

static inline unsigned int cpuid_ecx(unsigned int op)
{
 unsigned int eax, ebx, ecx, edx;

 cpuid(op, &eax, &ebx, &ecx, &edx);

 return ecx;
}

static inline unsigned int cpuid_edx(unsigned int op)
{
 unsigned int eax, ebx, ecx, edx;

 cpuid(op, &eax, &ebx, &ecx, &edx);

 return edx;
}


static inline void rep_nop(void)
{
 asm volatile("rep; nop" ::: "memory");
}

static inline void cpu_relax(void)
{
 rep_nop();
}


static inline void sync_core(void)
{
 int tmp;
# 730 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
  asm volatile("cpuid" : "=a" (tmp) : "0" (1)
        : "ebx", "ecx", "edx", "memory");
}

static inline void __monitor(const void *eax, unsigned long ecx,
        unsigned long edx)
{

 asm volatile(".byte 0x0f, 0x01, 0xc8;"
       :: "a" (eax), "c" (ecx), "d"(edx));
}

static inline void __mwait(unsigned long eax, unsigned long ecx)
{

 asm volatile(".byte 0x0f, 0x01, 0xc9;"
       :: "a" (eax), "c" (ecx));
}

static inline void __sti_mwait(unsigned long eax, unsigned long ecx)
{
 do { } while (0);

 asm volatile("sti; .byte 0x0f, 0x01, 0xc9;"
       :: "a" (eax), "c" (ecx));
}

extern void select_idle_routine(const struct cpuinfo_x86 *c);
extern void init_amd_e400_c1e_mask(void);

extern unsigned long boot_option_idle_override;
extern bool amd_e400_c1e_detected;

enum idle_boot_override {IDLE_NO_OVERRIDE=0, IDLE_HALT, IDLE_NOMWAIT,
    IDLE_POLL, IDLE_FORCE_MWAIT};

extern void enable_sep_cpu(void);
extern int sysenter_setup(void);

extern void early_trap_init(void);


extern struct desc_ptr early_gdt_descr;

extern void cpu_set_gdt(int);
extern void switch_to_new_gdt(int);
extern void load_percpu_segment(int);
extern void cpu_init(void);

static inline unsigned long get_debugctlmsr(void)
{
 unsigned long debugctlmsr = 0;





 do { int _err; debugctlmsr = paravirt_read_msr(0x000001d9, &_err); } while (0);

 return debugctlmsr;
}

static inline void update_debugctlmsr(unsigned long debugctlmsr)
{




 do { paravirt_write_msr(0x000001d9, (u32)((u64)(debugctlmsr)), ((u64)(debugctlmsr))>>32); } while (0);
}





extern unsigned int machine_id;
extern unsigned int machine_submodel_id;
extern unsigned int BIOS_revision;


extern int bootloader_type;
extern int bootloader_version;

extern char ignore_fpu_irq;
# 832 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
static inline void prefetch(const void *x)
{
 asm volatile ("661:\n\t" ".byte " "0x8d,0x74,0x26,0x00" "\n" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(0*32+25)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "prefetchnta (%1)" "\n664:\n" ".previous" : : "i" (0), "r" (x))


             ;
}






static inline void prefetchw(const void *x)
{
 asm volatile ("661:\n\t" ".byte " "0x8d,0x74,0x26,0x00" "\n" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(1*32+31)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "prefetchw (%1)" "\n664:\n" ".previous" : : "i" (0), "r" (x))


             ;
}

static inline void spin_lock_prefetch(const void *x)
{
 prefetchw(x);
}
# 890 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
extern unsigned long thread_saved_pc(struct task_struct *tsk);
# 956 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
extern void start_thread(struct pt_regs *regs, unsigned long new_ip,
            unsigned long new_sp);
# 971 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h"
extern int get_tsc_mode(unsigned long adr);
extern int set_tsc_mode(unsigned int val);

extern int amd_get_nb_id(int cpu);

struct aperfmperf {
 u64 aperf, mperf;
};

static inline void get_aperfmperf(struct aperfmperf *am)
{
 ({ static bool __warned; int __ret_warn_once = !!(!(__builtin_constant_p((3*32+28)) && ( ((((3*32+28))>>5)==0 && (1UL<<(((3*32+28))&31) & ((1<<((0*32+ 0) & 31))|0|0|(1<<((0*32+ 6) & 31))| (1<<((0*32+ 8) & 31))|0|0|(1<<((0*32+15) & 31))| 0|0))) || ((((3*32+28))>>5)==1 && (1UL<<(((3*32+28))&31) & (0|0))) || ((((3*32+28))>>5)==2 && (1UL<<(((3*32+28))&31) & 0)) || ((((3*32+28))>>5)==3 && (1UL<<(((3*32+28))&31) & (0))) || ((((3*32+28))>>5)==4 && (1UL<<(((3*32+28))&31) & 0)) || ((((3*32+28))>>5)==5 && (1UL<<(((3*32+28))&31) & 0)) || ((((3*32+28))>>5)==6 && (1UL<<(((3*32+28))&31) & 0)) || ((((3*32+28))>>5)==7 && (1UL<<(((3*32+28))&31) & 0)) || ((((3*32+28))>>5)==8 && (1UL<<(((3*32+28))&31) & 0)) || ((((3*32+28))>>5)==9 && (1UL<<(((3*32+28))&31) & 0)) ) ? 1 : (__builtin_constant_p(((3*32+28))) ? constant_test_bit(((3*32+28)), ((unsigned long *)((&boot_cpu_data)->x86_capability))) : variable_test_bit(((3*32+28)), ((unsigned long *)((&boot_cpu_data)->x86_capability)))))); if (__builtin_expect(!!(__ret_warn_once), 0)) if (({ int __ret_warn_on = !!(!__warned); if (__builtin_expect(!!(__ret_warn_on), 0)) warn_slowpath_null("/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/processor.h", 982); __builtin_expect(!!(__ret_warn_on), 0); })) __warned = true; __builtin_expect(!!(__ret_warn_once), 0); });

 do { int _err; am->aperf = paravirt_read_msr(0x000000e8, &_err); } while (0);
 do { int _err; am->mperf = paravirt_read_msr(0x000000e7, &_err); } while (0);
}



static inline
unsigned long calc_aperfmperf_ratio(struct aperfmperf *old,
        struct aperfmperf *new)
{
 u64 aperf = new->aperf - old->aperf;
 u64 mperf = new->mperf - old->mperf;
 unsigned long ratio = aperf;

 mperf >>= 10;
 if (mperf)
  ratio = div64_u64(aperf, mperf);

 return ratio;
}





extern const int amd_erratum_383[];
extern const int amd_erratum_400[];
extern bool cpu_has_amd_erratum(const int *);
# 23 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h" 2
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ftrace.h" 1
# 36 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/ftrace.h"
extern void mcount(void);

static inline unsigned long ftrace_call_adjust(unsigned long addr)
{




 return addr;
}



struct dyn_arch_ftrace {

};
# 24 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h" 2
# 1 "include/linux/atomic.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h" 1
# 23 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_read(const atomic_t *v)
{
 return (*(volatile int *)&(v)->counter);
}
# 35 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline void atomic_set(atomic_t *v, int i)
{
 v->counter = i;
}
# 47 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline void atomic_add(int i, atomic_t *v)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "addl %1,%0"
       : "+m" (v->counter)
       : "ir" (i));
}
# 61 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline void atomic_sub(int i, atomic_t *v)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "subl %1,%0"
       : "+m" (v->counter)
       : "ir" (i));
}
# 77 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_sub_and_test(int i, atomic_t *v)
{
 unsigned char c;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "subl %2,%0; sete %1"
       : "+m" (v->counter), "=qm" (c)
       : "ir" (i) : "memory");
 return c;
}







static inline void atomic_inc(atomic_t *v)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "incl %0"
       : "+m" (v->counter));
}







static inline void atomic_dec(atomic_t *v)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "decl %0"
       : "+m" (v->counter));
}
# 119 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_dec_and_test(atomic_t *v)
{
 unsigned char c;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "decl %0; sete %1"
       : "+m" (v->counter), "=qm" (c)
       : : "memory");
 return c != 0;
}
# 137 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_inc_and_test(atomic_t *v)
{
 unsigned char c;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "incl %0; sete %1"
       : "+m" (v->counter), "=qm" (c)
       : : "memory");
 return c != 0;
}
# 156 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_add_negative(int i, atomic_t *v)
{
 unsigned char c;

 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "addl %2,%0; sets %1"
       : "+m" (v->counter), "=qm" (c)
       : "ir" (i) : "memory");
 return c;
}
# 173 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_add_return(int i, atomic_t *v)
{







 return i + ({ __typeof__ (*((&v->counter))) __ret = ((i)); switch (sizeof(*((&v->counter)))) { case 1: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddb %b0, %1\n" : "+r" (__ret), "+m" (*((&v->counter))) : : "memory", "cc"); break; case 2: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddw %w0, %1\n" : "+r" (__ret), "+m" (*((&v->counter))) : : "memory", "cc"); break; case 4: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddl %0, %1\n" : "+r" (__ret), "+m" (*((&v->counter))) : : "memory", "cc"); break; case -1: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddq %q0, %1\n" : "+r" (__ret), "+m" (*((&v->counter))) : : "memory", "cc"); break; default: __xadd_wrong_size(); } __ret; });
# 192 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
}
# 201 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_sub_return(int i, atomic_t *v)
{
 return atomic_add_return(-i, v);
}




static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
 return ({ __typeof__(*(((&v->counter)))) __ret; __typeof__(*(((&v->counter)))) __old = (((old))); __typeof__(*(((&v->counter)))) __new = (((new))); switch ((sizeof(*&v->counter))) { case 1: { volatile u8 *__ptr = (volatile u8 *)(((&v->counter))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgb %2,%1" : "=a" (__ret), "+m" (*__ptr) : "q" (__new), "0" (__old) : "memory"); break; } case 2: { volatile u16 *__ptr = (volatile u16 *)(((&v->counter))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgw %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } case 4: { volatile u32 *__ptr = (volatile u32 *)(((&v->counter))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgl %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } case -1: { volatile u64 *__ptr = (volatile u64 *)(((&v->counter))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgq %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } default: __cmpxchg_wrong_size(); } __ret; });
}

static inline int atomic_xchg(atomic_t *v, int new)
{
 return ({ __typeof(*((&v->counter))) __x = ((new)); switch (sizeof(*&v->counter)) { case 1: { volatile u8 *__ptr = (volatile u8 *)((&v->counter)); asm volatile("xchgb %0,%1" : "=q" (__x), "+m" (*__ptr) : "0" (__x) : "memory"); break; } case 2: { volatile u16 *__ptr = (volatile u16 *)((&v->counter)); asm volatile("xchgw %0,%1" : "=r" (__x), "+m" (*__ptr) : "0" (__x) : "memory"); break; } case 4: { volatile u32 *__ptr = (volatile u32 *)((&v->counter)); asm volatile("xchgl %0,%1" : "=r" (__x), "+m" (*__ptr) : "0" (__x) : "memory"); break; } case -1: { volatile u64 *__ptr = (volatile u64 *)((&v->counter)); asm volatile("xchgq %0,%1" : "=r" (__x), "+m" (*__ptr) : "0" (__x) : "memory"); break; } default: __xchg_wrong_size(); } __x; });
}
# 228 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
 int c, old;
 c = atomic_read(v);
 for (;;) {
  if (__builtin_expect(!!(c == (u)), 0))
   break;
  old = atomic_cmpxchg((v), c, c + (a));
  if (__builtin_expect(!!(old == c), 1))
   break;
  c = old;
 }
 return c;
}
# 251 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline int atomic_dec_if_positive(atomic_t *v)
{
 int c, old, dec;
 c = atomic_read(v);
 for (;;) {
  dec = c - 1;
  if (__builtin_expect(!!(dec < 0), 0))
   break;
  old = atomic_cmpxchg((v), c, dec);
  if (__builtin_expect(!!(old == c), 1))
   break;
  c = old;
 }
 return dec;
}
# 274 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
static inline short int atomic_inc_short(short int *v)
{
 asm(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "addw $1, %0" : "+m" (*v));
 return *v;
}
# 312 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
typedef struct {
 u64 __attribute__((aligned(8))) counter;
} atomic64_t;
# 35 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline long long atomic64_cmpxchg(atomic64_t *v, long long o, long long n)
{
 return ((__typeof__(*(&v->counter)))__cmpxchg64((&v->counter), (unsigned long long)(o), (unsigned long long)(n)));
}
# 48 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline long long atomic64_xchg(atomic64_t *v, long long n)
{
 long long o;
 unsigned high = (unsigned)(n >> 32);
 unsigned low = (unsigned)n;
 asm volatile("call atomic64_" "xchg" "_cx8"
       : "=A" (o), "+b" (low), "+c" (high)
       : "S" (v)
       : "memory"
       );
 return o;
}
# 68 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline void atomic64_set(atomic64_t *v, long long i)
{
 unsigned high = (unsigned)(i >> 32);
 unsigned low = (unsigned)i;
 asm volatile("call atomic64_" "set" "_cx8"
       : "+b" (low), "+c" (high)
       : "S" (v)
       : "eax", "edx", "memory"
       );
}







static inline long long atomic64_read(atomic64_t *v)
{
 long long r;
 asm volatile("call atomic64_" "read" "_cx8"
       : "=A" (r), "+c" (v)
       : : "memory"
       );
 return r;
 }
# 102 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline long long atomic64_add_return(long long i, atomic64_t *v)
{
 asm volatile("call atomic64_" "add_return" "_cx8"
       : "+A" (i), "+c" (v)
       : : "memory"
       );
 return i;
}




static inline long long atomic64_sub_return(long long i, atomic64_t *v)
{
 asm volatile("call atomic64_" "sub_return" "_cx8"
       : "+A" (i), "+c" (v)
       : : "memory"
       );
 return i;
}

static inline long long atomic64_inc_return(atomic64_t *v)
{
 long long a;
 asm volatile("call atomic64_" "inc_return" "_cx8"
       : "=A" (a)
       : "S" (v)
       : "memory", "ecx"
       );
 return a;
}

static inline long long atomic64_dec_return(atomic64_t *v)
{
 long long a;
 asm volatile("call atomic64_" "dec_return" "_cx8"
       : "=A" (a)
       : "S" (v)
       : "memory", "ecx"
       );
 return a;
}
# 152 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline long long atomic64_add(long long i, atomic64_t *v)
{
 asm volatile("call atomic64_" "add_return" "_cx8"
       : "+A" (i), "+c" (v)
       : : "memory"
       );
 return i;
}
# 168 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline long long atomic64_sub(long long i, atomic64_t *v)
{
 asm volatile("call atomic64_" "sub_return" "_cx8"
       : "+A" (i), "+c" (v)
       : : "memory"
       );
 return i;
}
# 186 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline int atomic64_sub_and_test(long long i, atomic64_t *v)
{
 return atomic64_sub_return(i, v) == 0;
}







static inline void atomic64_inc(atomic64_t *v)
{
 asm volatile("call atomic64_" "inc_return" "_cx8"
       : : "S" (v)
       : "memory", "eax", "ecx", "edx"
       );
}







static inline void atomic64_dec(atomic64_t *v)
{
 asm volatile("call atomic64_" "dec_return" "_cx8"
       : : "S" (v)
       : "memory", "eax", "ecx", "edx"
       );
}
# 227 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline int atomic64_dec_and_test(atomic64_t *v)
{
 return atomic64_dec_return(v) == 0;
}
# 240 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline int atomic64_inc_and_test(atomic64_t *v)
{
 return atomic64_inc_return(v) == 0;
}
# 254 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline int atomic64_add_negative(long long i, atomic64_t *v)
{
 return atomic64_add_return(i, v) < 0;
}
# 268 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic64_32.h"
static inline int atomic64_add_unless(atomic64_t *v, long long a, long long u)
{
 unsigned low = (unsigned)u;
 unsigned high = (unsigned)(u >> 32);
 asm volatile("call atomic64_" "add_unless" "_cx8" "\n\t"
       : "+A" (a), "+c" (v), "+S" (low), "+D" (high)
       : : "memory");
 return (int)a;
}


static inline int atomic64_inc_not_zero(atomic64_t *v)
{
 int r;
 asm volatile("call atomic64_" "inc_not_zero" "_cx8"
       : "=a" (r)
       : "S" (v)
       : "ecx", "edx", "memory"
       );
 return r;
}

static inline long long atomic64_dec_if_positive(atomic64_t *v)
{
 long long r;
 asm volatile("call atomic64_" "dec_if_positive" "_cx8"
       : "=A" (r)
       : "S" (v)
       : "ecx", "memory"
       );
 return r;
}
# 313 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/atomic.h" 2
# 5 "include/linux/atomic.h" 2
# 15 "include/linux/atomic.h"
static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
 return __atomic_add_unless(v, a, u) != u;
}
# 42 "include/linux/atomic.h"
static inline int atomic_inc_not_zero_hint(atomic_t *v, int hint)
{
 int val, c = hint;


 if (!hint)
  return atomic_add_unless((v), 1, 0);

 do {
  val = atomic_cmpxchg(v, c, c + 1);
  if (val == c)
   return 1;
  c = val;
 } while (c);

 return 0;
}



static inline int atomic_inc_unless_negative(atomic_t *p)
{
 int v, v1;
 for (v = 0; v >= 0; v = v1) {
  v1 = atomic_cmpxchg(p, v, v + 1);
  if (__builtin_expect(!!(v1 == v), 1))
   return 1;
 }
 return 0;
}



static inline int atomic_dec_unless_positive(atomic_t *p)
{
 int v, v1;
 for (v = 0; v <= 0; v = v1) {
  v1 = atomic_cmpxchg(p, v, v - 1);
  if (__builtin_expect(!!(v1 == v), 1))
   return 1;
 }
 return 0;
}



static inline void atomic_or(int i, atomic_t *v)
{
 int old;
 int new;

 do {
  old = atomic_read(v);
  new = old | i;
 } while (atomic_cmpxchg(v, old, new) != old);
}


# 1 "include/asm-generic/atomic-long.h" 1
# 141 "include/asm-generic/atomic-long.h"
typedef atomic_t atomic_long_t;


static inline long atomic_long_read(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return (long)atomic_read(v);
}

static inline void atomic_long_set(atomic_long_t *l, long i)
{
 atomic_t *v = (atomic_t *)l;

 atomic_set(v, i);
}

static inline void atomic_long_inc(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 atomic_inc(v);
}

static inline void atomic_long_dec(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 atomic_dec(v);
}

static inline void atomic_long_add(long i, atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 atomic_add(i, v);
}

static inline void atomic_long_sub(long i, atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 atomic_sub(i, v);
}

static inline int atomic_long_sub_and_test(long i, atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return atomic_sub_and_test(i, v);
}

static inline int atomic_long_dec_and_test(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return atomic_dec_and_test(v);
}

static inline int atomic_long_inc_and_test(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return atomic_inc_and_test(v);
}

static inline int atomic_long_add_negative(long i, atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return atomic_add_negative(i, v);
}

static inline long atomic_long_add_return(long i, atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return (long)atomic_add_return(i, v);
}

static inline long atomic_long_sub_return(long i, atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return (long)atomic_sub_return(i, v);
}

static inline long atomic_long_inc_return(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return (long)(atomic_add_return(1, v));
}

static inline long atomic_long_dec_return(atomic_long_t *l)
{
 atomic_t *v = (atomic_t *)l;

 return (long)(atomic_sub_return(1, v));
}

static inline long atomic_long_add_unless(atomic_long_t *l, long a, long u)
{
 atomic_t *v = (atomic_t *)l;

 return (long)atomic_add_unless(v, a, u);
}
# 101 "include/linux/atomic.h" 2
# 25 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h" 2

struct thread_info {
 struct task_struct *task;
 struct exec_domain *exec_domain;
 __u32 flags;
 __u32 status;
 __u32 cpu;
 int preempt_count;

 mm_segment_t addr_limit;
 struct restart_block restart_block;
 void *sysenter_return;

 unsigned long previous_esp;


 __u8 supervisor_stack[0];

 int uaccess_err;
};
# 185 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h"
register unsigned long current_stack_pointer asm("esp") __attribute__((__used__));


static inline struct thread_info *current_thread_info(void)
{
 return (struct thread_info *)
  (current_stack_pointer & ~((((1UL) << 12) << 1) - 1));
}
# 254 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/thread_info.h"
static inline void set_restore_sigmask(void)
{
 struct thread_info *ti = current_thread_info();
 ti->status |= 0x0008;
 set_bit(2, (unsigned long *)&ti->flags);
}



extern void arch_task_cache_init(void);
extern void free_thread_info(struct thread_info *ti);
extern int arch_dup_task_struct(struct task_struct *dst, struct task_struct *src);
# 54 "include/linux/thread_info.h" 2
# 62 "include/linux/thread_info.h"
static inline void set_ti_thread_flag(struct thread_info *ti, int flag)
{
 set_bit(flag, (unsigned long *)&ti->flags);
}

static inline void clear_ti_thread_flag(struct thread_info *ti, int flag)
{
 clear_bit(flag, (unsigned long *)&ti->flags);
}

static inline int test_and_set_ti_thread_flag(struct thread_info *ti, int flag)
{
 return test_and_set_bit(flag, (unsigned long *)&ti->flags);
}

static inline int test_and_clear_ti_thread_flag(struct thread_info *ti, int flag)
{
 return test_and_clear_bit(flag, (unsigned long *)&ti->flags);
}

static inline int test_ti_thread_flag(struct thread_info *ti, int flag)
{
 return (__builtin_constant_p((flag)) ? constant_test_bit((flag), ((unsigned long *)&ti->flags)) : variable_test_bit((flag), ((unsigned long *)&ti->flags)));
}
# 10 "include/linux/preempt.h" 2

# 1 "include/linux/list.h" 1





# 1 "include/linux/poison.h" 1
# 7 "include/linux/list.h" 2
# 24 "include/linux/list.h"
static inline void INIT_LIST_HEAD(struct list_head *list)
{
 list->next = list;
 list->prev = list;
}
# 37 "include/linux/list.h"
static inline void __list_add(struct list_head *new,
         struct list_head *prev,
         struct list_head *next)
{
 next->prev = new;
 new->next = next;
 new->prev = prev;
 prev->next = new;
}
# 60 "include/linux/list.h"
static inline void list_add(struct list_head *new, struct list_head *head)
{
 __list_add(new, head, head->next);
}
# 74 "include/linux/list.h"
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
 __list_add(new, head->prev, head);
}
# 86 "include/linux/list.h"
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
 next->prev = prev;
 prev->next = next;
}
# 99 "include/linux/list.h"
static inline void __list_del_entry(struct list_head *entry)
{
 __list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head *entry)
{
 __list_del(entry->prev, entry->next);
 entry->next = ((void *) 0x00100100 + (0x0UL));
 entry->prev = ((void *) 0x00200200 + (0x0UL));
}
# 122 "include/linux/list.h"
static inline void list_replace(struct list_head *old,
    struct list_head *new)
{
 new->next = old->next;
 new->next->prev = new;
 new->prev = old->prev;
 new->prev->next = new;
}

static inline void list_replace_init(struct list_head *old,
     struct list_head *new)
{
 list_replace(old, new);
 INIT_LIST_HEAD(old);
}





static inline void list_del_init(struct list_head *entry)
{
 __list_del_entry(entry);
 INIT_LIST_HEAD(entry);
}






static inline void list_move(struct list_head *list, struct list_head *head)
{
 __list_del_entry(list);
 list_add(list, head);
}






static inline void list_move_tail(struct list_head *list,
      struct list_head *head)
{
 __list_del_entry(list);
 list_add_tail(list, head);
}






static inline int list_is_last(const struct list_head *list,
    const struct list_head *head)
{
 return list->next == head;
}





static inline int list_empty(const struct list_head *head)
{
 return head->next == head;
}
# 204 "include/linux/list.h"
static inline int list_empty_careful(const struct list_head *head)
{
 struct list_head *next = head->next;
 return (next == head) && (next == head->prev);
}





static inline void list_rotate_left(struct list_head *head)
{
 struct list_head *first;

 if (!list_empty(head)) {
  first = head->next;
  list_move_tail(first, head);
 }
}





static inline int list_is_singular(const struct list_head *head)
{
 return !list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct list_head *list,
  struct list_head *head, struct list_head *entry)
{
 struct list_head *new_first = entry->next;
 list->next = head->next;
 list->next->prev = list;
 list->prev = entry;
 entry->next = list;
 head->next = new_first;
 new_first->prev = head;
}
# 259 "include/linux/list.h"
static inline void list_cut_position(struct list_head *list,
  struct list_head *head, struct list_head *entry)
{
 if (list_empty(head))
  return;
 if (list_is_singular(head) &&
  (head->next != entry && head != entry))
  return;
 if (entry == head)
  INIT_LIST_HEAD(list);
 else
  __list_cut_position(list, head, entry);
}

static inline void __list_splice(const struct list_head *list,
     struct list_head *prev,
     struct list_head *next)
{
 struct list_head *first = list->next;
 struct list_head *last = list->prev;

 first->prev = prev;
 prev->next = first;

 last->next = next;
 next->prev = last;
}






static inline void list_splice(const struct list_head *list,
    struct list_head *head)
{
 if (!list_empty(list))
  __list_splice(list, head, head->next);
}






static inline void list_splice_tail(struct list_head *list,
    struct list_head *head)
{
 if (!list_empty(list))
  __list_splice(list, head->prev, head);
}
# 318 "include/linux/list.h"
static inline void list_splice_init(struct list_head *list,
        struct list_head *head)
{
 if (!list_empty(list)) {
  __list_splice(list, head, head->next);
  INIT_LIST_HEAD(list);
 }
}
# 335 "include/linux/list.h"
static inline void list_splice_tail_init(struct list_head *list,
      struct list_head *head)
{
 if (!list_empty(list)) {
  __list_splice(list, head->prev, head);
  INIT_LIST_HEAD(list);
 }
}
# 570 "include/linux/list.h"
static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
 h->next = ((void *)0);
 h->pprev = ((void *)0);
}

static inline int hlist_unhashed(const struct hlist_node *h)
{
 return !h->pprev;
}

static inline int hlist_empty(const struct hlist_head *h)
{
 return !h->first;
}

static inline void __hlist_del(struct hlist_node *n)
{
 struct hlist_node *next = n->next;
 struct hlist_node **pprev = n->pprev;
 *pprev = next;
 if (next)
  next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
 __hlist_del(n);
 n->next = ((void *) 0x00100100 + (0x0UL));
 n->pprev = ((void *) 0x00200200 + (0x0UL));
}

static inline void hlist_del_init(struct hlist_node *n)
{
 if (!hlist_unhashed(n)) {
  __hlist_del(n);
  INIT_HLIST_NODE(n);
 }
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
 struct hlist_node *first = h->first;
 n->next = first;
 if (first)
  first->pprev = &n->next;
 h->first = n;
 n->pprev = &h->first;
}


static inline void hlist_add_before(struct hlist_node *n,
     struct hlist_node *next)
{
 n->pprev = next->pprev;
 n->next = next;
 next->pprev = &n->next;
 *(n->pprev) = n;
}

static inline void hlist_add_after(struct hlist_node *n,
     struct hlist_node *next)
{
 next->next = n->next;
 n->next = next;
 next->pprev = &n->next;

 if(next->next)
  next->next->pprev = &next->next;
}


static inline void hlist_add_fake(struct hlist_node *n)
{
 n->pprev = &n->next;
}





static inline void hlist_move_list(struct hlist_head *old,
       struct hlist_head *new)
{
 new->first = old->first;
 if (new->first)
  new->first->pprev = &new->first;
 old->first = ((void *)0);
}
# 12 "include/linux/preempt.h" 2
# 106 "include/linux/preempt.h"
struct preempt_notifier;
# 122 "include/linux/preempt.h"
struct preempt_ops {
 void (*sched_in)(struct preempt_notifier *notifier, int cpu);
 void (*sched_out)(struct preempt_notifier *notifier,
     struct task_struct *next);
};
# 135 "include/linux/preempt.h"
struct preempt_notifier {
 struct hlist_node link;
 struct preempt_ops *ops;
};

void preempt_notifier_register(struct preempt_notifier *notifier);
void preempt_notifier_unregister(struct preempt_notifier *notifier);

static inline void preempt_notifier_init(struct preempt_notifier *notifier,
         struct preempt_ops *ops)
{
 INIT_HLIST_NODE(&notifier->link);
 notifier->ops = ops;
}
# 51 "include/linux/spinlock.h" 2






# 1 "include/linux/bottom_half.h" 1



extern void local_bh_disable(void);
extern void _local_bh_enable(void);
extern void local_bh_enable(void);
extern void local_bh_enable_ip(unsigned long ip);
# 58 "include/linux/spinlock.h" 2
# 81 "include/linux/spinlock.h"
# 1 "include/linux/spinlock_types.h" 1
# 13 "include/linux/spinlock_types.h"
# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock_types.h" 1
# 11 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock_types.h"
typedef u8 __ticket_t;
typedef u16 __ticketpair_t;
# 21 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock_types.h"
typedef struct arch_spinlock {
 union {
  __ticketpair_t head_tail;
  struct __raw_tickets {
   __ticket_t head, tail;
  } tickets;
 };
} arch_spinlock_t;



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/rwlock.h" 1
# 9 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/rwlock.h"
typedef union {
 s32 lock;
 s32 write;
} arch_rwlock_t;
# 33 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock_types.h" 2
# 14 "include/linux/spinlock_types.h" 2




# 1 "include/linux/lockdep.h" 1
# 12 "include/linux/lockdep.h"
struct task_struct;
struct lockdep_map;


extern int prove_locking;
extern int lock_stat;
# 348 "include/linux/lockdep.h"
static inline void lockdep_off(void)
{
}

static inline void lockdep_on(void)
{
}
# 389 "include/linux/lockdep.h"
struct lock_class_key { };
# 441 "include/linux/lockdep.h"
static inline void print_irqtrace_events(struct task_struct *curr)
{
}
# 19 "include/linux/spinlock_types.h" 2

typedef struct raw_spinlock {
 arch_spinlock_t raw_lock;
# 32 "include/linux/spinlock_types.h"
} raw_spinlock_t;
# 64 "include/linux/spinlock_types.h"
typedef struct spinlock {
 union {
  struct raw_spinlock rlock;
# 75 "include/linux/spinlock_types.h"
 };
} spinlock_t;
# 86 "include/linux/spinlock_types.h"
# 1 "include/linux/rwlock_types.h" 1
# 11 "include/linux/rwlock_types.h"
typedef struct {
 arch_rwlock_t raw_lock;
# 23 "include/linux/rwlock_types.h"
} rwlock_t;
# 87 "include/linux/spinlock_types.h" 2
# 82 "include/linux/spinlock.h" 2





# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock.h" 1
# 53 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock.h"
static inline __attribute__((always_inline)) void __ticket_spin_lock(arch_spinlock_t *lock)
{
 register struct __raw_tickets inc = { .tail = 1 };

 inc = ({ __typeof__ (*((&lock->tickets))) __ret = ((inc)); switch (sizeof(*((&lock->tickets)))) { case 1: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddb %b0, %1\n" : "+r" (__ret), "+m" (*((&lock->tickets))) : : "memory", "cc"); break; case 2: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddw %w0, %1\n" : "+r" (__ret), "+m" (*((&lock->tickets))) : : "memory", "cc"); break; case 4: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddl %0, %1\n" : "+r" (__ret), "+m" (*((&lock->tickets))) : : "memory", "cc"); break; case -1: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddq %q0, %1\n" : "+r" (__ret), "+m" (*((&lock->tickets))) : : "memory", "cc"); break; default: __xadd_wrong_size(); } __ret; });

 for (;;) {
  if (inc.head == inc.tail)
   break;
  cpu_relax();
  inc.head = (*(volatile typeof(lock->tickets.head) *)&(lock->tickets.head));
 }
 __asm__ __volatile__("": : :"memory");
}

static inline __attribute__((always_inline)) int __ticket_spin_trylock(arch_spinlock_t *lock)
{
 arch_spinlock_t old, new;

 old.tickets = (*(volatile typeof(lock->tickets) *)&(lock->tickets));
 if (old.tickets.head != old.tickets.tail)
  return 0;

 new.head_tail = old.head_tail + (1 << (sizeof(__ticket_t) * 8));


 return ({ __typeof__(*(((&lock->head_tail)))) __ret; __typeof__(*(((&lock->head_tail)))) __old = (((old.head_tail))); __typeof__(*(((&lock->head_tail)))) __new = (((new.head_tail))); switch ((sizeof(*&lock->head_tail))) { case 1: { volatile u8 *__ptr = (volatile u8 *)(((&lock->head_tail))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgb %2,%1" : "=a" (__ret), "+m" (*__ptr) : "q" (__new), "0" (__old) : "memory"); break; } case 2: { volatile u16 *__ptr = (volatile u16 *)(((&lock->head_tail))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgw %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } case 4: { volatile u32 *__ptr = (volatile u32 *)(((&lock->head_tail))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgl %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } case -1: { volatile u64 *__ptr = (volatile u64 *)(((&lock->head_tail))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgq %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } default: __cmpxchg_wrong_size(); } __ret; }) == old.head_tail;
}


static inline __attribute__((always_inline)) void __ticket_spin_unlock(arch_spinlock_t *lock)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "incb %0"
       : "+m" (lock->head_tail)
       :
       : "memory", "cc");
}
# 100 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock.h"
static inline int __ticket_spin_is_locked(arch_spinlock_t *lock)
{
 struct __raw_tickets tmp = (*(volatile typeof(lock->tickets) *)&(lock->tickets));

 return !!(tmp.tail ^ tmp.head);
}

static inline int __ticket_spin_is_contended(arch_spinlock_t *lock)
{
 struct __raw_tickets tmp = (*(volatile typeof(lock->tickets) *)&(lock->tickets));

 return ((tmp.tail - tmp.head) & ((__ticket_t)((1 << (sizeof(__ticket_t) * 8)) - 1))) > 1;
}
# 150 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock.h"
static inline void arch_spin_unlock_wait(arch_spinlock_t *lock)
{
 while (arch_spin_is_locked(lock))
  cpu_relax();
}
# 174 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock.h"
static inline int arch_read_can_lock(arch_rwlock_t *lock)
{
 return lock->lock > 0;
}





static inline int arch_write_can_lock(arch_rwlock_t *lock)
{
 return lock->write == 0x00100000;
}

static inline void arch_read_lock(arch_rwlock_t *rw)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "decl" " " " (%0)\n\t"
       "jns 1f\n"
       "call __read_lock_failed\n\t"
       "1:\n"
       ::"a" (rw) : "memory");
}

static inline void arch_write_lock(arch_rwlock_t *rw)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "subl %1" "," "(%0)\n\t"
       "jz 1f\n"
       "call __write_lock_failed\n\t"
       "1:\n"
       ::"a" (&rw->write), "i" (0x00100000)
       : "memory");
}

static inline int arch_read_trylock(arch_rwlock_t *lock)
{
 atomic_t *count = (atomic_t *)lock;

 if ((atomic_sub_return(1, count)) >= 0)
  return 1;
 atomic_inc(count);
 return 0;
}

static inline int arch_write_trylock(arch_rwlock_t *lock)
{
 atomic_t *count = (atomic_t *)&lock->write;

 if (atomic_sub_and_test(0x00100000, count))
  return 1;
 atomic_add(0x00100000, count);
 return 0;
}

static inline void arch_read_unlock(arch_rwlock_t *rw)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "incl" " " " %0"
       :"+m" (rw->lock) : : "memory");
}

static inline void arch_write_unlock(arch_rwlock_t *rw)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "addl %1" "," "%0"
       : "+m" (rw->write) : "i" (0x00100000) : "memory");
}
# 253 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/spinlock.h"
static inline void smp_mb__after_lock(void) { }
# 88 "include/linux/spinlock.h" 2
# 137 "include/linux/spinlock.h"
static inline void do_raw_spin_lock(raw_spinlock_t *lock)
{
 (void)0;
 arch_spin_lock(&lock->raw_lock);
}

static inline void
do_raw_spin_lock_flags(raw_spinlock_t *lock, unsigned long *flags)
{
 (void)0;
 arch_spin_lock_flags(&lock->raw_lock, *flags);
}

static inline int do_raw_spin_trylock(raw_spinlock_t *lock)
{
 return arch_spin_trylock(&(lock)->raw_lock);
}

static inline void do_raw_spin_unlock(raw_spinlock_t *lock)
{
 arch_spin_unlock(&lock->raw_lock);
 (void)0;
}
# 257 "include/linux/spinlock.h"
# 1 "include/linux/rwlock.h" 1
# 258 "include/linux/spinlock.h" 2





# 1 "include/linux/spinlock_api_smp.h" 1
# 18 "include/linux/spinlock_api_smp.h"
int in_lock_functions(unsigned long addr);



void __attribute__((section(".spinlock.text"))) _raw_spin_lock(raw_spinlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_spin_lock_nested(raw_spinlock_t *lock, int subclass)
        ;
void __attribute__((section(".spinlock.text")))
_raw_spin_lock_nest_lock(raw_spinlock_t *lock, struct lockdep_map *map)
        ;
void __attribute__((section(".spinlock.text"))) _raw_spin_lock_bh(raw_spinlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_spin_lock_irq(raw_spinlock_t *lock)
        ;

unsigned long __attribute__((section(".spinlock.text"))) _raw_spin_lock_irqsave(raw_spinlock_t *lock)
        ;
unsigned long __attribute__((section(".spinlock.text")))
_raw_spin_lock_irqsave_nested(raw_spinlock_t *lock, int subclass)
        ;
int __attribute__((section(".spinlock.text"))) _raw_spin_trylock(raw_spinlock_t *lock);
int __attribute__((section(".spinlock.text"))) _raw_spin_trylock_bh(raw_spinlock_t *lock);
void __attribute__((section(".spinlock.text"))) _raw_spin_unlock(raw_spinlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_spin_unlock_bh(raw_spinlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_spin_unlock_irq(raw_spinlock_t *lock) ;
void __attribute__((section(".spinlock.text")))
_raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags)
        ;
# 86 "include/linux/spinlock_api_smp.h"
static inline int __raw_spin_trylock(raw_spinlock_t *lock)
{
 do { } while (0);
 if (do_raw_spin_trylock(lock)) {
  do { } while (0);
  return 1;
 }
 do { } while (0);
 return 0;
}
# 104 "include/linux/spinlock_api_smp.h"
static inline unsigned long __raw_spin_lock_irqsave(raw_spinlock_t *lock)
{
 unsigned long flags;

 do { do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); flags = arch_local_irq_save(); } while (0); do { } while (0); } while (0);
 do { } while (0);
 do { } while (0);
# 119 "include/linux/spinlock_api_smp.h"
 do_raw_spin_lock_flags(lock, &flags);

 return flags;
}

static inline void __raw_spin_lock_irq(raw_spinlock_t *lock)
{
 do { arch_local_irq_disable(); do { } while (0); } while (0);
 do { } while (0);
 do { } while (0);
 do_raw_spin_lock(lock);
}

static inline void __raw_spin_lock_bh(raw_spinlock_t *lock)
{
 local_bh_disable();
 do { } while (0);
 do { } while (0);
 do_raw_spin_lock(lock);
}

static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
 do { } while (0);
 do { } while (0);
 do_raw_spin_lock(lock);
}



static inline void __raw_spin_unlock(raw_spinlock_t *lock)
{
 do { } while (0);
 do_raw_spin_unlock(lock);
 do { } while (0);
}

static inline void __raw_spin_unlock_irqrestore(raw_spinlock_t *lock,
         unsigned long flags)
{
 do { } while (0);
 do_raw_spin_unlock(lock);
 do { if (({ ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_irqs_disabled_flags(flags); })) { do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_local_irq_restore(flags); } while (0); do { } while (0); } else { do { } while (0); do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_local_irq_restore(flags); } while (0); } } while (0);
 do { } while (0);
}

static inline void __raw_spin_unlock_irq(raw_spinlock_t *lock)
{
 do { } while (0);
 do_raw_spin_unlock(lock);
 do { do { } while (0); arch_local_irq_enable(); } while (0);
 do { } while (0);
}

static inline void __raw_spin_unlock_bh(raw_spinlock_t *lock)
{
 do { } while (0);
 do_raw_spin_unlock(lock);
 do { } while (0);
 local_bh_enable_ip((unsigned long)__builtin_return_address(0));
}

static inline int __raw_spin_trylock_bh(raw_spinlock_t *lock)
{
 local_bh_disable();
 do { } while (0);
 if (do_raw_spin_trylock(lock)) {
  do { } while (0);
  return 1;
 }
 do { } while (0);
 local_bh_enable_ip((unsigned long)__builtin_return_address(0));
 return 0;
}

# 1 "include/linux/rwlock_api_smp.h" 1
# 18 "include/linux/rwlock_api_smp.h"
void __attribute__((section(".spinlock.text"))) _raw_read_lock(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_write_lock(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_read_lock_bh(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_write_lock_bh(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_read_lock_irq(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_write_lock_irq(rwlock_t *lock) ;
unsigned long __attribute__((section(".spinlock.text"))) _raw_read_lock_irqsave(rwlock_t *lock)
       ;
unsigned long __attribute__((section(".spinlock.text"))) _raw_write_lock_irqsave(rwlock_t *lock)
       ;
int __attribute__((section(".spinlock.text"))) _raw_read_trylock(rwlock_t *lock);
int __attribute__((section(".spinlock.text"))) _raw_write_trylock(rwlock_t *lock);
void __attribute__((section(".spinlock.text"))) _raw_read_unlock(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_write_unlock(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_read_unlock_bh(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_write_unlock_bh(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_read_unlock_irq(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text"))) _raw_write_unlock_irq(rwlock_t *lock) ;
void __attribute__((section(".spinlock.text")))
_raw_read_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
       ;
void __attribute__((section(".spinlock.text")))
_raw_write_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
       ;
# 117 "include/linux/rwlock_api_smp.h"
static inline int __raw_read_trylock(rwlock_t *lock)
{
 do { } while (0);
 if (arch_read_trylock(&(lock)->raw_lock)) {
  do { } while (0);
  return 1;
 }
 do { } while (0);
 return 0;
}

static inline int __raw_write_trylock(rwlock_t *lock)
{
 do { } while (0);
 if (arch_write_trylock(&(lock)->raw_lock)) {
  do { } while (0);
  return 1;
 }
 do { } while (0);
 return 0;
}
# 146 "include/linux/rwlock_api_smp.h"
static inline void __raw_read_lock(rwlock_t *lock)
{
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_read_lock(&(lock)->raw_lock); } while (0);
}

static inline unsigned long __raw_read_lock_irqsave(rwlock_t *lock)
{
 unsigned long flags;

 do { do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); flags = arch_local_irq_save(); } while (0); do { } while (0); } while (0);
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_read_lock(&((lock))->raw_lock); } while (0)
                                       ;
 return flags;
}

static inline void __raw_read_lock_irq(rwlock_t *lock)
{
 do { arch_local_irq_disable(); do { } while (0); } while (0);
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_read_lock(&(lock)->raw_lock); } while (0);
}

static inline void __raw_read_lock_bh(rwlock_t *lock)
{
 local_bh_disable();
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_read_lock(&(lock)->raw_lock); } while (0);
}

static inline unsigned long __raw_write_lock_irqsave(rwlock_t *lock)
{
 unsigned long flags;

 do { do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); flags = arch_local_irq_save(); } while (0); do { } while (0); } while (0);
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_write_lock(&((lock))->raw_lock); } while (0)
                                        ;
 return flags;
}

static inline void __raw_write_lock_irq(rwlock_t *lock)
{
 do { arch_local_irq_disable(); do { } while (0); } while (0);
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_write_lock(&(lock)->raw_lock); } while (0);
}

static inline void __raw_write_lock_bh(rwlock_t *lock)
{
 local_bh_disable();
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_write_lock(&(lock)->raw_lock); } while (0);
}

static inline void __raw_write_lock(rwlock_t *lock)
{
 do { } while (0);
 do { } while (0);
 do {(void)0; arch_write_lock(&(lock)->raw_lock); } while (0);
}



static inline void __raw_write_unlock(rwlock_t *lock)
{
 do { } while (0);
 do {arch_write_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { } while (0);
}

static inline void __raw_read_unlock(rwlock_t *lock)
{
 do { } while (0);
 do {arch_read_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { } while (0);
}

static inline void
__raw_read_unlock_irqrestore(rwlock_t *lock, unsigned long flags)
{
 do { } while (0);
 do {arch_read_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { if (({ ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_irqs_disabled_flags(flags); })) { do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_local_irq_restore(flags); } while (0); do { } while (0); } else { do { } while (0); do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_local_irq_restore(flags); } while (0); } } while (0);
 do { } while (0);
}

static inline void __raw_read_unlock_irq(rwlock_t *lock)
{
 do { } while (0);
 do {arch_read_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { do { } while (0); arch_local_irq_enable(); } while (0);
 do { } while (0);
}

static inline void __raw_read_unlock_bh(rwlock_t *lock)
{
 do { } while (0);
 do {arch_read_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { } while (0);
 local_bh_enable_ip((unsigned long)__builtin_return_address(0));
}

static inline void __raw_write_unlock_irqrestore(rwlock_t *lock,
          unsigned long flags)
{
 do { } while (0);
 do {arch_write_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { if (({ ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_irqs_disabled_flags(flags); })) { do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_local_irq_restore(flags); } while (0); do { } while (0); } else { do { } while (0); do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); arch_local_irq_restore(flags); } while (0); } } while (0);
 do { } while (0);
}

static inline void __raw_write_unlock_irq(rwlock_t *lock)
{
 do { } while (0);
 do {arch_write_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { do { } while (0); arch_local_irq_enable(); } while (0);
 do { } while (0);
}

static inline void __raw_write_unlock_bh(rwlock_t *lock)
{
 do { } while (0);
 do {arch_write_unlock(&(lock)->raw_lock); (void)0; } while (0);
 do { } while (0);
 local_bh_enable_ip((unsigned long)__builtin_return_address(0));
}
# 195 "include/linux/spinlock_api_smp.h" 2
# 264 "include/linux/spinlock.h" 2
# 272 "include/linux/spinlock.h"
static inline raw_spinlock_t *spinlock_check(spinlock_t *lock)
{
 return &lock->rlock;
}







static inline void spin_lock(spinlock_t *lock)
{
 _raw_spin_lock(&lock->rlock);
}

static inline void spin_lock_bh(spinlock_t *lock)
{
 _raw_spin_lock_bh(&lock->rlock);
}

static inline int spin_trylock(spinlock_t *lock)
{
 return (_raw_spin_trylock(&lock->rlock));
}
# 308 "include/linux/spinlock.h"
static inline void spin_lock_irq(spinlock_t *lock)
{
 _raw_spin_lock_irq(&lock->rlock);
}
# 323 "include/linux/spinlock.h"
static inline void spin_unlock(spinlock_t *lock)
{
 __raw_spin_unlock(&lock->rlock);
}

static inline void spin_unlock_bh(spinlock_t *lock)
{
 _raw_spin_unlock_bh(&lock->rlock);
}

static inline void spin_unlock_irq(spinlock_t *lock)
{
 __raw_spin_unlock_irq(&lock->rlock);
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
 do { ({ unsigned long __dummy; typeof(flags) __dummy2; (void)(&__dummy == &__dummy2); 1; }); _raw_spin_unlock_irqrestore(&lock->rlock, flags); } while (0);
}

static inline int spin_trylock_bh(spinlock_t *lock)
{
 return (_raw_spin_trylock_bh(&lock->rlock));
}

static inline int spin_trylock_irq(spinlock_t *lock)
{
 return ({ do { arch_local_irq_disable(); do { } while (0); } while (0); (_raw_spin_trylock(&lock->rlock)) ? 1 : ({ do { do { } while (0); arch_local_irq_enable(); } while (0); 0; }); });
}






static inline void spin_unlock_wait(spinlock_t *lock)
{
 arch_spin_unlock_wait(&(&lock->rlock)->raw_lock);
}

static inline int spin_is_locked(spinlock_t *lock)
{
 return arch_spin_is_locked(&(&lock->rlock)->raw_lock);
}

static inline int spin_is_contended(spinlock_t *lock)
{
 return arch_spin_is_contended(&(&lock->rlock)->raw_lock);
}

static inline int spin_can_lock(spinlock_t *lock)
{
 return (!arch_spin_is_locked(&(&lock->rlock)->raw_lock));
}

static inline void assert_spin_locked(spinlock_t *lock)
{
 do { if (__builtin_expect(!!(!arch_spin_is_locked(&(&lock->rlock)->raw_lock)), 0)) do { asm volatile("1:\tud2\n" ".pushsection __bug_table,\"a\"\n" "2:\t.long 1b, %c0\n" "\t.word %c1, 0\n" "\t.org 2b+%c2\n" ".popsection" : : "i" ("include/linux/spinlock.h"), "i" (380), "i" (sizeof(struct bug_entry))); __builtin_unreachable(); } while (0); } while(0);
}
# 396 "include/linux/spinlock.h"
extern int _atomic_dec_and_lock(atomic_t *atomic, spinlock_t *lock);
# 8 "include/linux/mmzone.h" 2

# 1 "include/linux/wait.h" 1
# 28 "include/linux/wait.h"
typedef struct __wait_queue wait_queue_t;
typedef int (*wait_queue_func_t)(wait_queue_t *wait, unsigned mode, int flags, void *key);
int default_wake_function(wait_queue_t *wait, unsigned mode, int flags, void *key);

struct __wait_queue {
 unsigned int flags;

 void *private;
 wait_queue_func_t func;
 struct list_head task_list;
};

struct wait_bit_key {
 void *flags;
 int bit_nr;
};

struct wait_bit_queue {
 struct wait_bit_key key;
 wait_queue_t wait;
};

struct __wait_queue_head {
 spinlock_t lock;
 struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;

struct task_struct;
# 80 "include/linux/wait.h"
extern void __init_waitqueue_head(wait_queue_head_t *q, struct lock_class_key *);
# 98 "include/linux/wait.h"
static inline void init_waitqueue_entry(wait_queue_t *q, struct task_struct *p)
{
 q->flags = 0;
 q->private = p;
 q->func = default_wake_function;
}

static inline void init_waitqueue_func_entry(wait_queue_t *q,
     wait_queue_func_t func)
{
 q->flags = 0;
 q->private = ((void *)0);
 q->func = func;
}

static inline int waitqueue_active(wait_queue_head_t *q)
{
 return !list_empty(&q->task_list);
}

extern void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);
extern void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait);
extern void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);

static inline void __add_wait_queue(wait_queue_head_t *head, wait_queue_t *new)
{
 list_add(&new->task_list, &head->task_list);
}




static inline void __add_wait_queue_exclusive(wait_queue_head_t *q,
           wait_queue_t *wait)
{
 wait->flags |= 0x01;
 __add_wait_queue(q, wait);
}

static inline void __add_wait_queue_tail(wait_queue_head_t *head,
      wait_queue_t *new)
{
 list_add_tail(&new->task_list, &head->task_list);
}

static inline void __add_wait_queue_tail_exclusive(wait_queue_head_t *q,
           wait_queue_t *wait)
{
 wait->flags |= 0x01;
 __add_wait_queue_tail(q, wait);
}

static inline void __remove_wait_queue(wait_queue_head_t *head,
       wait_queue_t *old)
{
 list_del(&old->task_list);
}

void __wake_up(wait_queue_head_t *q, unsigned int mode, int nr, void *key);
void __wake_up_locked_key(wait_queue_head_t *q, unsigned int mode, void *key);
void __wake_up_sync_key(wait_queue_head_t *q, unsigned int mode, int nr,
   void *key);
void __wake_up_locked(wait_queue_head_t *q, unsigned int mode);
void __wake_up_sync(wait_queue_head_t *q, unsigned int mode, int nr);
void __wake_up_bit(wait_queue_head_t *, void *, int);
int __wait_on_bit(wait_queue_head_t *, struct wait_bit_queue *, int (*)(void *), unsigned);
int __wait_on_bit_lock(wait_queue_head_t *, struct wait_bit_queue *, int (*)(void *), unsigned);
void wake_up_bit(void *, int);
int out_of_line_wait_on_bit(void *, int, int (*)(void *), unsigned);
int out_of_line_wait_on_bit_lock(void *, int, int (*)(void *), unsigned);
wait_queue_head_t *bit_waitqueue(void *, int);
# 574 "include/linux/wait.h"
extern void sleep_on(wait_queue_head_t *q);
extern long sleep_on_timeout(wait_queue_head_t *q,
          signed long timeout);
extern void interruptible_sleep_on(wait_queue_head_t *q);
extern long interruptible_sleep_on_timeout(wait_queue_head_t *q,
        signed long timeout);




void prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state);
void prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *wait, int state);
void finish_wait(wait_queue_head_t *q, wait_queue_t *wait);
void abort_exclusive_wait(wait_queue_head_t *q, wait_queue_t *wait,
   unsigned int mode, void *key);
int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key);
int wake_bit_function(wait_queue_t *wait, unsigned mode, int sync, void *key);
# 634 "include/linux/wait.h"
static inline int wait_on_bit(void *word, int bit,
    int (*action)(void *), unsigned mode)
{
 if (!(__builtin_constant_p((bit)) ? constant_test_bit((bit), (word)) : variable_test_bit((bit), (word))))
  return 0;
 return out_of_line_wait_on_bit(word, bit, action, mode);
}
# 658 "include/linux/wait.h"
static inline int wait_on_bit_lock(void *word, int bit,
    int (*action)(void *), unsigned mode)
{
 if (!test_and_set_bit(bit, word))
  return 0;
 return out_of_line_wait_on_bit_lock(word, bit, action, mode);
}
# 10 "include/linux/mmzone.h" 2



# 1 "include/linux/numa.h" 1
# 14 "include/linux/mmzone.h" 2

# 1 "include/linux/seqlock.h" 1
# 33 "include/linux/seqlock.h"
typedef struct {
 unsigned sequence;
 spinlock_t lock;
} seqlock_t;
# 58 "include/linux/seqlock.h"
static inline void write_seqlock(seqlock_t *sl)
{
 spin_lock(&sl->lock);
 ++sl->sequence;
 __asm__ __volatile__("": : :"memory");
}

static inline void write_sequnlock(seqlock_t *sl)
{
 __asm__ __volatile__("": : :"memory");
 sl->sequence++;
 spin_unlock(&sl->lock);
}

static inline int write_tryseqlock(seqlock_t *sl)
{
 int ret = spin_trylock(&sl->lock);

 if (ret) {
  ++sl->sequence;
  __asm__ __volatile__("": : :"memory");
 }
 return ret;
}


static inline __attribute__((always_inline)) unsigned read_seqbegin(const seqlock_t *sl)
{
 unsigned ret;

repeat:
 ret = (*(volatile typeof(sl->sequence) *)&(sl->sequence));
 if (__builtin_expect(!!(ret & 1), 0)) {
  cpu_relax();
  goto repeat;
 }
 asm volatile ("661:\n\t" "lock; addl $0,0(%%esp)" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(0*32+26)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "lfence" "\n664:\n" ".previous" : : : "memory");

 return ret;
}






static inline __attribute__((always_inline)) int read_seqretry(const seqlock_t *sl, unsigned start)
{
 asm volatile ("661:\n\t" "lock; addl $0,0(%%esp)" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(0*32+26)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "lfence" "\n664:\n" ".previous" : : : "memory");

 return __builtin_expect(!!(sl->sequence != start), 0);
}
# 119 "include/linux/seqlock.h"
typedef struct seqcount {
 unsigned sequence;
} seqcount_t;
# 139 "include/linux/seqlock.h"
static inline unsigned __read_seqcount_begin(const seqcount_t *s)
{
 unsigned ret;

repeat:
 ret = (*(volatile typeof(s->sequence) *)&(s->sequence));
 if (__builtin_expect(!!(ret & 1), 0)) {
  cpu_relax();
  goto repeat;
 }
 return ret;
}
# 161 "include/linux/seqlock.h"
static inline unsigned read_seqcount_begin(const seqcount_t *s)
{
 unsigned ret = __read_seqcount_begin(s);
 asm volatile ("661:\n\t" "lock; addl $0,0(%%esp)" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(0*32+26)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "lfence" "\n664:\n" ".previous" : : : "memory");
 return ret;
}
# 182 "include/linux/seqlock.h"
static inline int __read_seqcount_retry(const seqcount_t *s, unsigned start)
{
 return __builtin_expect(!!(s->sequence != start), 0);
}
# 197 "include/linux/seqlock.h"
static inline int read_seqcount_retry(const seqcount_t *s, unsigned start)
{
 asm volatile ("661:\n\t" "lock; addl $0,0(%%esp)" "\n662:\n" ".section .altinstructions,\"a\"\n" "	 .long 661b - .\n" "	 .long 663f - .\n" "	 .word " "(0*32+26)" "\n" "	 .byte 662b-661b\n" "	 .byte 664f-663f\n" ".previous\n" ".section .discard,\"aw\",@progbits\n" "	 .byte 0xff + (664f-663f) - (662b-661b)\n" ".previous\n" ".section .altinstr_replacement, \"ax\"\n" "663:\n\t" "lfence" "\n664:\n" ".previous" : : : "memory");

 return __read_seqcount_retry(s, start);
}






static inline void write_seqcount_begin(seqcount_t *s)
{
 s->sequence++;
 __asm__ __volatile__("": : :"memory");
}

static inline void write_seqcount_end(seqcount_t *s)
{
 __asm__ __volatile__("": : :"memory");
 s->sequence++;
}
# 228 "include/linux/seqlock.h"
static inline void write_seqcount_barrier(seqcount_t *s)
{
 __asm__ __volatile__("": : :"memory");
 s->sequence+=2;
}
# 16 "include/linux/mmzone.h" 2
# 1 "include/linux/nodemask.h" 1
# 98 "include/linux/nodemask.h"
typedef struct { unsigned long bits[((((1 << 0)) + (8 * sizeof(long)) - 1) / (8 * sizeof(long)))]; } nodemask_t;
extern nodemask_t _unused_nodemask_arg_;


static inline void __node_set(int node, volatile nodemask_t *dstp)
{
 set_bit(node, dstp->bits);
}


static inline void __node_clear(int node, volatile nodemask_t *dstp)
{
 clear_bit(node, dstp->bits);
}


static inline void __nodes_setall(nodemask_t *dstp, int nbits)
{
 bitmap_fill(dstp->bits, nbits);
}


static inline void __nodes_clear(nodemask_t *dstp, int nbits)
{
 bitmap_zero(dstp->bits, nbits);
}






static inline int __node_test_and_set(int node, nodemask_t *addr)
{
 return test_and_set_bit(node, addr->bits);
}



static inline void __nodes_and(nodemask_t *dstp, const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 bitmap_and(dstp->bits, src1p->bits, src2p->bits, nbits);
}



static inline void __nodes_or(nodemask_t *dstp, const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 bitmap_or(dstp->bits, src1p->bits, src2p->bits, nbits);
}



static inline void __nodes_xor(nodemask_t *dstp, const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 bitmap_xor(dstp->bits, src1p->bits, src2p->bits, nbits);
}



static inline void __nodes_andnot(nodemask_t *dstp, const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 bitmap_andnot(dstp->bits, src1p->bits, src2p->bits, nbits);
}



static inline void __nodes_complement(nodemask_t *dstp,
     const nodemask_t *srcp, int nbits)
{
 bitmap_complement(dstp->bits, srcp->bits, nbits);
}



static inline int __nodes_equal(const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 return bitmap_equal(src1p->bits, src2p->bits, nbits);
}



static inline int __nodes_intersects(const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 return bitmap_intersects(src1p->bits, src2p->bits, nbits);
}



static inline int __nodes_subset(const nodemask_t *src1p,
     const nodemask_t *src2p, int nbits)
{
 return bitmap_subset(src1p->bits, src2p->bits, nbits);
}


static inline int __nodes_empty(const nodemask_t *srcp, int nbits)
{
 return bitmap_empty(srcp->bits, nbits);
}


static inline int __nodes_full(const nodemask_t *srcp, int nbits)
{
 return bitmap_full(srcp->bits, nbits);
}


static inline int __nodes_weight(const nodemask_t *srcp, int nbits)
{
 return bitmap_weight(srcp->bits, nbits);
}



static inline void __nodes_shift_right(nodemask_t *dstp,
     const nodemask_t *srcp, int n, int nbits)
{
 bitmap_shift_right(dstp->bits, srcp->bits, n, nbits);
}



static inline void __nodes_shift_left(nodemask_t *dstp,
     const nodemask_t *srcp, int n, int nbits)
{
 bitmap_shift_left(dstp->bits, srcp->bits, n, nbits);
}





static inline int __first_node(const nodemask_t *srcp)
{
 return ({ int __min1 = ((1 << 0)); int __min2 = (find_first_bit(srcp->bits, (1 << 0))); __min1 < __min2 ? __min1: __min2; });
}


static inline int __next_node(int n, const nodemask_t *srcp)
{
 return ({ int __min1 = ((1 << 0)); int __min2 = (find_next_bit(srcp->bits, (1 << 0), n+1)); __min1 < __min2 ? __min1: __min2; });
}

static inline void init_nodemask_of_node(nodemask_t *mask, int node)
{
 __nodes_clear(&(*mask), (1 << 0));
 __node_set((node), &(*mask));
}
# 266 "include/linux/nodemask.h"
static inline int __first_unset_node(const nodemask_t *maskp)
{
 return ({ int __min1 = ((1 << 0)); int __min2 = (find_first_zero_bit(maskp->bits, (1 << 0))); __min1 < __min2 ? __min1: __min2; })
                                                  ;
}
# 300 "include/linux/nodemask.h"
static inline int __nodemask_scnprintf(char *buf, int len,
     const nodemask_t *srcp, int nbits)
{
 return bitmap_scnprintf(buf, len, srcp->bits, nbits);
}



static inline int __nodemask_parse_user(const char *buf, int len,
     nodemask_t *dstp, int nbits)
{
 return bitmap_parse_user(buf, len, dstp->bits, nbits);
}



static inline int __nodelist_scnprintf(char *buf, int len,
     const nodemask_t *srcp, int nbits)
{
 return bitmap_scnlistprintf(buf, len, srcp->bits, nbits);
}


static inline int __nodelist_parse(const char *buf, nodemask_t *dstp, int nbits)
{
 return bitmap_parselist(buf, dstp->bits, nbits);
}



static inline int __node_remap(int oldbit,
  const nodemask_t *oldp, const nodemask_t *newp, int nbits)
{
 return bitmap_bitremap(oldbit, oldp->bits, newp->bits, nbits);
}



static inline void __nodes_remap(nodemask_t *dstp, const nodemask_t *srcp,
  const nodemask_t *oldp, const nodemask_t *newp, int nbits)
{
 bitmap_remap(dstp->bits, srcp->bits, oldp->bits, newp->bits, nbits);
}



static inline void __nodes_onto(nodemask_t *dstp, const nodemask_t *origp,
  const nodemask_t *relmapp, int nbits)
{
 bitmap_onto(dstp->bits, origp->bits, relmapp->bits, nbits);
}



static inline void __nodes_fold(nodemask_t *dstp, const nodemask_t *origp,
  int sz, int nbits)
{
 bitmap_fold(dstp->bits, origp->bits, sz, nbits);
}
# 374 "include/linux/nodemask.h"
enum node_states {
 N_POSSIBLE,
 N_ONLINE,
 N_NORMAL_MEMORY,

 N_HIGH_MEMORY,



 N_CPU,
 NR_NODE_STATES
};






extern nodemask_t node_states[NR_NODE_STATES];
# 438 "include/linux/nodemask.h"
static inline int node_state(int node, enum node_states state)
{
 return node == 0;
}

static inline void node_set_state(int node, enum node_states state)
{
}

static inline void node_clear_state(int node, enum node_states state)
{
}

static inline int num_node_state(enum node_states state)
{
 return 1;
}
# 472 "include/linux/nodemask.h"
static inline int node_random(const nodemask_t *mask)
{
 return 0;
}
# 504 "include/linux/nodemask.h"
struct nodemask_scratch {
 nodemask_t mask1;
 nodemask_t mask2;
};
# 17 "include/linux/mmzone.h" 2
# 1 "include/linux/pageblock-flags.h" 1
# 29 "include/linux/pageblock-flags.h"
enum pageblock_bits {
 PB_migrate,
 PB_migrate_end = PB_migrate + 3 - 1,

 NR_PAGEBLOCK_BITS
};
# 60 "include/linux/pageblock-flags.h"
struct page;


unsigned long get_pageblock_flags_group(struct page *page,
     int start_bitidx, int end_bitidx);
void set_pageblock_flags_group(struct page *page, unsigned long flags,
     int start_bitidx, int end_bitidx);
# 18 "include/linux/mmzone.h" 2
# 1 "include/generated/bounds.h" 1
# 19 "include/linux/mmzone.h" 2
# 50 "include/linux/mmzone.h"
extern int page_group_by_mobility_disabled;

static inline int get_pageblock_migratetype(struct page *page)
{
 return get_pageblock_flags_group(page, PB_migrate, PB_migrate_end);
}

struct free_area {
 struct list_head free_list[5];
 unsigned long nr_free;
};

struct pglist_data;
# 71 "include/linux/mmzone.h"
struct zone_padding {
 char x[0];
} __attribute__((__aligned__(1 << (6))));





enum zone_stat_item {

 NR_FREE_PAGES,
 NR_LRU_BASE,
 NR_INACTIVE_ANON = NR_LRU_BASE,
 NR_ACTIVE_ANON,
 NR_INACTIVE_FILE,
 NR_ACTIVE_FILE,
 NR_UNEVICTABLE,
 NR_MLOCK,
 NR_ANON_PAGES,
 NR_FILE_MAPPED,

 NR_FILE_PAGES,
 NR_FILE_DIRTY,
 NR_WRITEBACK,
 NR_SLAB_RECLAIMABLE,
 NR_SLAB_UNRECLAIMABLE,
 NR_PAGETABLE,
 NR_KERNEL_STACK,

 NR_UNSTABLE_NFS,
 NR_BOUNCE,
 NR_VMSCAN_WRITE,
 NR_VMSCAN_IMMEDIATE,
 NR_WRITEBACK_TEMP,
 NR_ISOLATED_ANON,
 NR_ISOLATED_FILE,
 NR_SHMEM,
 NR_DIRTIED,
 NR_WRITTEN,
# 118 "include/linux/mmzone.h"
 NR_ANON_TRANSPARENT_HUGEPAGES,
 NR_VM_ZONE_STAT_ITEMS };
# 134 "include/linux/mmzone.h"
enum lru_list {
 LRU_INACTIVE_ANON = 0,
 LRU_ACTIVE_ANON = 0 + 1,
 LRU_INACTIVE_FILE = 0 + 2,
 LRU_ACTIVE_FILE = 0 + 2 + 1,
 LRU_UNEVICTABLE,
 NR_LRU_LISTS
};





static inline int is_file_lru(enum lru_list l)
{
 return (l == LRU_INACTIVE_FILE || l == LRU_ACTIVE_FILE);
}

static inline int is_active_lru(enum lru_list l)
{
 return (l == LRU_ACTIVE_ANON || l == LRU_ACTIVE_FILE);
}

static inline int is_unevictable_lru(enum lru_list l)
{
 return (l == LRU_UNEVICTABLE);
}
# 178 "include/linux/mmzone.h"
typedef unsigned isolate_mode_t;

enum zone_watermarks {
 WMARK_MIN,
 WMARK_LOW,
 WMARK_HIGH,
 NR_WMARK
};





struct per_cpu_pages {
 int count;
 int high;
 int batch;


 struct list_head lists[3];
};

struct per_cpu_pageset {
 struct per_cpu_pages pcp;




 s8 stat_threshold;
 s8 vm_stat_diff[NR_VM_ZONE_STAT_ITEMS];

};



enum zone_type {
# 233 "include/linux/mmzone.h"
 ZONE_DMA,
# 248 "include/linux/mmzone.h"
 ZONE_NORMAL,
# 258 "include/linux/mmzone.h"
 ZONE_HIGHMEM,

 ZONE_MOVABLE,
 __MAX_NR_ZONES
};
# 284 "include/linux/mmzone.h"
struct zone_reclaim_stat {
# 293 "include/linux/mmzone.h"
 unsigned long recent_rotated[2];
 unsigned long recent_scanned[2];
};

struct zone {



 unsigned long watermark[NR_WMARK];






 unsigned long percpu_drift_mark;
# 318 "include/linux/mmzone.h"
 unsigned long lowmem_reserve[4];
# 328 "include/linux/mmzone.h"
 struct per_cpu_pageset *pageset;



 spinlock_t lock;
 int all_unreclaimable;




 struct free_area free_area[11];






 unsigned long *pageblock_flags;
# 354 "include/linux/mmzone.h"
 unsigned int compact_considered;
 unsigned int compact_defer_shift;


 struct zone_padding _pad1_;


 spinlock_t lru_lock;
 struct zone_lru {
  struct list_head list;
 } lru[NR_LRU_LISTS];

 struct zone_reclaim_stat reclaim_stat;

 unsigned long pages_scanned;
 unsigned long flags;


 atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];





 unsigned int inactive_ratio;


 struct zone_padding _pad2_;
# 408 "include/linux/mmzone.h"
 wait_queue_head_t * wait_table;
 unsigned long wait_table_hash_nr_entries;
 unsigned long wait_table_bits;




 struct pglist_data *zone_pgdat;

 unsigned long zone_start_pfn;
# 429 "include/linux/mmzone.h"
 unsigned long spanned_pages;
 unsigned long present_pages;




 const char *name;
} __attribute__((__aligned__(1 << (6))));

typedef enum {
 ZONE_RECLAIM_LOCKED,
 ZONE_OOM_LOCKED,
 ZONE_CONGESTED,


} zone_flags_t;

static inline void zone_set_flag(struct zone *zone, zone_flags_t flag)
{
 set_bit(flag, &zone->flags);
}

static inline int zone_test_and_set_flag(struct zone *zone, zone_flags_t flag)
{
 return test_and_set_bit(flag, &zone->flags);
}

static inline void zone_clear_flag(struct zone *zone, zone_flags_t flag)
{
 clear_bit(flag, &zone->flags);
}

static inline int zone_is_reclaim_congested(const struct zone *zone)
{
 return (__builtin_constant_p((ZONE_CONGESTED)) ? constant_test_bit((ZONE_CONGESTED), (&zone->flags)) : variable_test_bit((ZONE_CONGESTED), (&zone->flags)));
}

static inline int zone_is_reclaim_locked(const struct zone *zone)
{
 return (__builtin_constant_p((ZONE_RECLAIM_LOCKED)) ? constant_test_bit((ZONE_RECLAIM_LOCKED), (&zone->flags)) : variable_test_bit((ZONE_RECLAIM_LOCKED), (&zone->flags)));
}

static inline int zone_is_oom_locked(const struct zone *zone)
{
 return (__builtin_constant_p((ZONE_OOM_LOCKED)) ? constant_test_bit((ZONE_OOM_LOCKED), (&zone->flags)) : variable_test_bit((ZONE_OOM_LOCKED), (&zone->flags)));
}
# 564 "include/linux/mmzone.h"
struct zonelist_cache;






struct zoneref {
 struct zone *zone;
 int zone_idx;
};
# 593 "include/linux/mmzone.h"
struct zonelist {
 struct zonelist_cache *zlcache_ptr;
 struct zoneref _zonerefs[((1 << 0) * 4) + 1];



};


struct node_active_region {
 unsigned long start_pfn;
 unsigned long end_pfn;
 int nid;
};




extern struct page *mem_map;
# 625 "include/linux/mmzone.h"
struct bootmem_data;
typedef struct pglist_data {
 struct zone node_zones[4];
 struct zonelist node_zonelists[1];
 int nr_zones;

 struct page *node_mem_map;

 struct page_cgroup *node_page_cgroup;
# 649 "include/linux/mmzone.h"
 unsigned long node_start_pfn;
 unsigned long node_present_pages;
 unsigned long node_spanned_pages;

 int node_id;
 wait_queue_head_t kswapd_wait;
 struct task_struct *kswapd;
 int kswapd_max_order;
 enum zone_type classzone_idx;
} pg_data_t;
# 676 "include/linux/mmzone.h"
# 1 "include/linux/memory_hotplug.h" 1



# 1 "include/linux/mmzone.h" 1
# 5 "include/linux/memory_hotplug.h" 2

# 1 "include/linux/notifier.h" 1
# 12 "include/linux/notifier.h"
# 1 "include/linux/errno.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/errno.h" 1
# 5 "include/linux/errno.h" 2
# 13 "include/linux/notifier.h" 2
# 1 "include/linux/mutex.h" 1
# 48 "include/linux/mutex.h"
struct mutex {

 atomic_t count;
 spinlock_t wait_lock;
 struct list_head wait_list;

 struct task_struct *owner;
# 63 "include/linux/mutex.h"
};





struct mutex_waiter {
 struct list_head list;
 struct task_struct *task;



};
# 95 "include/linux/mutex.h"
static inline void mutex_destroy(struct mutex *lock) {}
# 115 "include/linux/mutex.h"
extern void __mutex_init(struct mutex *lock, const char *name,
    struct lock_class_key *key);







static inline int mutex_is_locked(struct mutex *lock)
{
 return atomic_read(&lock->count) != 1;
}
# 152 "include/linux/mutex.h"
extern void mutex_lock(struct mutex *lock);
extern int mutex_lock_interruptible(struct mutex *lock);
extern int mutex_lock_killable(struct mutex *lock);
# 168 "include/linux/mutex.h"
extern int mutex_trylock(struct mutex *lock);
extern void mutex_unlock(struct mutex *lock);
extern int atomic_dec_and_mutex_lock(atomic_t *cnt, struct mutex *lock);
# 14 "include/linux/notifier.h" 2
# 1 "include/linux/rwsem.h" 1
# 20 "include/linux/rwsem.h"
struct rw_semaphore;





struct rw_semaphore {
 long count;
 raw_spinlock_t wait_lock;
 struct list_head wait_list;



};

extern struct rw_semaphore *rwsem_down_read_failed(struct rw_semaphore *sem);
extern struct rw_semaphore *rwsem_down_write_failed(struct rw_semaphore *sem);
extern struct rw_semaphore *rwsem_wake(struct rw_semaphore *);
extern struct rw_semaphore *rwsem_downgrade_wake(struct rw_semaphore *sem);


# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/rwsem.h" 1
# 63 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/rwsem.h"
static inline void __down_read(struct rw_semaphore *sem)
{
 asm volatile("# beginning down_read\n\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "incl" " " "(%1)\n\t"

       "  jns        1f\n"
       "  call call_rwsem_down_read_failed\n"
       "1:\n\t"
       "# ending down_read\n\t"
       : "+m" (sem->count)
       : "a" (sem)
       : "memory", "cc");
}




static inline int __down_read_trylock(struct rw_semaphore *sem)
{
 long result, tmp;
 asm volatile("# beginning __down_read_trylock\n\t"
       "  mov          %0,%1\n\t"
       "1:\n\t"
       "  mov          %1,%2\n\t"
       "  add          %3,%2\n\t"
       "  jle	     2f\n\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "  cmpxchg  %2,%0\n\t"
       "  jnz	     1b\n\t"
       "2:\n\t"
       "# ending __down_read_trylock\n\t"
       : "+m" (sem->count), "=&a" (result), "=&r" (tmp)
       : "i" (0x00000001L)
       : "memory", "cc");
 return result >= 0 ? 1 : 0;
}




static inline void __down_write_nested(struct rw_semaphore *sem, int subclass)
{
 long tmp;
 asm volatile("# beginning down_write\n\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "  xadd      %1,(%2)\n\t"

       "  test      %1,%1\n\t"

       "  jz        1f\n"
       "  call call_rwsem_down_write_failed\n"
       "1:\n"
       "# ending down_write"
       : "+m" (sem->count), "=d" (tmp)
       : "a" (sem), "1" (((-0x0000ffffL -1) + 0x00000001L))
       : "memory", "cc");
}

static inline void __down_write(struct rw_semaphore *sem)
{
 __down_write_nested(sem, 0);
}




static inline int __down_write_trylock(struct rw_semaphore *sem)
{
 long ret = ({ __typeof__(*(((&sem->count)))) __ret; __typeof__(*(((&sem->count)))) __old = (((0x00000000L))); __typeof__(*(((&sem->count)))) __new = (((((-0x0000ffffL -1) + 0x00000001L)))); switch ((sizeof(*&sem->count))) { case 1: { volatile u8 *__ptr = (volatile u8 *)(((&sem->count))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgb %2,%1" : "=a" (__ret), "+m" (*__ptr) : "q" (__new), "0" (__old) : "memory"); break; } case 2: { volatile u16 *__ptr = (volatile u16 *)(((&sem->count))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgw %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } case 4: { volatile u32 *__ptr = (volatile u32 *)(((&sem->count))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgl %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } case -1: { volatile u64 *__ptr = (volatile u64 *)(((&sem->count))); asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "cmpxchgq %2,%1" : "=a" (__ret), "+m" (*__ptr) : "r" (__new), "0" (__old) : "memory"); break; } default: __cmpxchg_wrong_size(); } __ret; })
                              ;
 if (ret == 0x00000000L)
  return 1;
 return 0;
}




static inline void __up_read(struct rw_semaphore *sem)
{
 long tmp;
 asm volatile("# beginning __up_read\n\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "  xadd      %1,(%2)\n\t"

       "  jns        1f\n\t"
       "  call call_rwsem_wake\n"
       "1:\n"
       "# ending __up_read\n"
       : "+m" (sem->count), "=d" (tmp)
       : "a" (sem), "1" (-0x00000001L)
       : "memory", "cc");
}




static inline void __up_write(struct rw_semaphore *sem)
{
 long tmp;
 asm volatile("# beginning __up_write\n\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "  xadd      %1,(%2)\n\t"

       "  jns        1f\n\t"
       "  call call_rwsem_wake\n"
       "1:\n\t"
       "# ending __up_write\n"
       : "+m" (sem->count), "=d" (tmp)
       : "a" (sem), "1" (-((-0x0000ffffL -1) + 0x00000001L))
       : "memory", "cc");
}




static inline void __downgrade_write(struct rw_semaphore *sem)
{
 asm volatile("# beginning __downgrade_write\n\t"
       ".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "addl" " " "%2,(%1)\n\t"




       "  jns       1f\n\t"
       "  call call_rwsem_downgrade_wake\n"
       "1:\n\t"
       "# ending __downgrade_write\n"
       : "+m" (sem->count)
       : "a" (sem), "er" (-(-0x0000ffffL -1))
       : "memory", "cc");
}




static inline void rwsem_atomic_add(long delta, struct rw_semaphore *sem)
{
 asm volatile(".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " " " "addl" " " "%1,%0"
       : "+m" (sem->count)
       : "er" (delta));
}




static inline long rwsem_atomic_update(long delta, struct rw_semaphore *sem)
{
 return delta + ({ __typeof__ (*((&sem->count))) __ret = ((delta)); switch (sizeof(*((&sem->count)))) { case 1: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddb %b0, %1\n" : "+r" (__ret), "+m" (*((&sem->count))) : : "memory", "cc"); break; case 2: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddw %w0, %1\n" : "+r" (__ret), "+m" (*((&sem->count))) : : "memory", "cc"); break; case 4: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddl %0, %1\n" : "+r" (__ret), "+m" (*((&sem->count))) : : "memory", "cc"); break; case -1: asm volatile (".section .smp_locks,\"a\"\n" ".balign 4\n" ".long 671f - .\n" ".previous\n" "671:" "\n\tlock; " "xaddq %q0, %1\n" : "+r" (__ret), "+m" (*((&sem->count))) : : "memory", "cc"); break; default: __xadd_wrong_size(); } __ret; });
}
# 42 "include/linux/rwsem.h" 2


static inline int rwsem_is_locked(struct rw_semaphore *sem)
{
 return sem->count != 0;
}
# 68 "include/linux/rwsem.h"
extern void __init_rwsem(struct rw_semaphore *sem, const char *name,
    struct lock_class_key *key);
# 81 "include/linux/rwsem.h"
extern void down_read(struct rw_semaphore *sem);




extern int down_read_trylock(struct rw_semaphore *sem);




extern void down_write(struct rw_semaphore *sem);




extern int down_write_trylock(struct rw_semaphore *sem);




extern void up_read(struct rw_semaphore *sem);




extern void up_write(struct rw_semaphore *sem);




extern void downgrade_write(struct rw_semaphore *sem);
# 15 "include/linux/notifier.h" 2
# 1 "include/linux/srcu.h" 1
# 32 "include/linux/srcu.h"
struct srcu_struct_array {
 int c[2];
};

struct srcu_struct {
 int completed;
 struct srcu_struct_array *per_cpu_ref;
 struct mutex mutex;



};
# 70 "include/linux/srcu.h"
int init_srcu_struct(struct srcu_struct *sp);






void cleanup_srcu_struct(struct srcu_struct *sp);
int __srcu_read_lock(struct srcu_struct *sp) ;
void __srcu_read_unlock(struct srcu_struct *sp, int idx) ;
void synchronize_srcu(struct srcu_struct *sp);
void synchronize_srcu_expedited(struct srcu_struct *sp);
long srcu_batches_completed(struct srcu_struct *sp);
# 103 "include/linux/srcu.h"
static inline int srcu_read_lock_held(struct srcu_struct *sp)
{
 return 1;
}
# 149 "include/linux/srcu.h"
static inline int srcu_read_lock(struct srcu_struct *sp)
{
 int retval = __srcu_read_lock(sp);

 do { } while (0);
 return retval;
}
# 164 "include/linux/srcu.h"
static inline void srcu_read_unlock(struct srcu_struct *sp, int idx)

{
 do { } while (0);
 __srcu_read_unlock(sp, idx);
}
# 16 "include/linux/notifier.h" 2
# 50 "include/linux/notifier.h"
struct notifier_block {
 int (*notifier_call)(struct notifier_block *, unsigned long, void *);
 struct notifier_block *next;
 int priority;
};

struct atomic_notifier_head {
 spinlock_t lock;
 struct notifier_block *head;
};

struct blocking_notifier_head {
 struct rw_semaphore rwsem;
 struct notifier_block *head;
};

struct raw_notifier_head {
 struct notifier_block *head;
};

struct srcu_notifier_head {
 struct mutex mutex;
 struct srcu_struct srcu;
 struct notifier_block *head;
};
# 89 "include/linux/notifier.h"
extern void srcu_init_notifier_head(struct srcu_notifier_head *nh);
# 115 "include/linux/notifier.h"
extern int atomic_notifier_chain_register(struct atomic_notifier_head *nh,
  struct notifier_block *nb);
extern int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
  struct notifier_block *nb);
extern int raw_notifier_chain_register(struct raw_notifier_head *nh,
  struct notifier_block *nb);
extern int srcu_notifier_chain_register(struct srcu_notifier_head *nh,
  struct notifier_block *nb);

extern int blocking_notifier_chain_cond_register(
  struct blocking_notifier_head *nh,
  struct notifier_block *nb);

extern int atomic_notifier_chain_unregister(struct atomic_notifier_head *nh,
  struct notifier_block *nb);
extern int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
  struct notifier_block *nb);
extern int raw_notifier_chain_unregister(struct raw_notifier_head *nh,
  struct notifier_block *nb);
extern int srcu_notifier_chain_unregister(struct srcu_notifier_head *nh,
  struct notifier_block *nb);

extern int atomic_notifier_call_chain(struct atomic_notifier_head *nh,
  unsigned long val, void *v);
extern int __atomic_notifier_call_chain(struct atomic_notifier_head *nh,
 unsigned long val, void *v, int nr_to_call, int *nr_calls);
extern int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
  unsigned long val, void *v);
extern int __blocking_notifier_call_chain(struct blocking_notifier_head *nh,
 unsigned long val, void *v, int nr_to_call, int *nr_calls);
extern int raw_notifier_call_chain(struct raw_notifier_head *nh,
  unsigned long val, void *v);
extern int __raw_notifier_call_chain(struct raw_notifier_head *nh,
 unsigned long val, void *v, int nr_to_call, int *nr_calls);
extern int srcu_notifier_call_chain(struct srcu_notifier_head *nh,
  unsigned long val, void *v);
extern int __srcu_notifier_call_chain(struct srcu_notifier_head *nh,
 unsigned long val, void *v, int nr_to_call, int *nr_calls);
# 165 "include/linux/notifier.h"
static inline int notifier_from_errno(int err)
{
 if (err)
  return 0x8000 | (0x0001 - err);

 return 0x0001;
}


static inline int notifier_to_errno(int ret)
{
 ret &= ~0x8000;
 return ret > 0x0001 ? 0x0001 - ret : 0;
}
# 209 "include/linux/notifier.h"
extern struct blocking_notifier_head reboot_notifier_list;
# 7 "include/linux/memory_hotplug.h" 2

struct page;
struct zone;
struct pglist_data;
struct mem_section;
# 188 "include/linux/memory_hotplug.h"
static inline void pgdat_resize_lock(struct pglist_data *p, unsigned long *f) {}
static inline void pgdat_resize_unlock(struct pglist_data *p, unsigned long *f) {}
static inline void pgdat_resize_init(struct pglist_data *pgdat) {}

static inline unsigned zone_span_seqbegin(struct zone *zone)
{
 return 0;
}
static inline int zone_span_seqretry(struct zone *zone, unsigned iv)
{
 return 0;
}
static inline void zone_span_writelock(struct zone *zone) {}
static inline void zone_span_writeunlock(struct zone *zone) {}
static inline void zone_seqlock_init(struct zone *zone) {}

static inline int mhp_notimplemented(const char *func)
{
 printk("<4>" "%s() called, with CONFIG_MEMORY_HOTPLUG disabled\n", func);
 dump_stack();
 return -38;
}

static inline void register_page_bootmem_info_node(struct pglist_data *pgdat)
{
}

static inline void lock_memory_hotplug(void) {}
static inline void unlock_memory_hotplug(void) {}
# 225 "include/linux/memory_hotplug.h"
static inline int is_mem_section_removable(unsigned long pfn,
     unsigned long nr_pages)
{
 return 0;
}


extern int mem_online_node(int nid);
extern int add_memory(int nid, u64 start, u64 size);
extern int arch_add_memory(int nid, u64 start, u64 size);
extern int remove_memory(u64 start, u64 size);
extern int sparse_add_one_section(struct zone *zone, unsigned long start_pfn,
        int nr_pages);
extern void sparse_remove_one_section(struct zone *zone, struct mem_section *ms);
extern struct page *sparse_decode_mem_map(unsigned long coded_mem_map,
       unsigned long pnum);
# 677 "include/linux/mmzone.h" 2

extern struct mutex zonelists_mutex;
void build_all_zonelists(void *data);
void wakeup_kswapd(struct zone *zone, int order, enum zone_type classzone_idx);
bool zone_watermark_ok(struct zone *z, int order, unsigned long mark,
  int classzone_idx, int alloc_flags);
bool zone_watermark_ok_safe(struct zone *z, int order, unsigned long mark,
  int classzone_idx, int alloc_flags);
enum memmap_context {
 MEMMAP_EARLY,
 MEMMAP_HOTPLUG,
};
extern int init_currently_empty_zone(struct zone *zone, unsigned long start_pfn,
         unsigned long size,
         enum memmap_context context);




static inline void memory_present(int nid, unsigned long start, unsigned long end) {}





static inline int local_memory_node(int node_id) { return node_id; };
# 714 "include/linux/mmzone.h"
static inline int populated_zone(struct zone *zone)
{
 return (!!zone->present_pages);
}

extern int movable_zone;

static inline int zone_movable_is_highmem(void)
{

 return movable_zone == ZONE_HIGHMEM;



}

static inline int is_highmem_idx(enum zone_type idx)
{

 return (idx == ZONE_HIGHMEM ||
  (idx == ZONE_MOVABLE && zone_movable_is_highmem()));



}

static inline int is_normal_idx(enum zone_type idx)
{
 return (idx == ZONE_NORMAL);
}







static inline int is_highmem(struct zone *zone)
{

 int zone_off = (char *)zone - (char *)zone->zone_pgdat->node_zones;
 return zone_off == ZONE_HIGHMEM * sizeof(*zone) ||
        (zone_off == ZONE_MOVABLE * sizeof(*zone) &&
  zone_movable_is_highmem());



}

static inline int is_normal(struct zone *zone)
{
 return zone == zone->zone_pgdat->node_zones + ZONE_NORMAL;
}

static inline int is_dma32(struct zone *zone)
{



 return 0;

}

static inline int is_dma(struct zone *zone)
{

 return zone == zone->zone_pgdat->node_zones + ZONE_DMA;



}


struct ctl_table;
int min_free_kbytes_sysctl_handler(struct ctl_table *, int,
     void *, size_t *, loff_t *);
extern int sysctl_lowmem_reserve_ratio[4 -1];
int lowmem_reserve_ratio_sysctl_handler(struct ctl_table *, int,
     void *, size_t *, loff_t *);
int percpu_pagelist_fraction_sysctl_handler(struct ctl_table *, int,
     void *, size_t *, loff_t *);
int sysctl_min_unmapped_ratio_sysctl_handler(struct ctl_table *, int,
   void *, size_t *, loff_t *);
int sysctl_min_slab_ratio_sysctl_handler(struct ctl_table *, int,
   void *, size_t *, loff_t *);

extern int numa_zonelist_order_handler(struct ctl_table *, int,
   void *, size_t *, loff_t *);
extern char numa_zonelist_order[];




extern struct pglist_data contig_page_data;
# 817 "include/linux/mmzone.h"
extern struct pglist_data *first_online_pgdat(void);
extern struct pglist_data *next_online_pgdat(struct pglist_data *pgdat);
extern struct zone *next_zone(struct zone *zone);
# 849 "include/linux/mmzone.h"
static inline struct zone *zonelist_zone(struct zoneref *zoneref)
{
 return zoneref->zone;
}

static inline int zonelist_zone_idx(struct zoneref *zoneref)
{
 return zoneref->zone_idx;
}

static inline int zonelist_node_idx(struct zoneref *zoneref)
{




 return 0;

}
# 882 "include/linux/mmzone.h"
struct zoneref *next_zones_zonelist(struct zoneref *z,
     enum zone_type highest_zoneidx,
     nodemask_t *nodes,
     struct zone **zone);
# 899 "include/linux/mmzone.h"
static inline struct zoneref *first_zones_zonelist(struct zonelist *zonelist,
     enum zone_type highest_zoneidx,
     nodemask_t *nodes,
     struct zone **zone)
{
 return next_zones_zonelist(zonelist->_zonerefs, highest_zoneidx, nodes,
        zone);
}
# 1128 "include/linux/mmzone.h"
void memory_present(int nid, unsigned long start, unsigned long end);
unsigned long __attribute__ ((__section__(".init.text"))) __attribute__((__cold__)) __attribute__((no_instrument_function)) node_memmap_size_bytes(int, unsigned long, unsigned long);
# 1162 "include/linux/mmzone.h"
static inline int memmap_valid_within(unsigned long pfn,
     struct page *page, struct zone *zone)
{
 return 1;
}
# 2 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/vmtest_headers.c" 2
# 1 "include/linux/mm_types.h" 1



# 1 "include/linux/auxvec.h" 1



# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/auxvec.h" 1
# 5 "include/linux/auxvec.h" 2
# 5 "include/linux/mm_types.h" 2




# 1 "include/linux/prio_tree.h" 1
# 14 "include/linux/prio_tree.h"
struct raw_prio_tree_node {
 struct prio_tree_node *left;
 struct prio_tree_node *right;
 struct prio_tree_node *parent;
};

struct prio_tree_node {
 struct prio_tree_node *left;
 struct prio_tree_node *right;
 struct prio_tree_node *parent;
 unsigned long start;
 unsigned long last;
};

struct prio_tree_root {
 struct prio_tree_node *prio_tree_node;
 unsigned short index_bits;
 unsigned short raw;




};

struct prio_tree_iter {
 struct prio_tree_node *cur;
 unsigned long mask;
 unsigned long value;
 int size_level;

 struct prio_tree_root *root;
 unsigned long r_index;
 unsigned long h_index;
};

static inline void prio_tree_iter_init(struct prio_tree_iter *iter,
  struct prio_tree_root *root, unsigned long r_index, unsigned long h_index)
{
 iter->root = root;
 iter->r_index = r_index;
 iter->h_index = h_index;
 iter->cur = ((void *)0);
}
# 84 "include/linux/prio_tree.h"
static inline int prio_tree_empty(const struct prio_tree_root *root)
{
 return root->prio_tree_node == ((void *)0);
}

static inline int prio_tree_root(const struct prio_tree_node *node)
{
 return node->parent == node;
}

static inline int prio_tree_left_empty(const struct prio_tree_node *node)
{
 return node->left == node;
}

static inline int prio_tree_right_empty(const struct prio_tree_node *node)
{
 return node->right == node;
}


struct prio_tree_node *prio_tree_replace(struct prio_tree_root *root,
                struct prio_tree_node *old, struct prio_tree_node *node);
struct prio_tree_node *prio_tree_insert(struct prio_tree_root *root,
                struct prio_tree_node *node);
void prio_tree_remove(struct prio_tree_root *root, struct prio_tree_node *node);
struct prio_tree_node *prio_tree_next(struct prio_tree_iter *iter);
# 10 "include/linux/mm_types.h" 2
# 1 "include/linux/rbtree.h" 1
# 100 "include/linux/rbtree.h"
struct rb_node
{
 unsigned long rb_parent_color;


 struct rb_node *rb_right;
 struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));


struct rb_root
{
 struct rb_node *rb_node;
};
# 123 "include/linux/rbtree.h"
static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
 rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rb_node *rb, int color)
{
 rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}
# 139 "include/linux/rbtree.h"
static inline void rb_init_node(struct rb_node *rb)
{
 rb->rb_parent_color = 0;
 rb->rb_right = ((void *)0);
 rb->rb_left = ((void *)0);
 (rb_set_parent(rb, rb));
}

extern void rb_insert_color(struct rb_node *, struct rb_root *);
extern void rb_erase(struct rb_node *, struct rb_root *);

typedef void (*rb_augment_f)(struct rb_node *node, void *data);

extern void rb_augment_insert(struct rb_node *node,
         rb_augment_f func, void *data);
extern struct rb_node *rb_augment_erase_begin(struct rb_node *node);
extern void rb_augment_erase_end(struct rb_node *node,
     rb_augment_f func, void *data);


extern struct rb_node *rb_next(const struct rb_node *);
extern struct rb_node *rb_prev(const struct rb_node *);
extern struct rb_node *rb_first(const struct rb_root *);
extern struct rb_node *rb_last(const struct rb_root *);


extern void rb_replace_node(struct rb_node *victim, struct rb_node *new,
       struct rb_root *root);

static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
    struct rb_node ** rb_link)
{
 node->rb_parent_color = (unsigned long )parent;
 node->rb_left = node->rb_right = ((void *)0);

 *rb_link = node;
}
# 11 "include/linux/mm_types.h" 2

# 1 "include/linux/completion.h" 1
# 25 "include/linux/completion.h"
struct completion {
 unsigned int done;
 wait_queue_head_t wait;
};
# 73 "include/linux/completion.h"
static inline void init_completion(struct completion *x)
{
 x->done = 0;
 do { static struct lock_class_key __key; __init_waitqueue_head((&x->wait), &__key); } while (0);
}

extern void wait_for_completion(struct completion *);
extern int wait_for_completion_interruptible(struct completion *x);
extern int wait_for_completion_killable(struct completion *x);
extern unsigned long wait_for_completion_timeout(struct completion *x,
         unsigned long timeout);
extern long wait_for_completion_interruptible_timeout(
 struct completion *x, unsigned long timeout);
extern long wait_for_completion_killable_timeout(
 struct completion *x, unsigned long timeout);
extern bool try_wait_for_completion(struct completion *x);
extern bool completion_done(struct completion *x);

extern void complete(struct completion *);
extern void complete_all(struct completion *);
# 13 "include/linux/mm_types.h" 2

# 1 "include/linux/page-debug-flags.h" 1
# 14 "include/linux/page-debug-flags.h"
enum page_debug_flags {
 PAGE_DEBUG_FLAG_POISON,
};
# 15 "include/linux/mm_types.h" 2

# 1 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/mmu.h" 1
# 14 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/../arch/x86/include/asm/mmu.h"
typedef struct {
 void *ldt;
 int size;






 struct mutex lock;
 void *vdso;


 struct desc_struct user_cs;
 unsigned long exec_limit;

} mm_context_t;


void leave_mm(int cpu);
# 17 "include/linux/mm_types.h" 2






struct address_space;
# 40 "include/linux/mm_types.h"
struct page {

 unsigned long flags;

 struct address_space *mapping;







 struct {
  union {
   unsigned long index;
   void *freelist;
  };

  union {

   unsigned long counters;

   struct {

    union {
# 81 "include/linux/mm_types.h"
     atomic_t _mapcount;

     struct {
      unsigned inuse:16;
      unsigned objects:15;
      unsigned frozen:1;
     };
    };
    atomic_t _count;
   };
  };
 };


 union {
  struct list_head lru;


  struct {
   struct page *next;




   short int pages;
   short int pobjects;

  };
 };


 union {
  unsigned long private;







  spinlock_t ptl;

  struct kmem_cache *slab;
  struct page *first_page;
 };
# 152 "include/linux/mm_types.h"
}






 __attribute__((__aligned__(2*sizeof(unsigned long))))

;

struct page_frag {
 struct page *page;




 __u16 offset;
 __u16 size;

};

typedef unsigned long vm_flags_t;






struct vm_region {
 struct rb_node vm_rb;
 vm_flags_t vm_flags;
 unsigned long vm_start;
 unsigned long vm_end;
 unsigned long vm_top;
 unsigned long vm_pgoff;
 struct file *vm_file;

 int vm_usage;
 bool vm_icache_flushed : 1;

};







struct vm_area_struct {
 struct mm_struct * vm_mm;
 unsigned long vm_start;
 unsigned long vm_end;



 struct vm_area_struct *vm_next, *vm_prev;

 pgprot_t vm_page_prot;
 unsigned long vm_flags;

 struct rb_node vm_rb;







 union {
  struct {
   struct list_head list;
   void *parent;
   struct vm_area_struct *head;
  } vm_set;

  struct raw_prio_tree_node prio_tree_node;
 } shared;







 struct list_head anon_vma_chain;

 struct anon_vma *anon_vma;


 const struct vm_operations_struct *vm_ops;


 unsigned long vm_pgoff;

 struct file * vm_file;
 void * vm_private_data;







};

struct core_thread {
 struct task_struct *task;
 struct core_thread *next;
};

struct core_state {
 atomic_t nr_threads;
 struct core_thread dumper;
 struct completion startup;
};

enum {
 MM_FILEPAGES,
 MM_ANONPAGES,
 MM_SWAPENTS,
 NR_MM_COUNTERS
};




struct task_rss_stat {
 int events;
 int count[NR_MM_COUNTERS];
};


struct mm_rss_stat {
 atomic_long_t count[NR_MM_COUNTERS];
};

struct mm_struct {
 struct vm_area_struct * mmap;
 struct rb_root mm_rb;
 struct vm_area_struct * mmap_cache;

 unsigned long (*get_unmapped_area) (struct file *filp,
    unsigned long addr, unsigned long len,
    unsigned long pgoff, unsigned long flags);
       unsigned long (*get_unmapped_exec_area) (struct file *filp,
    unsigned long addr, unsigned long len,
    unsigned long pgoff, unsigned long flags);
 void (*unmap_area) (struct mm_struct *mm, unsigned long addr);

 unsigned long mmap_base;
 unsigned long task_size;
 unsigned long cached_hole_size;
 unsigned long free_area_cache;
 pgd_t * pgd;
 atomic_t mm_users;
 atomic_t mm_count;
 int map_count;

 spinlock_t page_table_lock;
 struct rw_semaphore mmap_sem;

 struct list_head mmlist;





 unsigned long hiwater_rss;
 unsigned long hiwater_vm;

 unsigned long total_vm;
 unsigned long locked_vm;
 unsigned long pinned_vm;
 unsigned long shared_vm;
 unsigned long exec_vm;
 unsigned long stack_vm;
 unsigned long reserved_vm;
 unsigned long def_flags;
 unsigned long nr_ptes;
 unsigned long start_code, end_code, start_data, end_data;
 unsigned long start_brk, brk, start_stack;
 unsigned long arg_start, arg_end, env_start, env_end;

 unsigned long saved_auxv[(2*(2 + 19 + 1))];





 struct mm_rss_stat rss_stat;

 struct linux_binfmt *binfmt;

 cpumask_var_t cpu_vm_mask_var;


 mm_context_t context;
# 358 "include/linux/mm_types.h"
 unsigned int faultstamp;
 unsigned int token_priority;
 unsigned int last_interval;

 unsigned long flags;

 struct core_state *core_state;

 spinlock_t ioctx_lock;
 struct hlist_head ioctx_list;
# 380 "include/linux/mm_types.h"
 struct task_struct *owner;



 struct file *exe_file;
 unsigned long num_exe_file_vmas;

 struct mmu_notifier_mm *mmu_notifier_mm;


 pgtable_t pmd_huge_pte;




};

static inline void mm_init_cpumask(struct mm_struct *mm)
{



}


static inline cpumask_t *mm_cpumask(struct mm_struct *mm)
{
 return mm->cpu_vm_mask_var;
}
# 3 "/home/yabe/dev/src/ubuntu-precise/vmtest.headers/vmtest_headers.c" 2
