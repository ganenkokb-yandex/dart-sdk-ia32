#ifndef LIKE_REGISTERS_H_
#define LIKE_REGISTERS_H_

#include "vm/compiler/like_abi.h"
#include "vm/constants.h"

namespace dart {
namespace compiler {

#if defined(TARGET_ARCH_X64)
const Register TMP_LIKE_REG = RAX;
#elif defined(TARGET_ARCH_IA32)
const Register TMP_LIKE_REG = EAX;
#endif

class Assembler;
class Address;

void CreateInstance(int pool_offset);

void DeleteInstance(int pool_offset);

template <typename T, int POOL_OFFSET>
class LikeRegisterScopedAccess;
template <typename T, int POOL_OFFSET>
class LikeRegisterScopedAccessRead;

template <typename T, int POOL_OFFSET = 0>
class LikeRegisters {
 public:
  static constexpr int pool_offset = POOL_OFFSET;
  using TScoped = LikeRegisterScopedAccess<T, POOL_OFFSET>;
  using TScopedRead = LikeRegisterScopedAccessRead<T, POOL_OFFSET>;
  LikeRegisters(Assembler* assembler) : assembler_(assembler) {
    CreateInstance(POOL_OFFSET);
  }

  ~LikeRegisters() { DeleteInstance(POOL_OFFSET); }

  Address address(T value) const {
    return Address(THR, target::Thread::memory_pool_offset() + POOL_OFFSET +
                            static_cast<int>(value) * target::kWordSize);
  }

  TScoped scoped(T value, Register reg) { return TScoped(*this, value, reg); }

  TScopedRead scoped_read(T value, Register reg) {
    return TScopedRead(*this, value, reg);
  }

  void write_to_return_reg(T value) {
    assembler()->movq(CallingConventions::kReturnReg, address(value));
  }

  Assembler* assembler() { return assembler_; }

  void Push(T value) {
    assembler()->pushq(TMP_LIKE_REG);
    assembler()->movq(TMP_LIKE_REG, address(value));
    assembler()->xchgq(TMP_LIKE_REG, Address(SPREG, 0));
  }

  void Push() {
    for (auto i = 0; i < static_cast<int>(T::Size); ++i) {
      Push(static_cast<T>(i));
    }
  }

  void Pop(T value) {
    assembler()->xchgq(TMP_LIKE_REG, Address(SPREG, 0));
    assembler()->movq(address(value), TMP_LIKE_REG);
    assembler()->popq(TMP_LIKE_REG);
  }

  void Pop() {
    for (int i = static_cast<int>(T::Size) - 1; i >= 0; --i) {
      Pop(static_cast<T>(i));
    }
  }

  void LeaFromBaseRSP(T value, ScaleFactor scale, int disp) {
    auto scoped_reg = scoped(value, RAX_FOR_SCOPED);
    assembler()->leaq(scoped_reg.Reg(), Address(SPREG, scoped_reg.Reg(), scale,
                                                disp + target::kWordSize));
  }

  // This value is destination for move.
  // Method only wraps address creation for safe
  void MovFromBaseRSP(T value, Register r, ScaleFactor scale, int disp) {
    ASSERT(r != RAX_FOR_SCOPED);
    auto scoped_reg = scoped(value, RAX_FOR_SCOPED);
    assembler()->movq(scoped_reg.Reg(),
                      Address(SPREG, r, scale, disp + target::kWordSize));
  }

  void MovFromBaseRSP(T value, int disp) {
    auto scoped_reg = scoped(value, RAX_FOR_SCOPED);
    assembler()->movq(scoped_reg.Reg(),
                      Address(SPREG, disp + target::kWordSize));
  }

  void MovToBaseRSP(T value, int disp) {
    auto scoped_reg = scoped(value, RAX_FOR_SCOPED);
    assembler()->movl(Address(SPREG, disp + target::kWordSize),
                      scoped_reg.Reg());
  }

  // This value is destination for move.
  // Method only wraps address creation for safe
  void MovFromBaseRSP(T value, const TScoped& r, ScaleFactor scale, int disp) {
    ASSERT(RAX_FOR_SCOPED != r.Reg());
    auto scoped_reg = scoped(value, RAX_FOR_SCOPED);
    assembler()->movq(scoped_reg.Reg(), Address(SPREG, r.Reg(), scale,
                                                disp + 2 * target::kWordSize));
  }

  void MovFromBaseRSP(Register dst, T value, ScaleFactor scale, int disp) {
    ASSERT(dst != RAX_FOR_SCOPED);
    auto scoped_reg = scoped(value, RAX_FOR_SCOPED);
    assembler()->movq(
        dst, Address(SPREG, scoped_reg.Reg(), scale, disp + target::kWordSize));
  }

 private:
  Assembler* assembler_ = nullptr;
};
template <typename T, int POOL_OFFSET>
class LikeRegisterScopedAccess {
 public:
  using LikeRegistersType = LikeRegisters<T, POOL_OFFSET>;
  LikeRegisterScopedAccess(LikeRegistersType& register_on_stack,
                           T value,
                           Register reg)
      : register_on_stack_(register_on_stack), reg_(reg), value_(value) {
    register_on_stack_.assembler()->pushq(reg_);
    register_on_stack_.assembler()->movq(reg_,
                                         register_on_stack_.address(value_));
  }

  ~LikeRegisterScopedAccess() {
    register_on_stack_.assembler()->movq(register_on_stack_.address(value_),
                                         reg_);
    register_on_stack_.assembler()->popq(reg_);
  }

  Register Reg() const { return reg_; }

 private:
  LikeRegistersType& register_on_stack_;
  Register reg_;
  T value_;
};

template <typename T, int POOL_OFFSET>
class LikeRegisterScopedAccessRead {
 public:
  using LikeRegistersType = LikeRegisters<T, POOL_OFFSET>;
  LikeRegisterScopedAccessRead(LikeRegistersType& register_on_stack,
                               T value,
                               Register reg)
      : register_on_stack_(register_on_stack), reg_(reg), value_(value) {
    register_on_stack_.assembler()->pushq(reg_);
    register_on_stack_.assembler()->movq(reg_,
                                         register_on_stack_.address(value_));
  }

  ~LikeRegisterScopedAccessRead() {
    register_on_stack_.assembler()->popq(reg_);
  }

  Register Reg() const { return reg_; }

 private:
  LikeRegistersType& register_on_stack_;
  Register reg_;
  T value_;
};

using LikeABIRegisters = LikeRegisters<LikeABI, kReservedMemoryOffset>;

#if defined(TARGET_ARCH_X64) || defined(TARGET_ARCH_IA32)
#define REPLACE_TMP_REGS_WITH_MEMORY_1(REG)                                    \
  compiler::LikeABI REG = compiler::LikeABI::REG

#define REPLACE_TMP_REGS_WITH_MEMORY_2(REG1, REG2)                             \
  REPLACE_TMP_REGS_WITH_MEMORY_1(REG1);                                        \
  REPLACE_TMP_REGS_WITH_MEMORY_1(REG2);

#define REPLACE_TMP_REGS_WITH_MEMORY_3(REG1, REG2, REG3)                       \
  REPLACE_TMP_REGS_WITH_MEMORY_1(REG1);                                        \
  REPLACE_TMP_REGS_WITH_MEMORY_2(REG2, REG3)

#define REPLACE_TMP_REGS_WITH_MEMORY_4(REG1, REG2, REG3, REG4)                 \
  REPLACE_TMP_REGS_WITH_MEMORY_1(REG1);                                        \
  REPLACE_TMP_REGS_WITH_MEMORY_3(REG2, REG3, REG4)

#define REPLACE_TMP_REGS_WITH_MEMORY_5(REG1, REG2, REG3, REG4, REG5)           \
  REPLACE_TMP_REGS_WITH_MEMORY_1(REG1);                                        \
  REPLACE_TMP_REGS_WITH_MEMORY_4(REG2, REG3, REG4, REG5)
#endif  // defined(TARGET_ARCH_X64) || defined(TARGET_ARCH_IA32)

}  // namespace compiler
}  // namespace dart

#endif  // LIKE_REGISTERS_H_
