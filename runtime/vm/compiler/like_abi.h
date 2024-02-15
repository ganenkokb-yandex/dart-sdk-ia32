#ifndef LIKE_ABI_H_
#define LIKE_ABI_H_

namespace dart {
namespace compiler {

enum class LikeABI {
  EAX = 0,
  ECX = 1,
  EDX = 2,
  EBX = 3,
  ESP = 4,
  EBP = 5,
  ESI = 6,
  EDI = 7,

  RAX = 0,
  RCX = 1,
  RDX = 2,
  RBX = 3,
  RSP = 4,
  RBP = 5,
  RSI = 6,
  RDI = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  R13 = 13,
  R14 = 14,
  R15 = 15,

  TypeTestABI_kDstTypeReg,                    // TypeTestABI::kDstTypeReg
  TypeTestABI_kInstantiatorTypeArgumentsReg,  // TypeTestABI::kInstantiatorTypeArgumentsReg
  TypeTestABI_kFunctionTypeArgumentsReg,  // TypeTestABI::kFunctionTypeArgumentsReg
  kWriteBarrierObjectReg,  // Write barrier ABI kWriteBarrierObjectReg
  kWriteBarrierValueReg,   // Write barrier ABI kWriteBarrierValueReg
  kWriteBarrierSlotReg,    // Write barrier ABI kWriteBarrierSlotReg
  AllocateObjectABI_kTypeArgumentsReg,  // AllocateObjectABI::kTypeArgumentsReg
  TraceAllocation_TmpReg,
  AllocateObjectABI_kTagsReg,       // AllocateObjectABI::kTagsReg
  AllocateClosureABI_kFunctionReg,  // AllocateClosureABI::kFunctionReg
  PP,
  CODE,
  TTABI_kScratch,           // TypeTestABI::kScratchReg
  TTABI_kSubtypeTestCache,  // TypeTestABI::kSubtypeTestCacheReg,
  TMP,
  AssertSubtypeABI_kSubTypeReg,    // AssertSubtypeABI::kSubTypeReg
  AssertSubtypeABI_kSuperTypeReg,  // AssertSubtypeABI::kSuperTypeReg
  AssertSubtypeABI_kInstantiatorTypeArgumentsReg,  // AssertSubtypeABI::kInstantiatorTypeArgumentsReg
  AssertSubtypeABI_kFunctionTypeArgumentsReg,  // AssertSubtypeABI::kFunctionTypeArgumentsReg
  AssertSubtypeABI_kDstNameReg,                // AssertSubtypeABI::kDstNameReg
  DispatchTableNullErrorABI_kClassIdReg,  // DispatchTableNullErrorABI::kClassIdReg

  STCInternalRegs_kCacheEntryReg,  // STCInternalRegs::kCacheEntryReg
  STCInternalRegs_kInstanceCidOrSignatureReg,  // STCInternalRegs::kInstanceCidOrSignatureReg
  STCInternalRegs_kInstanceInstantiatorTypeArgumentsReg,  // STCInternalRegs::kInstanceInstantiatorTypeArgumentsReg

  SuspendStubABI_kTempReg,          // SuspendStubABI::kTempReg
  SuspendStubABI_kFrameSizeReg,     // SuspendStubABI::kFrameSizeReg
  SuspendStubABI_kSuspendStateReg,  // SuspendStubABI::kSuspendStateReg
  SuspendStubABI_kFunctionDataReg,  // SuspendStubABI::kFunctionDataReg
  SuspendStubABI_kSrcFrameReg,      // SuspendStubABI::kSrcFrameReg
  SuspendStubABI_kDstFrameReg,      // SuspendStubABI::kDstFrameReg

  CloneSuspendStateStubABI_kDestinationReg,  // CloneSuspendStateStubABI::kDestinationReg
  CloneSuspendStateStubABI_kTempReg,       // CloneSuspendStateStubABI::kTempReg
  CloneSuspendStateStubABI_kFrameSizeReg,  // CloneSuspendStateStubABI::kFrameSizeReg
  CloneSuspendStateStubABI_kSrcFrameReg,  // CloneSuspendStateStubABI::kSrcFrameReg
  CloneSuspendStateStubABI_kDstFrameReg,  // CloneSuspendStateStubABI::kDstFrameReg

  TypeTestABI_kSubtypeTestCacheResultReg,  // TypeTestABI::kSubtypeTestCacheResultReg

  kNoRegister,
};

}  // namespace compiler
}  // namespace dart

#endif  // LIKE_ABI_H_
