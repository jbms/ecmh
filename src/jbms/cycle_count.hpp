#ifndef HEADER_GUARD_4fb412d8f610e46d1e6176a8d4ec633a
#define HEADER_GUARD_4fb412d8f610e46d1e6176a8d4ec633a

#include <stdint.h>

namespace jbms {

// http://nadeausoftware.com/articles/2012/02/c_c_tip_how_detect_processor_type_using_compiler_predefined_macros
#if defined(__i386) || defined(_M_IX86)
inline uint64_t cycle_count_begin() {
  uint64_t x;
  // cpuid clobbers eax, ebx, ecx, edx
  // set eax to 0 to make cpuid do the least work
  __asm__ volatile("cpuid; rdtsc" : "=A"(x) : "a"(0) : "ebx", "ecx");
  return x;
}

inline uint64_t cycle_count_end() {
  uint64_t x;
  // cpuid clobbers eax, ebx, ecx, edx
  // set eax to 0 to make cpuid do the least work
  __asm__ volatile("cpuid; rdtsc" : "=A"(x) : "a"(0) : "ebx", "ecx");
  return x;
}

#elif defined(__x86_64__) || defined(_M_X64)
inline uint64_t cycle_count_begin() {
  uint64_t a, d;
  // cpuid clobbers rax, rbx, rcx, rdx
  // set eax to 0 to make cpuid do the least work
  __asm__ volatile("cpuid; rdtsc" : "=a"(a), "=d"(d) : "a"(0) : "rbx", "rcx");
  return (d << 32) | a;
}

inline uint64_t cycle_count_end() {
  uint64_t a, d;

  __asm__ volatile(
#ifdef HAVE_RDTSCP
                   "rdtscp; "
#else
                   "rdtsc; "
#endif
                   "mov %%rax, %0; " // save eax and edx since cpuid clobbers them
                   "mov %%rdx, %1; "
                   "xorl %%eax, %%eax; " // set eax to 0 to make cpuid do the least work
                   "cpuid" // cpuid clobbers rax, rbx, rcx, rdx
                   : "=r"(a), "=r"(d)
                   :
                   : "%rax", "%rbx", "%rcx", "%rdx");
  return (d << 32) | a;
}

#endif

}


#endif /* HEADER GUARD */
