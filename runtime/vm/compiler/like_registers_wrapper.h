#ifndef LIKE_REGISTERS_WRAPPER_H_
#define LIKE_REGISTERS_WRAPPER_H_

#include <map>
#include <set>
#include <vector>

#include "vm/compiler/like_registers.h"
#include "vm/constants.h"

class AddressRegsTracker {
 public:
  AddressRegsTracker();
  AddressRegsTracker(const AddressRegsTracker& other);
  ~AddressRegsTracker();

  void MarkRegAsAddressPart(dart::Register reg);

  static dart::RegList GetRegsInUse();

 private:
  std::vector<dart::Register> regs_;
  static thread_local std::map<dart::Register, int> usage_count_;
  static thread_local dart::RegList reg_in_use_;
};

template <typename... U>
inline dart::RegList MakeRegList(U... regs) {
  return ((1 << regs) | ...);
}

dart::Register FindFreeReg(dart::RegList busy_regs,
                           dart::RegList available_regs);

class MemoryRegister {
 public:
  MemoryRegister(dart::compiler::Assembler* assembler,
                 dart::compiler::LikeABI value,
                 bool read_access,
                 bool write_access,
                 dart::RegList busy_regs,
                 dart::Register use_reg);

  MemoryRegister(const MemoryRegister& other) = delete;
  MemoryRegister(MemoryRegister&& other);

  ~MemoryRegister();

  operator dart::Register() const { return reg_; }

 private:
  dart::Register GetReg(dart::Register use_reg, dart::RegList busy_regs);

  dart::Register reg_ = dart::kNoRegister;
  dart::compiler::Assembler* assembler_ = nullptr;
  int value_ = 0;
  bool read_access_ = false;
  bool write_access_ = false;
  dart::compiler::LikeRegisters<int, dart::kReservedMemoryOffset>
      internal_regs_;

  static thread_local std::set<int> in_use_check_;
  static thread_local int global_byse_regs_;
  static thread_local int stack_offset_;
};

class MemoryRegisterProvider {
 public:
  MemoryRegisterProvider(dart::compiler::Assembler* assembler,
                         dart::RegList busy_regs) {
    assembler_ = assembler;
    busy_regs_ = busy_regs;
  }

  template <typename... U>
  MemoryRegisterProvider(dart::compiler::Assembler* assembler, U... regs)
      : MemoryRegisterProvider(assembler,
                               MakeRegList(std::forward<U>(regs)...)) {}

  MemoryRegisterProvider(dart::compiler::Assembler* assembler)
      : MemoryRegisterProvider(assembler, dart::RegList{}) {}

  MemoryRegister Reg(
      dart::compiler::LikeABI value,
      dart::Register use_reg = dart::Register::kNoRegister) const {
    return MemoryRegister(assembler_, value, true, true, busy_regs_, use_reg);
  }

  MemoryRegister RegTmp(
      dart::Register use_reg = dart::Register::kNoRegister) const {
    return MemoryRegister(assembler_, dart::compiler::LikeABI::TMP, false,
                          false, busy_regs_, use_reg);
  }

  MemoryRegister RegReadOnly(
      dart::compiler::LikeABI value,
      dart::Register use_reg = dart::Register::kNoRegister) const {
    return MemoryRegister(assembler_, value, true, false, busy_regs_, use_reg);
  }

  MemoryRegister RegWriteOnly(
      dart::compiler::LikeABI value,
      dart::Register use_reg = dart::Register::kNoRegister) const {
    return MemoryRegister(assembler_, value, false, true, busy_regs_, use_reg);
  }

 private:
  dart::compiler::Assembler* assembler_;
  dart::RegList busy_regs_ = {};
};

#endif  // LIKE_REGISTERS_WRAPPER_H_
