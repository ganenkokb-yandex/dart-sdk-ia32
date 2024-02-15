// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"  // Needed here to get TARGET_ARCH_IA32.
#if defined(TARGET_ARCH_IA32)

#include "vm/code_patcher.h"
#include "vm/instructions.h"
#include "vm/instructions_ia32.h"

#include "vm/cpu.h"
#include "vm/object.h"

namespace dart {

intptr_t IndexFromPPLoadDisp8(uword start) {
  int8_t offset = *reinterpret_cast<int8_t*>(start);
  return ObjectPool::IndexFromOffset(offset);
}

intptr_t IndexFromPPLoadDisp32(uword start) {
  int32_t offset = LoadUnaligned(reinterpret_cast<int32_t*>(start));
  return ObjectPool::IndexFromOffset(offset);
}

bool DecodeLoadObjectFromPoolOrThread(uword pc, const Code& code, Object* obj) {
  ASSERT(code.ContainsInstructionAt(pc));
  return false;
}

intptr_t TypeTestingStubCallPattern::GetSubtypeTestCachePoolIndex() {
  // clang-format off
  static int16_t indirect_call_pattern[] = {
    /* 0x00: mov edi,dword ptr [esi+5E4h] */ 0x8B, 0xBE, -1, -1, -1, -1,
    /* 0x06: call dword ptr [edi+3]       */ 0xFF, 0x57, -1,
  };
  static int16_t direct_call_pattern[] = {
      0xe8, -1, -1, -1, -1,  // callq [PC + <offset>]
  };

  const int32_t pattern_pc_offset = 0x18 + 0x2;
  static int16_t pattern_disp8[] = {
    /* 0x00: mov dword ptr [esi+5A0h],ecx */ 0x89, 0x8E, -1, -1, -1, -1,
    /* 0x06: mov ecx,dword ptr [esi+618h] */ 0x8B, 0x8E, -1, -1, -1, -1,
    /* 0x0c: mov dword ptr [esi+59Ch],eax */ 0x89, 0x86, -1, -1, -1, -1,
    /* 0x12: mov eax,dword ptr [esi+60Ch] */ 0x8B, 0x86, -1, -1, -1, -1,
    /* 0x18: mov ecx,dword ptr [eax+17h]  */ 0x8B, 0x48, -1,
    /* 0x1b: mov dword ptr [esi+60Ch],eax */ 0x89, 0x86, -1, -1, -1, -1,
    /* 0x21: mov eax,dword ptr [esi+59Ch] */ 0x8B, 0x86, -1, -1, -1, -1,
    /* 0x27: mov dword ptr [esi+618h],ecx */ 0x89, 0x8E, -1, -1, -1, -1,
    /* 0x2d: mov ecx,dword ptr [esi+5A0h] */ 0x8B, 0x8E, -1, -1, -1, -1,
  };
  static int16_t pattern_disp32[] = {
    /* 0x00: mov dword ptr [esi+5A0h],ecx */ 0x89, 0x8E, -1, -1, -1, -1,
    /* 0x06: mov ecx,dword ptr [esi+618h] */ 0x8B, 0x8E, -1, -1, -1, -1,
    /* 0x0c: mov dword ptr [esi+59Ch],eax */ 0x89, 0x86, -1, -1, -1, -1,
    /* 0x12: mov eax,dword ptr [esi+60Ch] */ 0x8B, 0x86, -1, -1, -1, -1,
    /* 0x18: mov ecx,dword ptr [eax+0A7h] */ 0x8B, 0x88, -1, -1, -1, -1,
    /* 0x1e: mov dword ptr [esi+60Ch],eax */ 0x89, 0x86, -1, -1, -1, -1,
    /* 0x24: mov eax,dword ptr [esi+59Ch] */ 0x8B, 0x86, -1, -1, -1, -1,
    /* 0x2a: mov dword ptr [esi+618h],ecx */ 0x89, 0x8E, -1, -1, -1, -1,
    /* 0x30: mov ecx,dword ptr [esi+5A0h] */ 0x8B, 0x8E, -1, -1, -1, -1,
  };
  // clang-format on

  uword pc = pc_;
  if (MatchesPattern(pc, direct_call_pattern,
                     ARRAY_SIZE(direct_call_pattern))) {
    pc -= ARRAY_SIZE(direct_call_pattern);
  } else if (MatchesPattern(pc, indirect_call_pattern,
                            ARRAY_SIZE(indirect_call_pattern))) {
    pc -= ARRAY_SIZE(indirect_call_pattern);
  } else {
    FATAL("Failed to decode at %" Px, pc_);
  }

  if (MatchesPattern(pc, pattern_disp8, ARRAY_SIZE(pattern_disp8))) {
    auto idx = IndexFromPPLoadDisp8(pc - ARRAY_SIZE(pattern_disp8) +
                                    pattern_pc_offset);
    return idx;
  } else if (MatchesPattern(pc, pattern_disp32, ARRAY_SIZE(pattern_disp32))) {
    auto idx = IndexFromPPLoadDisp32(pc - ARRAY_SIZE(pattern_disp32) +
                                     pattern_pc_offset);
    return idx;
  } else {
    FATAL("Failed to decode at %" Px, pc);
  }
}

}  // namespace dart

#endif  // defined TARGET_ARCH_IA32
