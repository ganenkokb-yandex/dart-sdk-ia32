
#ifndef RUNTIME_VM_COMPILER_STUB_CODE_COMPILER_IA32_X64_H_
#define RUNTIME_VM_COMPILER_STUB_CODE_COMPILER_IA32_X64_H_

#if defined(TARGET_ARCH_IA32)
namespace dart {
namespace compiler {

void GenerateWriteBarrierStubHelper(Assembler* assembler, bool cards);
void PushArrayOfArguments(Assembler* assembler);

}  // namespace compiler
}  // namespace dart
#endif  // #if defined(TARGET_ARCH_IA32)

#endif  // RUNTIME_VM_COMPILER_STUB_CODE_COMPILER_IA32_X64_H_