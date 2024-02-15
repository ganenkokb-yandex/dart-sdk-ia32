// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_VM_CONSTANTS_X64_H_
#define RUNTIME_VM_CONSTANTS_X64_H_

#ifndef RUNTIME_VM_CONSTANTS_H_
#error Do not include constants_x64.h directly; use constants.h instead.
#endif

#include "platform/assert.h"
#include "platform/globals.h"
#include "platform/utils.h"

#include "vm/compiler/like_abi.h"
#include "vm/constants_base.h"
#include "vm/globals.h"

namespace dart {

#define R(reg) (static_cast<RegList>(1) << (reg))

enum Register {
  RAX = 0,
  RCX = 1,
  RDX = 2,
  RBX = 3,
  RSP = 4,  // SP
  RBP = 5,  // FP
  RSI = 6,
  RDI = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,  // TMP
  R12 = 12,  // CODE_REG
  R13 = 13,
  R14 = 14,  // THR
  R15 = 15,  // PP
  kNumberOfCpuRegisters = 16,
  kNoRegister = -1,  // Signals an illegal register.
};

enum ByteRegister {
  AL = 0,
  CL = 1,
  DL = 2,
  BL = 3,
  AH = 4,
  CH = 5,
  DH = 6,
  BH = 7,
  SPL = 4 | 0x10,
  BPL = 5 | 0x10,
  SIL = 6 | 0x10,
  DIL = 7 | 0x10,
  R8B = 8,
  R9B = 9,
  R10B = 10,
  R11B = 11,
  R12B = 12,
  R13B = 13,
  R14B = 14,
  R15B = 15,
  kNumberOfByteRegisters = 16,
  kNoByteRegister = -1  // Signals an illegal register.
};

inline constexpr Register FindOrigReg(dart::compiler::LikeABI like_abi) {
  switch (like_abi) {
    case dart::compiler::LikeABI::PP:
      return Register::R15;
    case dart::compiler::LikeABI::CODE:
      return Register::R12;
    case dart::compiler::LikeABI::kWriteBarrierObjectReg:
      return Register::RDX;
    case dart::compiler::LikeABI::kWriteBarrierValueReg:
      return Register::RAX;
    case dart::compiler::LikeABI::kWriteBarrierSlotReg:
      return Register::R13;
    case dart::compiler::LikeABI::STCInternalRegs_kCacheEntryReg:
      return Register::RDI;
    case dart::compiler::LikeABI::STCInternalRegs_kInstanceCidOrSignatureReg:
      return Register::R10;
    case dart::compiler::LikeABI::
        STCInternalRegs_kInstanceInstantiatorTypeArgumentsReg:
      return Register::R13;
    case dart::compiler::LikeABI::TypeTestABI_kDstTypeReg:
      return Register::RBX;
    case dart::compiler::LikeABI::TypeTestABI_kInstantiatorTypeArgumentsReg:
      return Register::RDX;
    case dart::compiler::LikeABI::TypeTestABI_kFunctionTypeArgumentsReg:
      return Register::RCX;
    case dart::compiler::LikeABI::TTABI_kSubtypeTestCache:
      return Register::R9;
    case dart::compiler::LikeABI::TTABI_kScratch:
      return Register::RSI;
    case dart::compiler::LikeABI::AssertSubtypeABI_kSubTypeReg:
      return Register::RAX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kSuperTypeReg:
      return Register::RBX;
    case dart::compiler::LikeABI::
        AssertSubtypeABI_kInstantiatorTypeArgumentsReg:
      return Register::RDX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kFunctionTypeArgumentsReg:
      return Register::RCX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kDstNameReg:
      return Register::R9;
    case dart::compiler::LikeABI::AllocateObjectABI_kTypeArgumentsReg:
      return Register::RDX;
    case dart::compiler::LikeABI::AllocateObjectABI_kTagsReg:
      return Register::R8;
    case dart::compiler::LikeABI::AllocateClosureABI_kFunctionReg:
      return Register::RBX;
    case dart::compiler::LikeABI::SuspendStubABI_kTempReg:
      return Register::RDX;
    case dart::compiler::LikeABI::SuspendStubABI_kFrameSizeReg:
      return Register::RCX;
    case dart::compiler::LikeABI::SuspendStubABI_kSuspendStateReg:
      return Register::RBX;
    case dart::compiler::LikeABI::SuspendStubABI_kFunctionDataReg:
      return Register::R8;
    case dart::compiler::LikeABI::SuspendStubABI_kSrcFrameReg:
      return Register::RSI;
    case dart::compiler::LikeABI::SuspendStubABI_kDstFrameReg:
      return Register::RDI;
    case dart::compiler::LikeABI::CloneSuspendStateStubABI_kDestinationReg:
      return Register::RBX;
    case dart::compiler::LikeABI::CloneSuspendStateStubABI_kTempReg:
      return Register::RDX;
    case dart::compiler::LikeABI::CloneSuspendStateStubABI_kFrameSizeReg:
      return Register::RCX;
    case dart::compiler::LikeABI::CloneSuspendStateStubABI_kSrcFrameReg:
      return Register::RSI;
    case dart::compiler::LikeABI::CloneSuspendStateStubABI_kDstFrameReg:
      return Register::RDI;
    case dart::compiler::LikeABI::DispatchTableNullErrorABI_kClassIdReg:
      return Register::RCX;
    default:
      RELEASE_ASSERT(false);
      return kNoRegister;
  }
}

inline ByteRegister ByteRegisterOf(Register reg) {
  if (RSP <= reg && reg <= RDI) {
    return static_cast<ByteRegister>(reg | 0x10);
  } else {
    return static_cast<ByteRegister>(reg);
  }
}

enum XmmRegister {
  XMM0 = 0,
  XMM1 = 1,
  XMM2 = 2,
  XMM3 = 3,
  XMM4 = 4,
  XMM5 = 5,
  XMM6 = 6,
  XMM7 = 7,
  XMM8 = 8,
  XMM9 = 9,
  XMM10 = 10,
  XMM11 = 11,
  XMM12 = 12,
  XMM13 = 13,
  XMM14 = 14,
  XMM15 = 15,
  kNumberOfXmmRegisters = 16,
  kNoXmmRegister = -1  // Signals an illegal register.
};

// Architecture independent aliases.
typedef XmmRegister FpuRegister;
const FpuRegister FpuTMP = XMM15;
const int kFpuRegisterSize = 16;
typedef simd128_value_t fpu_register_t;
const int kNumberOfFpuRegisters = kNumberOfXmmRegisters;
const FpuRegister kNoFpuRegister = kNoXmmRegister;

extern const char* const cpu_reg_names[kNumberOfCpuRegisters];
extern const char* const cpu_reg_abi_names[kNumberOfCpuRegisters];
extern const char* const cpu_reg_byte_names[kNumberOfByteRegisters];
extern const char* const fpu_reg_names[kNumberOfXmmRegisters];

enum RexBits {
  REX_NONE = 0,
  REX_B = 1 << 0,
  REX_X = 1 << 1,
  REX_R = 1 << 2,
  REX_W = 1 << 3,
  REX_PREFIX = 1 << 6
};

// Register aliases.
const Register TMP = R11;  // Used as scratch register by the assembler.
const Register TMP2 = kNoRegister;  // No second assembler scratch register.
// Caches object pool pointer in generated code.
#if USE_LIKE_ABI_SEPARATED
const dart::compiler::LikeABI PP = dart::compiler::LikeABI::PP;
#else
const dart::compiler::LikeABI PP = dart::compiler::LikeABI::R15;
#endif
const Register SPREG = RSP;          // Stack pointer register.
const Register FPREG = RBP;          // Frame pointer register.
const Register RECEIVER_REG = RDX;
const Register IC_DATA_REG = RBX;    // ICData/MegamorphicCache register.
const Register ARGS_DESC_REG = R10;  // Arguments descriptor register.
#if USE_LIKE_ABI_SEPARATED
const dart::compiler::LikeABI CODE_REG = dart::compiler::LikeABI::CODE;
#else
const dart::compiler::LikeABI CODE_REG = dart::compiler::LikeABI::R12;
#endif
// Set when calling Dart functions in JIT mode, used by LazyCompileStub.
const Register FUNCTION_REG = RAX;
const Register THR = R14;  // Caches current thread in generated code.
const Register CALLEE_SAVED_TEMP = RBX;

const Register RAX_FOR_SCOPED = RAX;
const Register RCX_FOR_SCOPED = RCX;
const Register RDX_FOR_SCOPED = RDX;
const Register RBX_FOR_SCOPED = RBX;

// ABI for catch-clause entry point.
const Register kExceptionObjectReg = RAX;
const Register kStackTraceObjectReg = RDX;

// ABI for write barrier stub.
#if USE_LIKE_ABI_SEPARATED
const dart::compiler::LikeABI kWriteBarrierObjectReg =
    dart::compiler::LikeABI::kWriteBarrierObjectReg;
const dart::compiler::LikeABI kWriteBarrierValueReg =
    dart::compiler::LikeABI::kWriteBarrierValueReg;
const dart::compiler::LikeABI kWriteBarrierSlotReg =
    dart::compiler::LikeABI::kWriteBarrierSlotReg;
#else
const dart::compiler::LikeABI kWriteBarrierObjectReg = dart::compiler::LikeABI::RDX;
const dart::compiler::LikeABI kWriteBarrierValueReg = dart::compiler::LikeABI::RAX;
const dart::compiler::LikeABI kWriteBarrierSlotReg = dart::compiler::LikeABI::R13;
#endif

// Common ABI for shared slow path stubs.
struct SharedSlowPathStubABI {
  static constexpr Register kResultReg = RAX;
};

// ABI for instantiation stubs.
struct InstantiationABI {
  static constexpr Register kUninstantiatedTypeArgumentsReg = RBX;
  static constexpr Register kInstantiatorTypeArgumentsReg = RDX;
  static constexpr Register kFunctionTypeArgumentsReg = RCX;
  static constexpr Register kResultTypeArgumentsReg = RAX;
  static constexpr Register kResultTypeReg = RAX;
  static constexpr Register kScratchReg = R9;
};

// Registers in addition to those listed in InstantiationABI used inside the
// implementation of the InstantiateTypeArguments stubs.
struct InstantiateTAVInternalRegs {
  // The set of registers that must be pushed/popped when probing a hash-based
  // cache due to overlap with the registers in InstantiationABI.
  static constexpr intptr_t kSavedRegisters = 0;

  // Additional registers used to probe hash-based caches.
  static constexpr Register kEntryStartReg = R10;
  static constexpr Register kProbeMaskReg = R13;
  static constexpr Register kProbeDistanceReg = R8;
  static constexpr Register kCurrentEntryIndexReg = RSI;
};

// Registers in addition to those listed in TypeTestABI used inside the
// implementation of type testing stubs that are _not_ preserved.
struct TTSInternalRegs {
  static constexpr Register kInstanceTypeArgumentsReg = RSI;
  static constexpr Register kScratchReg = R8;
  static constexpr Register kSubTypeArgumentReg = R10;
  static constexpr Register kSuperTypeArgumentReg = R13;

  // Must be pushed/popped whenever generic type arguments are being checked as
  // they overlap with registers in TypeTestABI.
  static constexpr intptr_t kSavedTypeArgumentRegisters = 0;

  static constexpr intptr_t kInternalRegisters =
      ((1 << kInstanceTypeArgumentsReg) | (1 << kScratchReg) |
       (1 << kSubTypeArgumentReg) | (1 << kSuperTypeArgumentReg)) &
      ~kSavedTypeArgumentRegisters;
};

// Registers in addition to those listed in TypeTestABI used inside the
// implementation of subtype test cache stubs that are _not_ preserved.
struct STCInternalRegs {
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kCacheEntryReg =
      dart::compiler::LikeABI::STCInternalRegs_kCacheEntryReg;
  static constexpr dart::compiler::LikeABI kInstanceCidOrSignatureReg =
      dart::compiler::LikeABI::STCInternalRegs_kInstanceCidOrSignatureReg;
  static constexpr dart::compiler::LikeABI
      kInstanceInstantiatorTypeArgumentsReg = dart::compiler::LikeABI::
          STCInternalRegs_kInstanceInstantiatorTypeArgumentsReg;
#else
  static constexpr dart::compiler::LikeABI kCacheEntryReg = dart::compiler::LikeABI::RDI;
  static constexpr dart::compiler::LikeABI kInstanceCidOrSignatureReg = dart::compiler::LikeABI::R10;
  static constexpr dart::compiler::LikeABI kInstanceInstantiatorTypeArgumentsReg = dart::compiler::LikeABI::R13;
#endif

  static constexpr intptr_t kInternalRegisters = 0;

#if 0
  static constexpr intptr_t kInternalRegisters =
      (1 << kCacheEntryReg) | (1 << kInstanceCidOrSignatureReg) |
      (1 << kInstanceInstantiatorTypeArgumentsReg);
#endif
};

// Calling convention when calling TypeTestingStub and SubtypeTestCacheStub.
struct TypeTestABI {
  static constexpr Register kInstanceReg = RAX;
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kDstTypeReg =
      dart::compiler::LikeABI::TypeTestABI_kDstTypeReg;
  static constexpr dart::compiler::LikeABI kInstantiatorTypeArgumentsReg =
      dart::compiler::LikeABI::TypeTestABI_kInstantiatorTypeArgumentsReg;
  static constexpr dart::compiler::LikeABI kFunctionTypeArgumentsReg =
      dart::compiler::LikeABI::TypeTestABI_kFunctionTypeArgumentsReg;
  static constexpr dart::compiler::LikeABI kSubtypeTestCacheReg =
      dart::compiler::LikeABI::TTABI_kSubtypeTestCache;
  static constexpr dart::compiler::LikeABI kScratchReg =
      dart::compiler::LikeABI::TTABI_kScratch;
  static constexpr dart::compiler::LikeABI kSubtypeTestCacheResultReg =
      dart::compiler::LikeABI::TypeTestABI_kSubtypeTestCacheResultReg;

#else
  static constexpr dart::compiler::LikeABI kDstTypeReg = dart::compiler::LikeABI::RBX;
  static constexpr dart::compiler::LikeABI kInstantiatorTypeArgumentsReg = dart::compiler::LikeABI::RDX;
  static constexpr dart::compiler::LikeABI kFunctionTypeArgumentsReg = dart::compiler::LikeABI::RCX;
  static constexpr dart::compiler::LikeABI kSubtypeTestCacheReg = dart::compiler::LikeABI::R9;
  static constexpr dart::compiler::LikeABI kScratchReg = dart::compiler::LikeABI::RSI;
  static constexpr dart::compiler::LikeABI kSubtypeTestCacheResultReg =
      dart::compiler::LikeABI::RSI;

#endif

  // For calls to InstanceOfStub.
  static constexpr Register kInstanceOfResultReg = kInstanceReg;
  // For calls to SubtypeNTestCacheStub. Must not overlap with any other
  // registers above, for it is also used internally as kNullReg in those stubs.
  // static constexpr Register kSubtypeTestCacheResultReg = RSI;

  // No registers need saving across SubtypeTestCacheStub calls.
  // static constexpr intptr_t kSubtypeTestCacheStubCallerSavedRegisters =
  //     1 << kSubtypeTestCacheResultReg;

  static constexpr intptr_t kPreservedAbiRegisters = (1 << kInstanceReg);

  static constexpr intptr_t kNonPreservedAbiRegisters =
      TTSInternalRegs::kInternalRegisters |
      STCInternalRegs::kInternalRegisters |
       (1 << R12); // CODE_REG

  static constexpr intptr_t kAbiRegisters =
      kPreservedAbiRegisters | kNonPreservedAbiRegisters;
};

// Calling convention when calling AssertSubtypeStub.
struct AssertSubtypeABI {
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kSubTypeReg =
      dart::compiler::LikeABI::AssertSubtypeABI_kSubTypeReg;
  static constexpr dart::compiler::LikeABI kSuperTypeReg =
      dart::compiler::LikeABI::AssertSubtypeABI_kSuperTypeReg;
  static constexpr dart::compiler::LikeABI kInstantiatorTypeArgumentsReg =
      dart::compiler::LikeABI::AssertSubtypeABI_kInstantiatorTypeArgumentsReg;
  static constexpr dart::compiler::LikeABI kFunctionTypeArgumentsReg =
      dart::compiler::LikeABI::AssertSubtypeABI_kFunctionTypeArgumentsReg;
  static constexpr dart::compiler::LikeABI kDstNameReg =
      dart::compiler::LikeABI::AssertSubtypeABI_kDstNameReg;
#else
  static constexpr dart::compiler::LikeABI kSubTypeReg = dart::compiler::LikeABI::RAX;
  static constexpr dart::compiler::LikeABI kSuperTypeReg = dart::compiler::LikeABI::RBX;
  static constexpr dart::compiler::LikeABI kInstantiatorTypeArgumentsReg = dart::compiler::LikeABI::RDX;
  static constexpr dart::compiler::LikeABI kFunctionTypeArgumentsReg = dart::compiler::LikeABI::RCX;
  static constexpr dart::compiler::LikeABI kDstNameReg = dart::compiler::LikeABI::R9;
#endif

#if 0
  static constexpr intptr_t kAbiRegisters =
      (1 << kSubTypeReg) | (1 << kSuperTypeReg) |
      (1 << kInstantiatorTypeArgumentsReg) | (1 << kFunctionTypeArgumentsReg) |
      (1 << kDstNameReg);
#endif

  // No result register, as AssertSubtype is only run for side effect
  // (throws if the subtype check fails).
};

// ABI for InitStaticFieldStub.
struct InitStaticFieldABI {
  static constexpr Register kFieldReg = RDX;
  static constexpr Register kResultReg = RAX;
};

// Registers used inside the implementation of InitLateStaticFieldStub.
struct InitLateStaticFieldInternalRegs {
  static constexpr Register kAddressReg = RCX;
  static constexpr Register kScratchReg = RSI;
};

// ABI for InitInstanceFieldStub.
struct InitInstanceFieldABI {
  static constexpr Register kInstanceReg = RBX;
  static constexpr Register kFieldReg = RDX;
  static constexpr Register kResultReg = RAX;
};

// Registers used inside the implementation of InitLateInstanceFieldStub.
struct InitLateInstanceFieldInternalRegs {
  static constexpr Register kAddressReg = RCX;
  static constexpr Register kScratchReg = RSI;
};

// ABI for LateInitializationError stubs.
struct LateInitializationErrorABI {
  static constexpr Register kFieldReg = RSI;
};

// ABI for ThrowStub.
struct ThrowABI {
  static constexpr Register kExceptionReg = RAX;
};

// ABI for ReThrowStub.
struct ReThrowABI {
  static constexpr Register kExceptionReg = RAX;
  static constexpr Register kStackTraceReg = RBX;
};

// ABI for AssertBooleanStub.
struct AssertBooleanABI {
  static constexpr Register kObjectReg = RAX;
};

// ABI for RangeErrorStub.
struct RangeErrorABI {
  static constexpr Register kLengthReg = RAX;
  static constexpr Register kIndexReg = RBX;
};

// ABI for AllocateObjectStub.
struct AllocateObjectABI {
  static constexpr Register kResultReg = RAX;
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kTypeArgumentsReg =
      dart::compiler::LikeABI::AllocateObjectABI_kTypeArgumentsReg;
  static constexpr dart::compiler::LikeABI kTagsReg =
      dart::compiler::LikeABI::AllocateObjectABI_kTagsReg;
#else
  static constexpr dart::compiler::LikeABI kTypeArgumentsReg = dart::compiler::LikeABI::RDX;
  static constexpr dart::compiler::LikeABI kTagsReg = dart::compiler::LikeABI::R8;
#endif
};

// ABI for AllocateClosureStub.
struct AllocateClosureABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kFunctionReg =
      dart::compiler::LikeABI::AllocateClosureABI_kFunctionReg;
#else
  static constexpr dart::compiler::LikeABI kFunctionReg = dart::compiler::LikeABI::RBX;
#endif
  static constexpr Register kContextReg = RDX;
  static constexpr Register kScratchReg = R13;
};

// ABI for AllocateMintShared*Stub.
struct AllocateMintABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kTempReg = RBX;
};

// ABI for Allocate{Mint,Double,Float32x4,Float64x2}Stub.
struct AllocateBoxABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kTempReg = RBX;
};

// ABI for AllocateArrayStub.
struct AllocateArrayABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kLengthReg = R10;
  static constexpr Register kTypeArgumentsReg = RBX;
};

// ABI for AllocateRecordStub.
struct AllocateRecordABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kShapeReg = RBX;
  static constexpr Register kTemp1Reg = RDX;
  static constexpr Register kTemp2Reg = RCX;
};

// ABI for AllocateSmallRecordStub (AllocateRecord2, AllocateRecord2Named,
// AllocateRecord3, AllocateRecord3Named).
struct AllocateSmallRecordABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kShapeReg = R10;
  static constexpr Register kValue0Reg = RBX;
  static constexpr Register kValue1Reg = RDX;
  static constexpr Register kValue2Reg = RCX;
  static constexpr Register kTempReg = RDI;
};

// ABI for AllocateTypedDataArrayStub.
struct AllocateTypedDataArrayABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kLengthReg = kResultReg;
};

// ABI for BoxDoubleStub.
struct BoxDoubleStubABI {
  static constexpr FpuRegister kValueReg = XMM0;
  static constexpr Register kTempReg = RBX;
  static constexpr Register kResultReg = RAX;
};

// ABI for DoubleToIntegerStub.
struct DoubleToIntegerStubABI {
  static constexpr FpuRegister kInputReg = XMM0;
  static constexpr Register kRecognizedKindReg = RAX;
  static constexpr Register kResultReg = RAX;
};

// ABI for SuspendStub (AwaitStub, AwaitWithTypeCheckStub, YieldAsyncStarStub,
// SuspendSyncStarAtStartStub, SuspendSyncStarAtYieldStub).
struct SuspendStubABI {
  static constexpr Register kArgumentReg = RAX;
  static constexpr Register kTypeArgsReg = RDX;  // Can be the same as kTempReg
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kTempReg =
      dart::compiler::LikeABI::SuspendStubABI_kTempReg;
  static constexpr dart::compiler::LikeABI kFrameSizeReg =
      dart::compiler::LikeABI::SuspendStubABI_kFrameSizeReg;
  static constexpr dart::compiler::LikeABI kSuspendStateReg =
      dart::compiler::LikeABI::SuspendStubABI_kSuspendStateReg;
  static constexpr dart::compiler::LikeABI kFunctionDataReg =
      dart::compiler::LikeABI::SuspendStubABI_kFunctionDataReg;
  static constexpr dart::compiler::LikeABI kSrcFrameReg =
      dart::compiler::LikeABI::SuspendStubABI_kSrcFrameReg;
  static constexpr dart::compiler::LikeABI kDstFrameReg =
      dart::compiler::LikeABI::SuspendStubABI_kDstFrameReg;
#else
  static constexpr dart::compiler::LikeABI kTempReg = dart::compiler::LikeABI::RDX;
  static constexpr dart::compiler::LikeABI kFrameSizeReg = dart::compiler::LikeABI::RCX;
  static constexpr dart::compiler::LikeABI kSuspendStateReg = dart::compiler::LikeABI::RBX;
  static constexpr dart::compiler::LikeABI kFunctionDataReg = dart::compiler::LikeABI::R8;
  static constexpr dart::compiler::LikeABI kSrcFrameReg = dart::compiler::LikeABI::RSI;
  static constexpr dart::compiler::LikeABI kDstFrameReg = dart::compiler::LikeABI::RDI;
#endif
  // Number of bytes to skip after
  // suspend stub return address in order to resume.
  // X64: mov rsp, rbp; pop rbp; ret
  static constexpr intptr_t kResumePcDistance = 5;
};

// ABI for InitSuspendableFunctionStub (InitAsyncStub, InitAsyncStarStub,
// InitSyncStarStub).
struct InitSuspendableFunctionStubABI {
  static constexpr Register kTypeArgsReg = RAX;
};

// ABI for ResumeStub
struct ResumeStubABI {
  static constexpr Register kSuspendStateReg = RBX;
  static constexpr Register kTempReg = RDX;
  // Registers for the frame copying (the 1st part).
  static constexpr Register kFrameSizeReg = RCX;
  static constexpr Register kSrcFrameReg = RSI;
  static constexpr Register kDstFrameReg = RDI;
  // Registers for control transfer.
  // (the 2nd part, can reuse registers from the 1st part)
  static constexpr Register kResumePcReg = RCX;
  // Can also reuse kSuspendStateReg but should not conflict with CODE_REG/PP.
  static constexpr Register kExceptionReg = RSI;
  static constexpr Register kStackTraceReg = RDI;
};

// ABI for ReturnStub (ReturnAsyncStub, ReturnAsyncNotFutureStub,
// ReturnAsyncStarStub).
struct ReturnStubABI {
  static constexpr Register kSuspendStateReg = RBX;
};

// ABI for AsyncExceptionHandlerStub.
struct AsyncExceptionHandlerStubABI {
  static constexpr Register kSuspendStateReg = RBX;
};

// ABI for CloneSuspendStateStub.
struct CloneSuspendStateStubABI {
  static constexpr Register kSourceReg = RAX;
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kDestinationReg =
      dart::compiler::LikeABI::CloneSuspendStateStubABI_kDestinationReg;
  static constexpr dart::compiler::LikeABI kTempReg =
      dart::compiler::LikeABI::CloneSuspendStateStubABI_kTempReg;
  static constexpr dart::compiler::LikeABI kFrameSizeReg =
      dart::compiler::LikeABI::CloneSuspendStateStubABI_kFrameSizeReg;
  static constexpr dart::compiler::LikeABI kSrcFrameReg =
      dart::compiler::LikeABI::CloneSuspendStateStubABI_kSrcFrameReg;
  static constexpr dart::compiler::LikeABI kDstFrameReg =
      dart::compiler::LikeABI::CloneSuspendStateStubABI_kDstFrameReg;
#else
  static constexpr dart::compiler::LikeABI kDestinationReg = dart::compiler::LikeABI::RBX;
  static constexpr dart::compiler::LikeABI kTempReg = dart::compiler::LikeABI::RDX;
  static constexpr dart::compiler::LikeABI kFrameSizeReg = dart::compiler::LikeABI::RCX;
  static constexpr dart::compiler::LikeABI kSrcFrameReg = dart::compiler::LikeABI::RSI;
  static constexpr dart::compiler::LikeABI kDstFrameReg = dart::compiler::LikeABI::RDI;
#endif
};

// ABI for FfiAsyncCallbackSendStub.
struct FfiAsyncCallbackSendStubABI {
  static constexpr Register kArgsReg = RAX;
};

// ABI for DispatchTableNullErrorStub and consequently for all dispatch
// table calls (though normal functions will not expect or use this
// register). This ABI is added to distinguish memory corruption errors from
// null errors.
struct DispatchTableNullErrorABI {
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kClassIdReg =
      dart::compiler::LikeABI::DispatchTableNullErrorABI_kClassIdReg;
#else
  static constexpr dart::compiler::LikeABI kClassIdReg = dart::compiler::LikeABI::RCX;
#endif
};

typedef uint32_t RegList;
const RegList kAllCpuRegistersList = 0xFFFF;
const RegList kAllFpuRegistersList = 0xFFFF;

const RegList kReservedCpuRegisters =
    R(SPREG) | R(FPREG) | R(TMP) | R(R15) | R(THR);
constexpr intptr_t kNumberOfReservedCpuRegisters = 5;
// CPU registers available to Dart allocator.
const RegList kDartAvailableCpuRegs =
    kAllCpuRegistersList & ~kReservedCpuRegisters;
constexpr int kNumberOfDartAvailableCpuRegs =
    kNumberOfCpuRegisters - kNumberOfReservedCpuRegisters;
// Low numbered registers sometimes require fewer prefixes.
constexpr int kRegisterAllocationBias = 0;
constexpr int kStoreBufferWrapperSize = 15;

#if defined(DART_TARGET_OS_WINDOWS)
const RegList kAbiPreservedCpuRegs =
    R(RBX) | R(RSI) | R(RDI) | R(R12) | R(R13) | R(R14) | R(R15);
const RegList kAbiVolatileFpuRegs =
    R(XMM0) | R(XMM1) | R(XMM2) | R(XMM3) | R(XMM4) | R(XMM5);
#else
const RegList kAbiPreservedCpuRegs = R(RBX) | R(R12) | R(R13) | R(R14) | R(R15);
const RegList kAbiVolatileFpuRegs = kAllFpuRegistersList;
#endif

// Registers available to Dart that are not preserved by runtime calls.
const RegList kDartVolatileCpuRegs =
    kDartAvailableCpuRegs & ~kAbiPreservedCpuRegs;

enum ScaleFactor {
  TIMES_1 = 0,
  TIMES_2 = 1,
  TIMES_4 = 2,
  TIMES_8 = 3,
  // Note that Intel addressing does not support this addressing.
  // > Scale factor — A value of 2, 4, or 8 that is multiplied by the index
  // > value.
  // https://software.intel.com/en-us/download/intel-64-and-ia-32-architectures-sdm-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4
  // 3.7.5 Specifying an Offset
  TIMES_16 = 4,
// We can't include vm/compiler/runtime_api.h, so just be explicit instead
// of using (dart::)kWordSizeLog2.
#if defined(TARGET_ARCH_IS_64_BIT)
  // Used for Smi-boxed indices.
  TIMES_HALF_WORD_SIZE = kInt64SizeLog2 - 1,
  // Used for unboxed indices.
  TIMES_WORD_SIZE = kInt64SizeLog2,
#else
#error "Unexpected word size"
#endif
#if !defined(DART_COMPRESSED_POINTERS)
  TIMES_COMPRESSED_WORD_SIZE = TIMES_WORD_SIZE,
#else
  TIMES_COMPRESSED_WORD_SIZE = TIMES_HALF_WORD_SIZE,
#endif
  // Used for Smi-boxed indices.
  TIMES_COMPRESSED_HALF_WORD_SIZE = TIMES_COMPRESSED_WORD_SIZE - 1,
  TIMES_2_IA32_4_X64 = TIMES_4,
  TIMES_4_IA32_8_X64 = TIMES_8,
};

class CallingConventions {
 public:
#if defined(DART_TARGET_OS_WINDOWS)
  static constexpr Register kArg1Reg = RCX;
  static constexpr Register kArg2Reg = RDX;
  static constexpr Register kArg3Reg = R8;
  static constexpr Register kArg4Reg = R9;
  static const Register ArgumentRegisters[];
  static constexpr intptr_t kArgumentRegisters =
      R(kArg1Reg) | R(kArg2Reg) | R(kArg3Reg) | R(kArg4Reg);
  static constexpr intptr_t kNumArgRegs = 4;
  static constexpr Register kPointerToReturnStructRegisterCall = kArg1Reg;

  static const XmmRegister FpuArgumentRegisters[];
  static constexpr intptr_t kFpuArgumentRegisters =
      R(XMM0) | R(XMM1) | R(XMM2) | R(XMM3);
  static constexpr intptr_t kNumFpuArgRegs = 4;

  // Whether ArgumentRegisters[i] prevents using XmmArgumentRegisters[i] at the
  // same time and vice versa.
  static constexpr bool kArgumentIntRegXorFpuReg = true;

  // AL not set on vararg calls in Windows.
  static constexpr Register kVarArgFpuRegisterCount = kNoRegister;

  // > The x64 Application Binary Interface (ABI) uses a four-register
  // > fast-call calling convention by default. Space is allocated on the call
  // > stack as a shadow store for callees to save those registers.
  // https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-160
  //
  // The caller allocates this space. The caller should also reclaim this space
  // after the call to restore the stack to its original state if needed.
  //
  // This is also known as home space.
  // https://devblogs.microsoft.com/oldnewthing/20160623-00/?p=93735
  static constexpr intptr_t kShadowSpaceBytes = 4 * kWordSize;

  static constexpr intptr_t kVolatileCpuRegisters =
      R(RAX) | R(RCX) | R(RDX) | R(R8) | R(R9) | R(R10) | R(R11);

  static constexpr RegList kVolatileXmmRegisters = kAbiVolatileFpuRegs;

  static constexpr intptr_t kCalleeSaveXmmRegisters =
      R(XMM6) | R(XMM7) | R(XMM8) | R(XMM9) | R(XMM10) | R(XMM11) | R(XMM12) |
      R(XMM13) | R(XMM14) | R(XMM15);

  static constexpr XmmRegister xmmFirstNonParameterReg = XMM4;

  // Windows x64 ABI specifies that small objects are passed in registers.
  // Otherwise they are passed by reference.
  static const size_t kRegisterTransferLimit = 16;

  static constexpr Register kReturnReg = RAX;
  static constexpr Register kSecondReturnReg = RDX;
  static constexpr FpuRegister kReturnFpuReg = XMM0;
  static constexpr Register kPointerToReturnStructRegisterReturn = kReturnReg;

  // Whether larger than wordsize arguments are aligned to even registers.
  static constexpr AlignmentStrategy kArgumentRegisterAlignment =
      kAlignedToWordSize;
  static constexpr AlignmentStrategy kArgumentRegisterAlignmentVarArgs =
      kArgumentRegisterAlignment;

  // How stack arguments are aligned.
  static constexpr AlignmentStrategy kArgumentStackAlignment =
      kAlignedToWordSize;

  // How fields in compounds are aligned.
  static constexpr AlignmentStrategy kFieldAlignment = kAlignedToValueSize;

  // Whether 1 or 2 byte-sized arguments or return values are passed extended
  // to 4 bytes.
  static constexpr ExtensionStrategy kReturnRegisterExtension = kNotExtended;
  static constexpr ExtensionStrategy kArgumentRegisterExtension = kNotExtended;
  static constexpr ExtensionStrategy kArgumentStackExtension = kNotExtended;

#else
  static constexpr Register kArg1Reg = RDI;
  static constexpr Register kArg2Reg = RSI;
  static constexpr Register kArg3Reg = RDX;
  static constexpr Register kArg4Reg = RCX;
  static constexpr Register kArg5Reg = R8;
  static constexpr Register kArg6Reg = R9;
  static const Register ArgumentRegisters[];
  static constexpr intptr_t kArgumentRegisters = R(kArg1Reg) | R(kArg2Reg) |
                                                 R(kArg3Reg) | R(kArg4Reg) |
                                                 R(kArg5Reg) | R(kArg6Reg);
  static constexpr intptr_t kNumArgRegs = 6;
  static constexpr Register kPointerToReturnStructRegisterCall = kArg1Reg;

  static const XmmRegister FpuArgumentRegisters[];
  static constexpr intptr_t kFpuArgumentRegisters =
      R(XMM0) | R(XMM1) | R(XMM2) | R(XMM3) | R(XMM4) | R(XMM5) | R(XMM6) |
      R(XMM7);
  static constexpr intptr_t kNumFpuArgRegs = 8;

  // Whether ArgumentRegisters[i] prevents using XmmArgumentRegisters[i] at the
  // same time and vice versa.
  static constexpr bool kArgumentIntRegXorFpuReg = false;

  // > For calls that may call functions that use varargs or stdargs
  // > (prototype-less calls or calls to functions containing ellipsis (...) in
  // > the declaration) %al16 is used as hidden argument to specify the number
  // > of vector registers used. The contents of %al do not need to match
  // > exactly the number of registers, but must be an upper bound on the number
  // > of vector registers used and is in the range 0–8 inclusive.
  // System V ABI spec.
  static constexpr Register kVarArgFpuRegisterCount = RAX;

  static constexpr intptr_t kShadowSpaceBytes = 0;

  static constexpr intptr_t kVolatileCpuRegisters = R(RAX) | R(RCX) | R(RDX) |
                                                    R(RSI) | R(RDI) | R(R8) |
                                                    R(R9) | R(R10) | R(R11);

  static constexpr RegList kVolatileXmmRegisters = kAbiVolatileFpuRegs;

  static constexpr intptr_t kCalleeSaveXmmRegisters = 0;

  static constexpr XmmRegister xmmFirstNonParameterReg = XMM8;

  static constexpr Register kReturnReg = RAX;
  static constexpr Register kSecondReturnReg = RDX;
  static constexpr FpuRegister kReturnFpuReg = XMM0;
  static constexpr FpuRegister kSecondReturnFpuReg = XMM1;
  static constexpr Register kPointerToReturnStructRegisterReturn = kReturnReg;

  // Whether larger than wordsize arguments are aligned to even registers.
  static constexpr AlignmentStrategy kArgumentRegisterAlignment =
      kAlignedToWordSize;
  static constexpr AlignmentStrategy kArgumentRegisterAlignmentVarArgs =
      kArgumentRegisterAlignment;

  // How stack arguments are aligned.
  static constexpr AlignmentStrategy kArgumentStackAlignment =
      kAlignedToWordSize;

  // How fields in compounds are aligned.
  static constexpr AlignmentStrategy kFieldAlignment = kAlignedToValueSize;

  // Whether 1 or 2 byte-sized arguments or return values are passed extended
  // to 4 bytes.
  // Note that `kReturnRegisterExtension != kArgumentRegisterExtension`, which
  // effectively means that the caller is responsable for truncating and
  // extending both arguments and return value.
  static constexpr ExtensionStrategy kReturnRegisterExtension = kNotExtended;
  static constexpr ExtensionStrategy kArgumentRegisterExtension = kExtendedTo4;
  static constexpr ExtensionStrategy kArgumentStackExtension = kExtendedTo4;

#endif

  static constexpr intptr_t kCalleeSaveCpuRegisters = kAbiPreservedCpuRegs;

  COMPILE_ASSERT((kArgumentRegisters & kReservedCpuRegisters) == 0);

  static constexpr Register kFfiAnyNonAbiRegister = R12;
  static constexpr Register kFirstNonArgumentRegister = RAX;
  static constexpr Register kSecondNonArgumentRegister = RBX;
  static constexpr Register kStackPointerRegister = SPREG;

  COMPILE_ASSERT(((R(kFfiAnyNonAbiRegister)) & kCalleeSaveCpuRegisters) != 0);

  COMPILE_ASSERT(
      ((R(kFirstNonArgumentRegister) | R(kSecondNonArgumentRegister)) &
       (kArgumentRegisters | R(kPointerToReturnStructRegisterCall))) == 0);
};

#undef R

class Instr {
 public:
  static constexpr uint8_t kHltInstruction = 0xF4;
  // We prefer not to use the int3 instruction since it conflicts with gdb.
  static constexpr uint8_t kBreakPointInstruction = kHltInstruction;
  static constexpr int kBreakPointInstructionSize = 1;
  static constexpr uint8_t kGdbBreakpointInstruction = 0xcc;

  bool IsBreakPoint() {
    ASSERT(kBreakPointInstructionSize == 1);
    return (*reinterpret_cast<const uint8_t*>(this)) == kBreakPointInstruction;
  }

  // Instructions are read out of a code stream. The only way to get a
  // reference to an instruction is to convert a pointer. There is no way
  // to allocate or create instances of class Instr.
  // Use the At(pc) function to create references to Instr.
  static Instr* At(uword pc) { return reinterpret_cast<Instr*>(pc); }

 private:
  DISALLOW_ALLOCATION();
  // We need to prevent the creation of instances of class Instr.
  DISALLOW_IMPLICIT_CONSTRUCTORS(Instr);
};

// The largest multibyte nop we will emit.  This could go up to 15 if it
// becomes important to us.
const int MAX_NOP_SIZE = 8;

const uint64_t kBreakInstructionFiller = 0xCCCCCCCCCCCCCCCCL;

}  // namespace dart

#endif  // RUNTIME_VM_CONSTANTS_X64_H_
