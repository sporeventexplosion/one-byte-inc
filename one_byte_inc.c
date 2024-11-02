#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static int (*one_byte_inc_ptr)(int);

int init_one_byte_inc() {
  // Map the page that holds our code below 4 GiB
  // We're serious about security: we stay W^X at all times
  unsigned char *page = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
  if (page == NULL) {
    return -1;
  }

  unsigned short cs64;
  __asm__("movw %%cs,%0" : "=r"(cs64));
  __asm__("movw %%cs,%0" : "=r"(cs64));
  // Linux places the 64-bit user code segment 2 entries above the 32-bit one
  unsigned short cs32 = cs64 - 16;

  unsigned char *code = page;

  // the upper halves of the low 8 GPRs are not preserved when switching between
  // 64-bit mode and compatibility mode; we need to manually preserve them. For
  // the AMD64 System V ABI, only rbx and rbp need to be preserved.
  *code++ = 0x53; // push rbx
  *code++ = 0x55; // push rbp

  // move our argument into eax
  *code++ = 0x89; // mov eax, edi
  *code++ = 0xf8;
  // We need to preserve the stack pointer across the switch, so we store the
  // high half of rsp in edi
  *code++ = 0x48; // mov rdi, rsp
  *code++ = 0x89;
  *code++ = 0xe7;
  *code++ = 0x48; // shr rdi, 32
  *code++ = 0xc1;
  *code++ = 0xef;
  *code++ = 0x20;
  *code++ = 0xff; // jmp far dword [rip]
  *code++ = 0x2d;
  *code++ = 0x00;
  *code++ = 0x00;
  *code++ = 0x00;
  *code++ = 0x00;

  unsigned addr = ((unsigned long)code) + 6;
  // our jump target as a "m16:32". The address comes before the segment
  // selector The address to jump to comes directly after these six bytes.
  memcpy(code, &addr, 4);
  memcpy(code + 4, &cs32, 2);
  code += 6;

  // We're now in compatibility mode

  // clang-format off
  //
  //     |||||||||||||||||||     \\\\\         |||||        /|||||||||||\                   |||||||||||||||||||          /|||||||\          \\\\\          /////     |||||     |||||     |||||
  //     |||||||||||||||||||     |\\\\\        |||||       ///|||||||||\\\                  |||||||||||||||||||         /|||||||||\          \\\\\        /////      |||||     |||||     |||||
  //     |||||||||||||||||||     ||\\\\\       |||||      //////|||||\\\\\\                 |||||||||||||||||||        /|||||||||||\          \\\\\      /////       |||||     |||||     |||||
  //            |||||            |||\\\\\      |||||     //////       \\\\\\                |||||                     //////   \\\\\\          \\\\\    /////        |||||     |||||     |||||
  //            |||||            ||||\\\\\     |||||     |||||         |||||                |||||                    //////     \\\\\\          \\\\\  /////         |||||     |||||     |||||
  //            |||||            |||||\\\\\    |||||     |||||                              |||||                   //////       \\\\\\          \\\\\/////          |||||     |||||     |||||
  //            |||||            ||||| \\\\\   |||||     |||||                              |||||||||||||||         |||||         |||||           \\\||///           |||||     |||||     |||||
  //            |||||            |||||  \\\\\  |||||     |||||                              |||||||||||||||         |||||||||||||||||||           ||||||||           |||||     |||||     |||||
  //            |||||            |||||   \\\\\ |||||     |||||                              |||||||||||||||         |||||||||||||||||||           ///||\\\           |||||     |||||     |||||
  //            |||||            |||||    \\\\\|||||     |||||                              |||||                   |||||||||||||||||||          /////\\\\\          |||||     |||||     |||||
  //            |||||            |||||     \\\\\||||     |||||         |||||                |||||                   |||||         |||||         /////  \\\\\         |||||     |||||     |||||
  //            |||||            |||||      \\\\\|||     \\\\\\       //////                |||||                   |||||         |||||        /////    \\\\\                                 .
  //     |||||||||||||||||||     |||||       \\\\\||      \\\\\\|||||//////                 |||||||||||||||||||     |||||         |||||       /////      \\\\\       |||||     |||||     |||||
  //     |||||||||||||||||||     |||||        \\\\\|       \\\|||||||||///                  |||||||||||||||||||     |||||         |||||      /////        \\\\\      |||||     |||||     |||||
  //     |||||||||||||||||||     |||||         \\\\\        \|||||||||||/                   |||||||||||||||||||     |||||         |||||     /////          \\\\\     |||||     |||||     |||||
  //
  // clang-format on
  *code++ = 0x40;

  // Now we have to do a far jump back.
  // In protected mode you can encode the destination directly inside the
  // instruction

  addr = ((unsigned long)code) + 7;
  *code++ = 0xea; // jmp far 64bitseg:addr
  memcpy(code, &addr, 4);
  memcpy(code + 4, &cs64, 2);
  code += 6;

  // Back in 64-bit mode
  // Put our stack pointer back together
  // It's not clear if the upper half of rsp is guaranteed to be zero here, so
  // we explicitly zero it. For rax, on the other hand, the ABI allows there to
  // be junk in the upper half, so we don't need to care about it.
  *code++ = 0x89; // mov esp, esp
  *code++ = 0xe4;
  *code++ = 0x48; // shl rdi, 32
  *code++ = 0xc1;
  *code++ = 0xe7;
  *code++ = 0x20;
  *code++ = 0x48; // or rsp, rdi
  *code++ = 0x09;
  *code++ = 0xfc;

  // Now we have our stack pointer, restore our saved registers
  *code++ = 0x5d; // pop rbp
  *code++ = 0x5b; // pop rbx
  *code++ = 0xc3; // ret

  // Make our page executable
  if (mprotect(page, 4096, PROT_READ | PROT_EXEC) != 0) {
    munmap(page, 4096);
    return -1;
  }

  one_byte_inc_ptr = (int (*)(int))page;

  return 0;
}

int one_byte_inc(int x) {
  return one_byte_inc_ptr(x);
}
