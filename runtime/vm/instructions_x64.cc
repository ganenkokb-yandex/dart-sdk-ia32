// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"  // Needed here to get TARGET_ARCH_X64.
#if defined(TARGET_ARCH_X64)

#include "platform/unaligned.h"

#include "vm/code_patcher.h"
#include "vm/instructions.h"
#include "vm/instructions_x64.h"

#include "vm/constants.h"
#include "vm/cpu.h"
#include "vm/object.h"
#include "vm/object_store.h"

namespace dart {

// [start] is the address of a displacement inside a load instruction
intptr_t IndexFromPPLoadDisp8(uword start) {
  int8_t offset = *reinterpret_cast<int8_t*>(start);
  return ObjectPool::IndexFromOffset(offset);
}

intptr_t IndexFromPPLoadDisp32(uword start) {
  int32_t offset = LoadUnaligned(reinterpret_cast<int32_t*>(start));
  return ObjectPool::IndexFromOffset(offset);
}

bool DecodeLoadObjectFromPoolOrThread(uword pc, const Code& code, Object* obj) {
#if 0
  ASSERT(code.ContainsInstructionAt(pc));

  uint8_t* bytes = reinterpret_cast<uint8_t*>(pc);

  COMPILE_ASSERT(THR == R14);
  if ((bytes[0] == 0x49) || (bytes[0] == 0x4d)) {
    if ((bytes[1] == 0x8b) || (bytes[1] == 0x3b)) {   // movq, cmpq
      if ((bytes[2] & 0xc7) == (0x80 | (THR & 7))) {  // [r14+disp32]
        int32_t offset = LoadUnaligned(reinterpret_cast<int32_t*>(pc + 3));
        return Thread::ObjectAtOffset(offset, obj);
      }
      if ((bytes[2] & 0xc7) == (0x40 | (THR & 7))) {  // [r14+disp8]
        uint8_t offset = *reinterpret_cast<uint8_t*>(pc + 3);
        return Thread::ObjectAtOffset(offset, obj);
      }
    }
  }

  if (((bytes[0] == 0x41) && (bytes[1] == 0xff) && (bytes[2] == 0x76))) {
    // push [r14+disp8]
    uint8_t offset = *reinterpret_cast<uint8_t*>(pc + 3);
    return Thread::ObjectAtOffset(offset, obj);
  }

  COMPILE_ASSERT(PP == R15);
  if ((bytes[0] == 0x49) || (bytes[0] == 0x4d)) {
    if ((bytes[1] == 0x8b) || (bytes[1] == 0x3b)) {  // movq, cmpq
      if ((bytes[2] & 0xc7) == (0x80 | (PP & 7))) {  // [r15+disp32]
        intptr_t index = IndexFromPPLoadDisp32(pc + 3);
        return ObjectAtPoolIndex(code, index, obj);
      }
      if ((bytes[2] & 0xc7) == (0x40 | (PP & 7))) {  // [r15+disp8]
        intptr_t index = IndexFromPPLoadDisp8(pc + 3);
        return ObjectAtPoolIndex(code, index, obj);
      }
    }
  }
#endif
  return false;
}

intptr_t TypeTestingStubCallPattern::GetSubtypeTestCachePoolIndex() {
  static int16_t indirect_call_pattern[] = {
      0xff, -1 /* 0x53 or 0x56 */, 0x07,  // callq [RBX/RSI + 0x7]
  };
  static int16_t direct_call_pattern[] = {
      0xe8, -1, -1, -1, -1,  // callq [PC + <offset>]
  };
  static int16_t pattern_disp8[] = {
      // 0:  49 89 8e ff ff 00 00    mov    QWORD PTR [r14+0xffff],rcx
      // 7:  49 8b 8e ff ff 00 00    mov    rcx,QWORD PTR [r14+0xffff]
      // e:  49 89 86 ff ff 00 00    mov    QWORD PTR [r14+0xffff],rax
      // 15: 49 8b 86 ff ff 00 00    mov    rax,QWORD PTR [r14+0xffff]
      // 1c: 48 8b 48 ff             mov    rcx,QWORD PTR [rax-0x1]
      // 20: 49 89 86 ff ff 00 00    mov    QWORD PTR [r14+0xffff],rax
      // 27: 49 8b 86 ff ff 00 00    mov    rax,QWORD PTR [r14+0xffff]
      // 2e: 49 89 8e ff ff 00 00    mov    QWORD PTR [r14+0xffff],rcx
      // 35: 49 8b 8e ff ff 00 00    mov    rcx,QWORD PTR [r14+0xffff]
      // 3c: 49 8b b6 ff ff 00 00    mov    rsi,QWORD PTR [r14+0xffff]
      0x49, 0x89, 0x8e, -1, -1, -1, -1,
      0x49, 0x8b, 0x8e, -1, -1, -1, -1,
      0x49, 0x89, 0x86, -1, -1, -1, -1,
      0x49, 0x8b, 0x86, -1, -1, -1, -1,
      0x48, 0x8b, 0x48, -1,
      0x49, 0x89, 0x86, -1, -1, -1, -1,
      0x49, 0x8b, 0x86, -1, -1, -1, -1,
      0x49, 0x89, 0x8e, -1, -1, -1, -1,
      0x49, 0x8b, 0x8e, -1, -1, -1, -1,
      0x49, 0x8b, 0xb6, -1, -1, -1, -1,
  };
  static int16_t pattern_disp32[] = {
      // 0:  49 89 8e ff ff 00 00    mov    QWORD PTR [r14+0xffff],rcx
      // 7:  49 8b 8e ff ff 00 00    mov    rcx,QWORD PTR [r14+0xffff]
      // e:  49 89 86 ff ff 00 00    mov    QWORD PTR [r14+0xffff],rax
      // 15: 49 8b 86 ff ff 00 00    mov    rax,QWORD PTR [r14+0xffff]
      // 1c: 48 8b 88 ff ff 00 00    mov    rcx,QWORD PTR [rax+0xffff]
      // 23: 49 89 86 ff ff ff ff    mov    QWORD PTR [r14-0x1],rax
      // 2a: 49 8b 86 ff ff 00 00    mov    rax,QWORD PTR [r14+0xffff]
      // 31: 49 89 8e ff ff 00 00    mov    QWORD PTR [r14+0xffff],rcx
      // 38: 49 8b 8e ff ff 00 00    mov    rcx,QWORD PTR [r14+0xffff]
      // 3f: 49 8b b6 ff ff 00 00    mov    rsi,QWORD PTR [r14+0xffff]
      0x49, 0x89, 0x8e, -1, -1, -1, -1,
      0x49, 0x8b, 0x8e, -1, -1, -1, -1,
      0x49, 0x89, 0x86, -1, -1, -1, -1,
      0x49, 0x8b, 0x86, -1, -1, -1, -1,
      0x48, 0x8b, 0x88, -1, -1, -1, -1,
      0x49, 0x89, 0x86, -1, -1, -1, -1,
      0x49, 0x8b, 0x86, -1, -1, -1, -1,
      0x49, 0x89, 0x8e, -1, -1, -1, -1,
      0x49, 0x8b, 0x8e, -1, -1, -1, -1,
      0x49, 0x8b, 0xb6, -1, -1, -1, -1,
  };
  int offset_for_offset = 31;
  uword pc = pc_;
  int skip_offset = 0;
  if (MatchesPattern(pc, direct_call_pattern,
                     ARRAY_SIZE(direct_call_pattern))) {
    pc -= ARRAY_SIZE(direct_call_pattern);
    skip_offset = 7;
  } else if (MatchesPattern(pc, indirect_call_pattern,
                            ARRAY_SIZE(indirect_call_pattern))) {
    pc -= ARRAY_SIZE(indirect_call_pattern);
  } else {
    FATAL("Failed to decode at %" Px, pc_);
  }

  if (MatchesPattern(pc, pattern_disp8,
                     ARRAY_SIZE(pattern_disp8) - skip_offset)) {
    return IndexFromPPLoadDisp8(pc - ARRAY_SIZE(pattern_disp8) + skip_offset +
                                offset_for_offset);
  } else if (MatchesPattern(pc, pattern_disp32,
                            ARRAY_SIZE(pattern_disp32) - skip_offset)) {
    return IndexFromPPLoadDisp32(pc - ARRAY_SIZE(pattern_disp32) + skip_offset +
                                 offset_for_offset);
  } else {
    FATAL("Failed to decode at %" Px, pc);
  }
}

}  // namespace dart

#endif  // defined TARGET_ARCH_X64
