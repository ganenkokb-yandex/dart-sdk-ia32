// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_VM_CONSTANTS_IA32_H_
#define RUNTIME_VM_CONSTANTS_IA32_H_

#ifndef RUNTIME_VM_CONSTANTS_H_
#error Do not include constants_ia32.h directly; use constants.h instead.
#endif

#include "platform/assert.h"
#include "platform/globals.h"
#include "platform/utils.h"

#include "vm/compiler/like_abi.h"
#include "vm/constants_base.h"
#include "vm/globals.h"

namespace dart {

#define R(reg) (1 << (reg))

enum Register {
  EAX = 0,
  ECX = 1,
  EDX = 2,
  EBX = 3,
  ESP = 4,  // SP
  EBP = 5,  // FP
  ESI = 6,  // THR
  EDI = 7,
  RAX = EAX,
  RCX = ECX,
  RDX = EDX,
  RBX = EBX,
  RSP = ESP,
  RBP = EBP,
  RSI = ESI,
  RDI = EDI,
  kNumberOfCpuRegisters = 8,
  kNoRegister = -1,  // Signals an illegal register.
};

// Low and high bytes registers of the first four general purpose registers.
// The other four general purpose registers do not have byte registers.
enum ByteRegister {
  AL = 0,
  CL = 1,
  DL = 2,
  BL = 3,
  AH = 4,
  CH = 5,
  DH = 6,
  BH = 7,
  kNumberOfByteRegisters = 8,
  kNoByteRegister = -1  // Signals an illegal register.
};

inline constexpr Register FindOrigReg(dart::compiler::LikeABI like_abi) {
  switch (like_abi) {
    case dart::compiler::LikeABI::kWriteBarrierObjectReg:
      return Register::EDX;
    case dart::compiler::LikeABI::kWriteBarrierValueReg:
      return Register::EBX;
    case dart::compiler::LikeABI::kWriteBarrierSlotReg:
      return Register::EDI;
    case dart::compiler::LikeABI::TypeTestABI_kDstTypeReg:
      return Register::EBX;
    case dart::compiler::LikeABI::TypeTestABI_kInstantiatorTypeArgumentsReg:
      return Register::EDX;
    case dart::compiler::LikeABI::TypeTestABI_kFunctionTypeArgumentsReg:
      return Register::ECX;
    case dart::compiler::LikeABI::TTABI_kSubtypeTestCache:
      return Register::EDI;
    case dart::compiler::LikeABI::AllocateClosureABI_kFunctionReg:
      return Register::EBX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kSubTypeReg:
      return Register::EAX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kSuperTypeReg:
      return Register::EBX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kInstantiatorTypeArgumentsReg:
      return Register::EDX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kFunctionTypeArgumentsReg:
      return Register::ECX;
    case dart::compiler::LikeABI::AssertSubtypeABI_kDstNameReg:
      return Register::EDI;
    case dart::compiler::LikeABI::DispatchTableNullErrorABI_kClassIdReg:
      return Register::EAX;
    case dart::compiler::LikeABI::AllocateObjectABI_kTypeArgumentsReg:
      return Register::EDX;
    default:
      RELEASE_ASSERT(false);
      return kNoRegister;
  }
}

inline ByteRegister ByteRegisterOf(Register reg) {
  // This only works for EAX, ECX, EDX, EBX.
  // Remaining Register values map to high byte of the above registers.
  RELEASE_ASSERT(reg == Register::EAX || reg == Register::ECX ||
                 reg == Register::EDX || reg == Register::EBX);
  return static_cast<ByteRegister>(reg);
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
  kNumberOfXmmRegisters = 8,
  kNoXmmRegister = -1  // Signals an illegal register.
};

// Architecture independent aliases.
typedef XmmRegister FpuRegister;
const FpuRegister FpuTMP = XMM7;
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
// TODO(xafster): Remove TMP
const Register TMP = kNoRegister;   // No scratch register used by assembler.
const Register TMP2 = kNoRegister;  // No second assembler scratch register.
#if USE_LIKE_ABI_SEPARATED
const dart::compiler::LikeABI PP = dart::compiler::LikeABI::PP;
#else
const dart::compiler::LikeABI PP = dart::compiler::LikeABI::kNoRegister;
#endif
// Set when calling Dart functions in JIT mode, used by LazyCompileStub.
const Register FUNCTION_REG = EAX;
const Register SPREG = ESP;          // Stack pointer register.
const Register FPREG = EBP;          // Frame pointer register.
const Register RECEIVER_REG = EBX;
const Register IC_DATA_REG = ECX;    // ICData/MegamorphicCache register.
const Register ARGS_DESC_REG = EDX;  // Arguments descriptor register.
#if USE_LIKE_ABI_SEPARATED
const dart::compiler::LikeABI CODE_REG = dart::compiler::LikeABI::CODE;
#else
const dart::compiler::LikeABI CODE_REG = dart::compiler::LikeABI::R12;
#endif
const Register THR = ESI;            // Caches current thread in generated code.
const Register CALLEE_SAVED_TEMP = EBX;
const Register CALLEE_SAVED_TEMP2 = EDI;
const Register RAX_FOR_SCOPED = EAX;
const Register RCX_FOR_SCOPED = ECX;
const Register RDX_FOR_SCOPED = EDX;
const Register RBX_FOR_SCOPED = EBX;

// ABI for catch-clause entry point.
const Register kExceptionObjectReg = EAX;
const Register kStackTraceObjectReg = EDX;

// ABI for write barrier stub.
#if USE_LIKE_ABI_SEPARATED
const dart::compiler::LikeABI kWriteBarrierObjectReg =
    dart::compiler::LikeABI::kWriteBarrierObjectReg;
const dart::compiler::LikeABI kWriteBarrierValueReg =
    dart::compiler::LikeABI::kWriteBarrierValueReg;
const dart::compiler::LikeABI kWriteBarrierSlotReg =
    dart::compiler::LikeABI::kWriteBarrierSlotReg;
#else
const dart::compiler::LikeABI kWriteBarrierObjectReg = dart::compiler::LikeABI::EDX;
const dart::compiler::LikeABI kWriteBarrierValueReg = dart::compiler::LikeABI::EBX;
const dart::compiler::LikeABI kWriteBarrierSlotReg = dart::compiler::LikeABI::EDI;
#endif

// Common ABI for shared slow path stubs.
struct SharedSlowPathStubABI {
  static constexpr Register kResultReg = EAX;
};

// ABI for instantiation stubs.
struct InstantiationABI {
  static constexpr Register kUninstantiatedTypeArgumentsReg = EBX;
  static constexpr Register kInstantiatorTypeArgumentsReg = EDX;
  static constexpr Register kFunctionTypeArgumentsReg = ECX;
  static constexpr Register kResultTypeArgumentsReg = EAX;
  static constexpr Register kResultTypeReg = EAX;
  static constexpr Register kScratchReg =
      EDI;  // On ia32 we don't use CODE_REG.
};

// Registers in addition to those listed in InstantiationABI used inside the
// implementation of the InstantiateTypeArguments stubs.
struct InstantiateTAVInternalRegs {
  // On IA32, we don't do hash cache checks in the stub. We only define
  // kSavedRegisters to avoid needing to #ifdef uses of it.
  static constexpr intptr_t kSavedRegisters = 0;
};

// Registers in addition to those listed in TypeTestABI used inside the
// implementation of type testing stubs that are _not_ preserved.
struct TTSInternalRegs {
  static const Register kInstanceTypeArgumentsReg = EBX;
  // static const Register kScratchReg = R8;
  static const Register kScratchReg = EDI;
  // static const Register kSubTypeArgumentReg = R10;
  static const Register kSubTypeArgumentReg = EDX;
  // static const Register kSuperTypeArgumentReg = R13;
  static const Register kSuperTypeArgumentReg = ECX;

  // Must be pushed/popped whenever generic type arguments are being checked as
  // they overlap with registers in TypeTestABI.
  static const intptr_t kSavedTypeArgumentRegisters = 0;

  static const intptr_t kInternalRegisters =
      ((1 << kInstanceTypeArgumentsReg) | (1 << kScratchReg) |
       (1 << kSubTypeArgumentReg) | (1 << kSuperTypeArgumentReg)) &
      ~kSavedTypeArgumentRegisters;
};

// Calling convention when calling SubtypeTestCacheStub.
// Although ia32 uses a stack-based calling convention, we keep the same
// 'TypeTestABI' name for symmetry with other architectures with a proper ABI.
// Note that ia32 has no support for type testing stubs.
struct TypeTestABI {
  static constexpr Register kInstanceReg = EAX;
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
  static constexpr dart::compiler::LikeABI kDstTypeReg = dart::compiler::LikeABI::EBX;
  static constexpr dart::compiler::LikeABI kInstantiatorTypeArgumentsReg = dart::compiler::LikeABI::EDX;
  static constexpr dart::compiler::LikeABI kFunctionTypeArgumentsReg = dart::compiler::LikeABI::ECX;
  static constexpr dart::compiler::LikeABI kSubtypeTestCacheReg =
      dart::compiler::LikeABI::EDI;  // On ia32 we don't use CODE_REG.
#endif // 0

  // For call to InstanceOfStub.
  static constexpr Register kInstanceOfResultReg = kInstanceReg;
  // For call to SubtypeNTestCacheStub.
  // static const Register kSubtypeTestCacheResultReg = EDI;
  //     TypeTestABI::kSubtypeTestCacheReg;
  // No registers need saving across SubtypeTestCacheStub calls.
  // static const intptr_t kSubtypeTestCacheStubCallerSavedRegisters =
  //     1 << kSubtypeTestCacheResultReg;

  static const intptr_t kPreservedAbiRegisters =
      (1 << kInstanceReg);

  static const intptr_t kNonPreservedAbiRegisters =
      TTSInternalRegs::kInternalRegisters;

  static const intptr_t kAbiRegisters =
      kPreservedAbiRegisters | kNonPreservedAbiRegisters;
};

// For calling the ia32-specific AssertAssignableStub
struct AssertAssignableStubABI {
  static constexpr Register kDstNameReg = EBX;
  static constexpr Register kSubtypeTestReg = ECX;

  static constexpr intptr_t kInstanceSlotFromFp = 2 + 3;
  static constexpr intptr_t kDstTypeSlotFromFp = 2 + 2;
  static constexpr intptr_t kInstantiatorTAVSlotFromFp = 2 + 1;
  static constexpr intptr_t kFunctionTAVSlotFromFp = 2 + 0;
};

// ABI for InitStaticFieldStub.
struct InitStaticFieldABI {
  static constexpr Register kFieldReg = EDX;
  static constexpr Register kResultReg = EAX;
};

// Registers used inside the implementation of InitLateStaticFieldStub.
struct InitLateStaticFieldInternalRegs {
  static constexpr Register kAddressReg = ECX;
  static constexpr Register kScratchReg = EDI;
};

// ABI for InitInstanceFieldStub.
struct InitInstanceFieldABI {
  static constexpr Register kInstanceReg = EBX;
  static constexpr Register kFieldReg = EDX;
  static constexpr Register kResultReg = EAX;
};

// Registers used inside the implementation of InitLateInstanceFieldStub.
struct InitLateInstanceFieldInternalRegs {
  static constexpr Register kAddressReg = ECX;
  static constexpr Register kScratchReg = EDI;
};

// ABI for LateInitializationError stubs.
struct LateInitializationErrorABI {
  static constexpr Register kFieldReg = EDI;
};

// ABI for ThrowStub.
struct ThrowABI {
  static constexpr Register kExceptionReg = EAX;
};

// ABI for ReThrowStub.
struct ReThrowABI {
  static constexpr Register kExceptionReg = EAX;
  static constexpr Register kStackTraceReg = EBX;
};

// ABI for AssertBooleanStub.
struct AssertBooleanABI {
  static constexpr Register kObjectReg = EAX;
};

// ABI for RangeErrorStub.
struct RangeErrorABI {
  static constexpr Register kLengthReg = EAX;
  static constexpr Register kIndexReg = EBX;
};

// ABI for AllocateObjectStub.
struct AllocateObjectABI {
  static const Register kResultReg = EAX;
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kTypeArgumentsReg =
      dart::compiler::LikeABI::AllocateObjectABI_kTypeArgumentsReg;
  static constexpr dart::compiler::LikeABI kTagsReg =
      dart::compiler::LikeABI::AllocateObjectABI_kTagsReg;
#else
  static constexpr dart::compiler::LikeABI kTypeArgumentsReg = dart::compiler::LikeABI::EDX;
  static constexpr dart::compiler::LikeABI kTagsReg = dart::compiler::LikeABI::kNoRegister;
#endif
};

// ABI for Allocate{Mint,Double,Float32x4,Float64x2}Stub.
struct AllocateBoxABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kTempReg = EBX;
};

// ABI for AllocateClosureStub.
struct AllocateClosureABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kFunctionReg =
      dart::compiler::LikeABI::AllocateClosureABI_kFunctionReg;
#else
  static constexpr dart::compiler::LikeABI kFunctionReg = dart::compiler::LikeABI::EBX;
#endif
  static constexpr Register kContextReg = ECX;
  static constexpr Register kScratchReg = EDX;
};

// ABI for AllocateMintShared*Stub.
struct AllocateMintABI {
  static const Register kResultReg = AllocateObjectABI::kResultReg;
  static const Register kTempReg = EBX;
};

// ABI for AllocateArrayStub.
struct AllocateArrayABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kLengthReg = EDX;
  static constexpr Register kTypeArgumentsReg = ECX;
};

// ABI for AllocateRecordStub.
struct AllocateRecordABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kShapeReg = EDX;
  static constexpr Register kTemp1Reg = EBX;
  static constexpr Register kTemp2Reg = EDI;
};

// ABI for AllocateSmallRecordStub (AllocateRecord2, AllocateRecord2Named,
// AllocateRecord3, AllocateRecord3Named).
struct AllocateSmallRecordABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kShapeReg = EBX;
  static constexpr Register kValue0Reg = ECX;
  static constexpr Register kValue1Reg = EDX;
  static constexpr Register kValue2Reg = kNoRegister;
  static constexpr Register kTempReg = EDI;
};

// ABI for AllocateTypedDataArrayStub.
struct AllocateTypedDataArrayABI {
  static constexpr Register kResultReg = AllocateObjectABI::kResultReg;
  static constexpr Register kLengthReg = kResultReg;
};

// ABI for BoxDoubleStub.
struct BoxDoubleStubABI {
  static constexpr FpuRegister kValueReg = XMM0;
  static constexpr Register kTempReg = EBX;
  static constexpr Register kResultReg = EAX;
};

// ABI for DoubleToIntegerStub.
struct DoubleToIntegerStubABI {
  static constexpr FpuRegister kInputReg = XMM0;
  static constexpr Register kRecognizedKindReg = EAX;
  static constexpr Register kResultReg = EAX;
};

// ABI for SuspendStub (AwaitStub, AwaitWithTypeCheckStub, YieldAsyncStarStub,
// SuspendSyncStarAtStartStub, SuspendSyncStarAtYieldStub).
struct SuspendStubABI {
  static constexpr Register kArgumentReg = EAX;
  static constexpr Register kTypeArgsReg = EDX;  // Can be the same as kTempReg
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
  static constexpr dart::compiler::LikeABI kTempReg = dart::compiler::LikeABI::EDX;
  static constexpr dart::compiler::LikeABI kFrameSizeReg = dart::compiler::LikeABI::ECX;
  static constexpr dart::compiler::LikeABI kSuspendStateReg = dart::compiler::LikeABI::EBX;
  static constexpr dart::compiler::LikeABI kFunctionDataReg = dart::compiler::LikeABI::EDI;
  static constexpr dart::compiler::LikeABI kSrcFrameReg = dart::compiler::LikeABI::ESI;
  static constexpr dart::compiler::LikeABI kDstFrameReg = dart::compiler::LikeABI::EDI;
#endif
  // Number of bytes to skip after
  // suspend stub return address in order to resume.
  // IA32: mov esp, ebp; pop ebp; ret
  static constexpr intptr_t kResumePcDistance = 4;
};

// ABI for InitSuspendableFunctionStub (InitAsyncStub, InitAsyncStarStub,
// InitSyncStarStub).
struct InitSuspendableFunctionStubABI {
  static constexpr Register kTypeArgsReg = EAX;
};

// ABI for ResumeStub
struct ResumeStubABI {
  static constexpr Register kSuspendStateReg = EBX;
  static constexpr Register kTempReg = EDX;
  // Registers for the frame copying (the 1st part).
  static constexpr Register kFrameSizeReg = ECX;
  // Can reuse THR.
  static constexpr Register kSrcFrameReg = ESI;
  // Can reuse CODE_REG.
  static constexpr Register kDstFrameReg = EDI;
  // Registers for control transfer.
  // (the 2nd part, can reuse registers from the 1st part)
  static constexpr Register kResumePcReg = ECX;
  // Can also reuse kSuspendStateReg but should not conflict with CODE_REG.
  static constexpr Register kExceptionReg = EAX;
  static constexpr Register kStackTraceReg = EBX;
};

// ABI for ReturnStub (ReturnAsyncStub, ReturnAsyncNotFutureStub,
// ReturnAsyncStarStub).
struct ReturnStubABI {
  static constexpr Register kSuspendStateReg = EBX;
};

// ABI for AsyncExceptionHandlerStub.
struct AsyncExceptionHandlerStubABI {
  static constexpr Register kSuspendStateReg = EBX;
};

// ABI for CloneSuspendStateStub.
struct CloneSuspendStateStubABI {
  static constexpr Register kSourceReg = EAX;
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
  static constexpr dart::compiler::LikeABI kDestinationReg = dart::compiler::LikeABI::EBX;
  static constexpr dart::compiler::LikeABI kTempReg = dart::compiler::LikeABI::EDX;
  static constexpr dart::compiler::LikeABI kFrameSizeReg = dart::compiler::LikeABI::ECX;
  static constexpr dart::compiler::LikeABI kSrcFrameReg = dart::compiler::LikeABI::ESI;
  static constexpr dart::compiler::LikeABI kDstFrameReg = dart::compiler::LikeABI::EDI;
#endif
};

// ABI for FfiAsyncCallbackSendStub.
struct FfiAsyncCallbackSendStubABI {
  static constexpr Register kArgsReg = EAX;
};

// ABI for DispatchTableNullErrorStub and consequently for all dispatch
// table calls (though normal functions will not expect or use this
// register). This ABI is added to distinguish memory corruption errors from
// null errors.
// Note: dispatch table calls are never actually generated on IA32, this
// declaration is only added for completeness.
struct DispatchTableNullErrorABI {
#if USE_LIKE_ABI_SEPARATED
  static constexpr dart::compiler::LikeABI kClassIdReg =
      dart::compiler::LikeABI::DispatchTableNullErrorABI_kClassIdReg;
#else
  static constexpr dart::compiler::LikeABI kClassIdReg = dart::compiler::LikeABI::EAX;
#endif
};

typedef uint32_t RegList;
const RegList kAllCpuRegistersList = 0xFF;
const RegList kAllFpuRegistersList = (1 << kNumberOfFpuRegisters) - 1;

const intptr_t kReservedCpuRegisters = (1 << SPREG) | (1 << FPREG) | (1 << THR);
constexpr intptr_t kNumberOfReservedCpuRegisters = 3;
// CPU registers available to Dart allocator.
const RegList kDartAvailableCpuRegs =
    kAllCpuRegistersList & ~kReservedCpuRegisters;
constexpr int kNumberOfDartAvailableCpuRegs =
    kNumberOfCpuRegisters - kNumberOfReservedCpuRegisters;
// No reason to prefer certain registers on IA32.
constexpr int kRegisterAllocationBias = 0;
constexpr int kStoreBufferWrapperSize = 13;

const RegList kAbiPreservedCpuRegs = (1 << EDI) | (1 << ESI) | (1 << EBX);

// Registers available to Dart that are not preserved by runtime calls.
const RegList kDartVolatileCpuRegs =
    kDartAvailableCpuRegs & ~kAbiPreservedCpuRegs;

const RegList kAbiVolatileFpuRegs = kAllFpuRegistersList;

#undef R

enum ScaleFactor {
  TIMES_1 = 0,
  TIMES_2 = 1,
  TIMES_4 = 2,
  TIMES_8 = 3,
  TIMES_16 = 4,
// We can't include vm/compiler/runtime_api.h, so just be explicit instead
// of using (dart::)kWordSizeLog2.
#if defined(TARGET_ARCH_IS_32_BIT)
  // Used for Smi-boxed indices.
  TIMES_HALF_WORD_SIZE = kInt32SizeLog2 - 1,
  // Used for unboxed indices.
  TIMES_WORD_SIZE = kInt32SizeLog2,
#else
#error "Unexpected word size"
#endif
#if !defined(DART_COMPRESSED_POINTERS)
  TIMES_COMPRESSED_WORD_SIZE = TIMES_WORD_SIZE,
#else
#error Cannot compress IA32
#endif
  // Used for Smi-boxed indices.
  TIMES_COMPRESSED_HALF_WORD_SIZE = TIMES_COMPRESSED_WORD_SIZE - 1,
  TIMES_2_IA32_4_X64 = TIMES_2,
  TIMES_4_IA32_8_X64 = TIMES_4,
};

class Instr {
 public:
  static constexpr uint8_t kHltInstruction = 0xF4;
  // We prefer not to use the int3 instruction since it conflicts with gdb.
  static constexpr uint8_t kBreakPointInstruction = kHltInstruction;
  static constexpr int kBreakPointInstructionSize = 1;

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

class CallingConventions {
 public:
  // TODO(xafster): to remove
  static const Register ArgumentRegisters[];

  static constexpr intptr_t kArgumentRegisters = 0;
  static constexpr intptr_t kFpuArgumentRegisters = 0;
  static constexpr intptr_t kNumArgRegs = 0;
#if 0
  static constexpr Register kPointerToReturnStructRegisterCall = kNoRegister;
#endif  // 0

  static const XmmRegister FpuArgumentRegisters[];
  static constexpr intptr_t kXmmArgumentRegisters = 0;
  static constexpr intptr_t kNumFpuArgRegs = 0;

  static constexpr intptr_t kCalleeSaveCpuRegisters = kAbiPreservedCpuRegs;

  static constexpr bool kArgumentIntRegXorFpuReg = false;

  static constexpr Register kReturnReg = EAX;
  static constexpr Register kSecondReturnReg = EDX;
  static constexpr Register kPointerToReturnStructRegisterReturn = kReturnReg;

  // Whether the callee uses `ret 4` instead of `ret` to return with struct
  // return values.
  // See: https://c9x.me/x86/html/file_module_x86_id_280.html
#if defined(_WIN32)
  static constexpr bool kUsesRet4 = false;
#else
  static constexpr bool kUsesRet4 = true;
#endif

  // Floating point values are returned on the "FPU stack" (in "ST" registers).
  // However, we use XMM0 in our compiler pipeline as the location.
  // The move from and to ST is done in FfiCallInstr::EmitNativeCode and
  // NativeReturnInstr::EmitNativeCode.
  static constexpr XmmRegister kReturnFpuReg = XMM0;

  static constexpr Register kFfiAnyNonAbiRegister = EBX;
  static constexpr Register kFirstNonArgumentRegister = EAX;
  static constexpr Register kSecondNonArgumentRegister = ECX;
  static constexpr Register kStackPointerRegister = SPREG;

  // Whether larger than wordsize arguments are aligned to even registers.
  static constexpr AlignmentStrategy kArgumentRegisterAlignment =
      kAlignedToWordSize;
  static constexpr AlignmentStrategy kArgumentRegisterAlignmentVarArgs =
      kArgumentRegisterAlignment;

  // How stack arguments are aligned.
  static constexpr AlignmentStrategy kArgumentStackAlignment =
      kAlignedToWordSize;

  // How fields in compounds are aligned.
#if defined(DART_TARGET_OS_WINDOWS)
  static constexpr AlignmentStrategy kFieldAlignment = kAlignedToValueSize;
#else
  static constexpr AlignmentStrategy kFieldAlignment =
      kAlignedToValueSizeBut8AlignedTo4;
#endif

  // Whether 1 or 2 byte-sized arguments or return values are passed extended
  // to 4 bytes.
  static constexpr ExtensionStrategy kReturnRegisterExtension = kNotExtended;
  static constexpr ExtensionStrategy kArgumentRegisterExtension = kNotExtended;
  static constexpr ExtensionStrategy kArgumentStackExtension = kExtendedTo4;
};

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
  static constexpr dart::compiler::LikeABI kSubTypeReg = dart::compiler::LikeABI::EAX;
  static constexpr dart::compiler::LikeABI kSuperTypeReg = dart::compiler::LikeABI::EBX;
  static constexpr dart::compiler::LikeABI kInstantiatorTypeArgumentsReg = dart::compiler::LikeABI::EDX;
  static constexpr dart::compiler::LikeABI kFunctionTypeArgumentsReg = dart::compiler::LikeABI::ECX;
  static constexpr dart::compiler::LikeABI kDstNameReg = dart::compiler::LikeABI::EDI;
#endif

  // No result register, as AssertSubtype is only run for side effect
  // (throws if the subtype check fails).
};

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
  static_assert(false && "Has no implementation");
#endif

  static constexpr intptr_t kInternalRegisters = 0;

#if 0
  static constexpr intptr_t kInternalRegisters =
      (1 << kCacheEntryReg) | (1 << kInstanceCidOrSignatureReg) |
      (1 << kInstanceInstantiatorTypeArgumentsReg);
#endif
};

const uword kBreakInstructionFiller = 0xCCCCCCCC;

}  // namespace dart

#endif  // RUNTIME_VM_CONSTANTS_IA32_H_
