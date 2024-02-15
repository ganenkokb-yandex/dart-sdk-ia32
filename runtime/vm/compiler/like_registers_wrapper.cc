#include "vm/compiler/like_registers_wrapper.h"
#include "vm/compiler/assembler/assembler.h"

thread_local std::map<dart::Register, int> AddressRegsTracker::usage_count_;
thread_local dart::RegList AddressRegsTracker::reg_in_use_ = {};

AddressRegsTracker::AddressRegsTracker() = default;

AddressRegsTracker::AddressRegsTracker(const AddressRegsTracker& other) {
  regs_ = other.regs_;
  for (const auto& reg : regs_) {
    if (usage_count_[reg]++ == 0) {
      reg_in_use_ |= 1 << reg;
    }
  }
}

AddressRegsTracker::~AddressRegsTracker() {
  for (const auto& reg : regs_) {
    ASSERT(usage_count_[reg] > 0);
    if (--usage_count_[reg] == 0) {
      reg_in_use_ &= ~(1 << reg);
    }
  }
}

void AddressRegsTracker::MarkRegAsAddressPart(dart::Register reg) {
  if (usage_count_[reg]++ == 0) {
    reg_in_use_ |= 1 << reg;
  }
  regs_.push_back(reg);
}

dart::RegList AddressRegsTracker::GetRegsInUse() {
  return reg_in_use_;
}

dart::Register FindFreeReg(dart::RegList busy_regs,
                           dart::RegList available_regs) {
  RELEASE_ASSERT((available_regs & dart::kDartAvailableCpuRegs) ==
                 available_regs);
  available_regs &= dart::kDartAvailableCpuRegs & ~busy_regs;
  for (intptr_t i = 0; i < dart::kNumberOfCpuRegisters; ++i) {
    if (available_regs & (1 << i)) {
      return static_cast<dart::Register>(i);
    }
  }
  RELEASE_ASSERT(false);
  return dart::kNoRegister;
}

thread_local std::set<int> MemoryRegister::in_use_check_;
thread_local int MemoryRegister::global_byse_regs_ = 0;
thread_local int MemoryRegister::stack_offset_ = 0;

MemoryRegister::MemoryRegister(dart::compiler::Assembler* assembler,
                               dart::compiler::LikeABI value,
                               bool read_access,
                               bool write_access,
                               dart::RegList busy_regs,
                               dart::Register use_reg)
    : reg_(GetReg(use_reg, busy_regs)),
      assembler_(assembler),
      value_(static_cast<int>(value)),
      read_access_(read_access),
      write_access_(write_access),
      internal_regs_(assembler) {
  ++stack_offset_;
  RELEASE_ASSERT(dart::kReservedMemoryOffset >=
                 stack_offset_ * dart::compiler::target::kWordSize);
  auto address = dart::compiler::Address(
      dart::THR, dart::compiler::target::Thread::memory_pool_offset() +
                     dart::kReservedMemoryOffset -
                     stack_offset_ * dart::compiler::target::kWordSize);
  assembler_->movq(address, reg_);
  if (read_access_) {
    assembler_->movq(reg_, internal_regs_.address(value_));
  }
  RELEASE_ASSERT((global_byse_regs_ & (1 << reg_)) == 0);
  global_byse_regs_ |= (1 << reg_);
  if (write_access_) {
    RELEASE_ASSERT(!in_use_check_.count(value_));
    in_use_check_.insert(value_);
  }
}

MemoryRegister::MemoryRegister(MemoryRegister&& other)
    : reg_(std::move(other.reg_)),
      assembler_(std::move(other.assembler_)),
      value_(std::move(other.value_)),
      read_access_(std::move(other.read_access_)),
      write_access_(std::move(other.write_access_)),
      internal_regs_(std::move(other.internal_regs_)) {
  other.reg_ == dart::Register::kNoRegister;
  other.assembler_ = nullptr;
}

MemoryRegister::~MemoryRegister() {
  if (!assembler_) {
    return;
  }
  if (write_access_) {
    assembler_->movq(internal_regs_.address(value_), reg_);
  }
  RELEASE_ASSERT(dart::kReservedMemoryOffset >=
                 stack_offset_ * dart::compiler::target::kWordSize);
  auto address = dart::compiler::Address(
      dart::THR, dart::compiler::target::Thread::memory_pool_offset() +
                     dart::kReservedMemoryOffset -
                     stack_offset_ * dart::compiler::target::kWordSize);
  --stack_offset_;
  assembler_->movq(reg_, address);
  RELEASE_ASSERT((global_byse_regs_ & (1 << reg_)) != 0);
  global_byse_regs_ &= ~(1 << reg_);
  if (write_access_) {
    in_use_check_.erase(value_);
  }
}

dart::Register MemoryRegister::GetReg(dart::Register use_reg,
                                      dart::RegList busy_regs) {
  if (use_reg == dart::Register::kNoRegister) {
    return FindFreeReg(
        busy_regs | global_byse_regs_ | AddressRegsTracker::GetRegsInUse(),
        dart::kDartAvailableCpuRegs);
  }
  return use_reg;
}
