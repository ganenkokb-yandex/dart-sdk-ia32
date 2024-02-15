// Copyright (c) 2013, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef RUNTIME_VM_COMPILER_ASSEMBLER_ASSEMBLER_X64_H_
#define RUNTIME_VM_COMPILER_ASSEMBLER_ASSEMBLER_X64_H_

#if defined(DART_PRECOMPILED_RUNTIME)
#error "AOT runtime should not use compiler sources (including header files)"
#endif  // defined(DART_PRECOMPILED_RUNTIME)

#ifndef RUNTIME_VM_COMPILER_ASSEMBLER_ASSEMBLER_H_
#error Do not include assembler_x64.h directly; use assembler.h instead.
#endif

#include <functional>

#include "platform/assert.h"
#include "platform/utils.h"
#include "vm/compiler/assembler/assembler_base.h"
#include "vm/compiler/like_registers.h"
#include "vm/compiler/like_registers_wrapper.h"
#include "vm/constants.h"
#include "vm/constants_x86.h"
#include "vm/hash_map.h"
#include "vm/pointer_tagging.h"

namespace dart {

// Forward declarations.
class FlowGraphCompiler;
class RegisterSet;

namespace compiler {

class Immediate : public ValueObject {
 public:
  explicit Immediate(int64_t value) : value_(value) {}

  Immediate(const Immediate& other) : ValueObject(), value_(other.value_) {}

  int64_t value() const { return value_; }

  bool is_int8() const { return Utils::IsInt(8, value_); }
  bool is_uint8() const { return Utils::IsUint(8, value_); }
  bool is_int16() const { return Utils::IsInt(16, value_); }
  bool is_uint16() const { return Utils::IsUint(16, value_); }
  bool is_int32() const { return Utils::IsInt(32, value_); }
  bool is_uint32() const { return Utils::IsUint(32, value_); }

 private:
  const int64_t value_;

  // TODO(5411081): Add DISALLOW_COPY_AND_ASSIGN(Immediate) once the mac
  // build issue is resolved.
  // And remove the unnecessary copy constructor.
};

class Operand : public ValueObject {
 public:
  uint8_t rex() const { return rex_; }

  uint8_t mod() const { return (encoding_at(0) >> 6) & 3; }

  Register rm() const {
    int rm_rex = (rex_ & REX_B) << 3;
    return static_cast<Register>(rm_rex + (encoding_at(0) & 7));
  }

  ScaleFactor scale() const {
    return static_cast<ScaleFactor>((encoding_at(1) >> 6) & 3);
  }

  Register index() const {
    int index_rex = (rex_ & REX_X) << 2;
    return static_cast<Register>(index_rex + ((encoding_at(1) >> 3) & 7));
  }

  Register base() const {
    int base_rex = (rex_ & REX_B) << 3;
    return static_cast<Register>(base_rex + (encoding_at(1) & 7));
  }

  int8_t disp8() const {
    ASSERT(length_ >= 2);
    return static_cast<int8_t>(encoding_[length_ - 1]);
  }

  int32_t disp32() const {
    ASSERT(length_ >= 5);
    return bit_copy<int32_t>(encoding_[length_ - 4]);
  }

  Operand(const Operand& other)
      : ValueObject(), length_(other.length_), rex_(other.rex_) {
    memmove(&encoding_[0], &other.encoding_[0], other.length_);
    like_abi_base_ = other.like_abi_base_;
    like_abi_index_ = other.like_abi_index_;
  }

  Operand& operator=(const Operand& other) {
    length_ = other.length_;
    rex_ = other.rex_;
    memmove(&encoding_[0], &other.encoding_[0], other.length_);
    like_abi_base_ = other.like_abi_base_;
    like_abi_index_ = other.like_abi_index_;
    return *this;
  }

  bool Equals(const Operand& other) const {
    if (length_ != other.length_) return false;
    if (rex_ != other.rex_) return false;
    for (uint8_t i = 0; i < length_; i++) {
      if (encoding_[i] != other.encoding_[i]) return false;
    }
    return true;
  }

 protected:
  dart::compiler::LikeABI like_abi_base_ = dart::compiler::LikeABI::kNoRegister;
  dart::compiler::LikeABI like_abi_index_ =
      dart::compiler::LikeABI::kNoRegister;

  Operand() : length_(0), rex_(REX_NONE) {}  // Needed by subclass Address.

  void SetModRM(int mod, Register rm) {
    ASSERT((mod & ~3) == 0);
    if ((rm > 7) && !((rm == R12) && (mod != 3))) {
      rex_ |= REX_B;
    }
    encoding_[0] = (mod << 6) | (rm & 7);
    length_ = 1;
  }

  void SetSIB(ScaleFactor scale, Register index, Register base) {
    ASSERT(length_ == 1);
    ASSERT((scale & ~3) == 0);
    if (base > 7) {
      ASSERT((rex_ & REX_B) == 0);  // Must not have REX.B already set.
      rex_ |= REX_B;
    }
    if (index > 7) rex_ |= REX_X;
    encoding_[1] = (scale << 6) | ((index & 7) << 3) | (base & 7);
    length_ = 2;
  }

  void SetDisp8(int8_t disp) {
    ASSERT(length_ == 1 || length_ == 2);
    encoding_[length_++] = static_cast<uint8_t>(disp);
  }

  void SetDisp32(int32_t disp) {
    ASSERT(length_ == 1 || length_ == 2);
    memmove(&encoding_[length_], &disp, sizeof(disp));
    length_ += sizeof(disp);
  }

 private:
  uint8_t length_;
  uint8_t rex_;
  uint8_t encoding_[6];

  explicit Operand(Register reg) : rex_(REX_NONE) { SetModRM(3, reg); }

  // Get the operand encoding byte at the given index.
  uint8_t encoding_at(intptr_t index) const {
    ASSERT(index >= 0 && index < length_);
    return encoding_[index];
  }

  // Returns whether or not this operand is really the given register in
  // disguise. Used from the assembler to generate better encodings.
  bool IsRegister(Register reg) const {
    return ((reg > 7 ? 1 : 0) == (rex_ & REX_B))  // REX.B match.
           && ((encoding_at(0) & 0xF8) == 0xC0)  // Addressing mode is register.
           && ((encoding_at(0) & 0x07) == reg);  // Register codes match.
  }

  friend class Assembler;
};

class Address : public Operand {
 public:
  Address(Register base, int32_t disp) {
    regs_tracker_.MarkRegAsAddressPart(base);
    if ((disp == 0) && ((base & 7) != RBP)) {
      SetModRM(0, base);
      if ((base & 7) == RSP) {
        SetSIB(TIMES_1, RSP, base);
      }
    } else if (Utils::IsInt(8, disp)) {
      SetModRM(1, base);
      if ((base & 7) == RSP) {
        SetSIB(TIMES_1, RSP, base);
      }
      SetDisp8(disp);
    } else {
      SetModRM(2, base);
      if ((base & 7) == RSP) {
        SetSIB(TIMES_1, RSP, base);
      }
      SetDisp32(disp);
    }
  }

  Address(dart::compiler::LikeABI base, int32_t disp)
      : Address(Register::kNoRegister, disp) {
    ASSERT(base != dart::compiler::LikeABI::kNoRegister);
    like_abi_base_ = base;
    make_address_ = [disp](Register new_base, Register new_index) {
      ASSERT(new_base != kNoRegister);
      ASSERT(new_index == kNoRegister);
      return Address(new_base, disp);
    };
  }

  // This addressing mode does not exist.
  Address(Register base, Register r);

  Address(Register index, ScaleFactor scale, int32_t disp) {
    regs_tracker_.MarkRegAsAddressPart(index);
    ASSERT(index != RSP);       // Illegal addressing mode.
    ASSERT(scale != TIMES_16);  // Unsupported scale factor.
    SetModRM(0, RSP);
    SetSIB(scale, index, RBP);
    SetDisp32(disp);
  }

  // This addressing mode does not exist.
  Address(Register index, ScaleFactor scale, Register r);

  Address(Register base, Register index, ScaleFactor scale, int32_t disp) {
    regs_tracker_.MarkRegAsAddressPart(base);
    regs_tracker_.MarkRegAsAddressPart(index);
    ASSERT(index != RSP);       // Illegal addressing mode.
    ASSERT(scale != TIMES_16);  // Unsupported scale factor.
    if ((disp == 0) && ((base & 7) != RBP)) {
      SetModRM(0, RSP);
      SetSIB(scale, index, base);
    } else if (Utils::IsInt(8, disp)) {
      SetModRM(1, RSP);
      SetSIB(scale, index, base);
      SetDisp8(disp);
    } else {
      SetModRM(2, RSP);
      SetSIB(scale, index, base);
      SetDisp32(disp);
    }
  }

  Address(dart::compiler::LikeABI base,
          Register index,
          ScaleFactor scale,
          int32_t disp)
      : Address(Register::kNoRegister, index, scale, disp) {
    ASSERT(base != dart::compiler::LikeABI::kNoRegister);
    like_abi_base_ = base;
    make_address_ = [index, scale, disp](Register new_base,
                                         Register new_index) {
      ASSERT(new_base != kNoRegister);
      ASSERT(new_index == kNoRegister);
      return Address(new_base, index, scale, disp);
    };
  }

  Address(Register base,
          dart::compiler::LikeABI index,
          ScaleFactor scale,
          int32_t disp)
      : Address(base, Register::kNoRegister, scale, disp) {
    ASSERT(index != dart::compiler::LikeABI::kNoRegister);
    like_abi_index_ = index;
    make_address_ = [base, scale, disp](Register new_base, Register new_index) {
      ASSERT(new_base == kNoRegister);
      ASSERT(new_index != kNoRegister);
      return Address(base, new_index, scale, disp);
    };
  }

  Address(dart::compiler::LikeABI base,
          dart::compiler::LikeABI index,
          ScaleFactor scale,
          int32_t disp)
      : Address(Register::kNoRegister, Register::kNoRegister, scale, disp) {
    ASSERT(base != dart::compiler::LikeABI::kNoRegister);
    ASSERT(index != dart::compiler::LikeABI::kNoRegister);
    like_abi_base_ = base;
    like_abi_index_ = index;
    make_address_ = [scale, disp](Register new_base, Register new_index) {
      ASSERT(new_base != kNoRegister);
      ASSERT(new_index != kNoRegister);
      return Address(new_base, new_index, scale, disp);
    };
  }

  // This addressing mode does not exist.
  Address(Register base, Register index, ScaleFactor scale, Register r);

  Address(const Address& other)
      : Operand(other),
        regs_tracker_(other.regs_tracker_),
        make_address_(other.make_address_) {}

  Address& operator=(const Address& other) {
    Operand::operator=(other);
    regs_tracker_ = other.regs_tracker_;
    make_address_ = other.make_address_;
    return *this;
  }

  Address MakeAddress(Register new_base, Register new_index) const {
    return make_address_(new_base, new_index);
  }

  static Address AddressRIPRelative(int32_t disp) {
    return Address(RIPRelativeDisp(disp));
  }
  static Address AddressBaseImm32(Register base, int32_t disp) {
    return Address(base, disp, true);
  }

  // This addressing mode does not exist.
  static Address AddressBaseImm32(Register base, Register r);

 private:
  Address(Register base, int32_t disp, bool fixed) {
    regs_tracker_.MarkRegAsAddressPart(base);
    ASSERT(fixed);
    SetModRM(2, base);
    if ((base & 7) == RSP) {
      SetSIB(TIMES_1, RSP, base);
    }
    SetDisp32(disp);
  }

  struct RIPRelativeDisp {
    explicit RIPRelativeDisp(int32_t disp) : disp_(disp) {}
    const int32_t disp_;
  };

  explicit Address(const RIPRelativeDisp& disp) {
    SetModRM(0, static_cast<Register>(0x5));
    SetDisp32(disp.disp_);
  }

  AddressRegsTracker regs_tracker_;
  std::function<Address(Register new_base, Register new_index)> make_address_;
};

class FieldAddress : public Address {
 public:
  FieldAddress(Register base, int32_t disp)
      : Address(base, disp - kHeapObjectTag) {}

  FieldAddress(dart::compiler::LikeABI base, int32_t disp)
      : Address(base, disp - kHeapObjectTag) {}

  // This addressing mode does not exist.
  FieldAddress(Register base, Register r);

  FieldAddress(Register base, Register index, ScaleFactor scale, int32_t disp)
      : Address(base, index, scale, disp - kHeapObjectTag) {}

  FieldAddress(Register base,
               dart::compiler::LikeABI index,
               ScaleFactor scale,
               int32_t disp)
      : Address(base, index, scale, disp - kHeapObjectTag) {}

  // This addressing mode does not exist.
  FieldAddress(Register base, Register index, ScaleFactor scale, Register r);

  FieldAddress(const FieldAddress& other) : Address(other) {}

  FieldAddress& operator=(const FieldAddress& other) {
    Address::operator=(other);
    return *this;
  }
};

#if !defined(DART_COMPRESSED_POINTERS)
#define OBJ(op) op##q
#else
#define OBJ(op) op##l
#endif

class Assembler : public AssemblerBase {
 public:
  explicit Assembler(ObjectPoolBuilder* object_pool_builder,
                     intptr_t far_branch_level = 0);

  ~Assembler() {}

  /*
   * Emit Machine Instructions.
   */
  void call(Register reg) { EmitUnaryL(reg, 0xFF, 2); }
  void call(const Address& address_orig) {
    auto address = UpdateAddress(address_orig, RSI);
    EmitUnaryL(address, 0xFF, 2);
  }
  void call(Label* label);
  void call(const ExternalLabel* label);

  void pushq(Register reg);
  void pushq(const Address& address) { EmitUnaryL(address, 0xFF, 6); }
  void pushq(const Immediate& imm);
  void PushImmediate(const Immediate& imm) { pushq(imm); }
  void PushImmediate(int64_t value) { PushImmediate(Immediate(value)); }

  void popq(Register reg);
  void popq(const Address& address) { EmitUnaryL(address, 0x8F, 0); }

  void setcc(Condition condition, ByteRegister dst);

  void EnterFullSafepoint();
  void ExitFullSafepoint(bool ignore_unwind_in_progress);
  void TransitionGeneratedToNative(Register destination_address,
                                   Register new_exit_frame,
                                   Register new_exit_through_ffi,
                                   bool enter_safepoint);
  void TransitionNativeToGenerated(bool leave_safepoint,
                                   bool ignore_unwind_in_progress = false);

// Register-register, register-address and address-register instructions.
#define RR(width, name, ...)                                                   \
  void name(Register dst, Register src) { Emit##width(dst, src, __VA_ARGS__); }
#define RA(width, name, ...)                                                   \
  void name(Register dst, const Address& src) {                                \
    Emit##width(dst, src, __VA_ARGS__);                                        \
  }
#define RAB(name, ...)                                                         \
  void name(ByteRegister dst, const Address& src) {                            \
    EmitB(dst, src, __VA_ARGS__);                                              \
  }
#define AR(width, name, ...)                                                   \
  void name(const Address& dst, Register src) {                                \
    Emit##width(src, dst, __VA_ARGS__);                                        \
  }
#define ARB(name, ...)                                                         \
  void name(const Address& dst, ByteRegister src) {                            \
    EmitB(src, dst, __VA_ARGS__);                                              \
  }
#define REGULAR_INSTRUCTION(name, ...)                                         \
  RA(W, name##w, __VA_ARGS__)                                                  \
  RA(L, name##l, __VA_ARGS__)                                                  \
  RA(Q, name##q, __VA_ARGS__)                                                  \
  RR(W, name##w, __VA_ARGS__)                                                  \
  RR(L, name##l, __VA_ARGS__)                                                  \
  RR(Q, name##q, __VA_ARGS__)
  REGULAR_INSTRUCTION(test, 0x85)
  REGULAR_INSTRUCTION(xchg, 0x87)
  REGULAR_INSTRUCTION(imul, 0xAF, 0x0F)
  REGULAR_INSTRUCTION(bsf, 0xBC, 0x0F)
  REGULAR_INSTRUCTION(bsr, 0xBD, 0x0F)
  REGULAR_INSTRUCTION(popcnt, 0xB8, 0x0F, 0xF3)
  REGULAR_INSTRUCTION(lzcnt, 0xBD, 0x0F, 0xF3)
#undef REGULAR_INSTRUCTION
  RA(Q, movsxd, 0x63)
  RR(Q, movsxd, 0x63)
  ARB(movb, 0x88)
  AR(L, movl, 0x89)
  AR(Q, movq, 0x89)
  AR(W, movw, 0x89)
  RAB(movb, 0x8A)
  RA(L, movl, 0x8B)
  RA(Q, movq, 0x8B)
  RR(L, movl, 0x8B)
  RA(Q, leaq, 0x8D)
  RA(L, leal, 0x8D)
  AR(L, cmpxchgl, 0xB1, 0x0F)
  AR(Q, cmpxchgq, 0xB1, 0x0F)
  RA(L, cmpxchgl, 0xB1, 0x0F)
  RA(Q, cmpxchgq, 0xB1, 0x0F)
  RR(L, cmpxchgl, 0xB1, 0x0F)
  RR(Q, cmpxchgq, 0xB1, 0x0F)
  RA(Q, movzxb, 0xB6, 0x0F)
  RR(Q, movzxb, 0xB6, 0x0F)
  RA(Q, movzxw, 0xB7, 0x0F)
  RR(Q, movzxw, 0xB7, 0x0F)
  RA(Q, movsxb, 0xBE, 0x0F)
  RR(Q, movsxb, 0xBE, 0x0F)
  RA(Q, movsxw, 0xBF, 0x0F)
  RR(Q, movsxw, 0xBF, 0x0F)
#define DECLARE_CMOV(name, code)                                               \
  RR(Q, cmov##name##q, 0x40 + code, 0x0F)                                      \
  RR(L, cmov##name##l, 0x40 + code, 0x0F)                                      \
  RA(Q, cmov##name##q, 0x40 + code, 0x0F)                                      \
  RA(L, cmov##name##l, 0x40 + code, 0x0F)
  X86_CONDITIONAL_SUFFIXES(DECLARE_CMOV)
#undef DECLARE_CMOV
#undef AA
#undef RA
#undef AR

#define SIMPLE(name, ...)                                                      \
  void name() { EmitSimple(__VA_ARGS__); }
  SIMPLE(cpuid, 0x0F, 0xA2)
  SIMPLE(fcos, 0xD9, 0xFF)
  SIMPLE(fincstp, 0xD9, 0xF7)
  SIMPLE(fsin, 0xD9, 0xFE)
  SIMPLE(lock, 0xF0)
  SIMPLE(rep_movsb, 0xF3, 0xA4)
  SIMPLE(rep_movsw, 0xF3, 0x66, 0xA5)
  SIMPLE(rep_movsd, 0xF3, 0xA5)
  SIMPLE(rep_movsq, 0xF3, 0x48, 0xA5)
#undef SIMPLE
// XmmRegister operations with another register or an address.
#define XX(width, name, ...)                                                   \
  void name(XmmRegister dst, XmmRegister src) {                                \
    Emit##width(dst, src, __VA_ARGS__);                                        \
  }
#define XA(width, name, ...)                                                   \
  void name(XmmRegister dst, const Address& src) {                             \
    Emit##width(dst, src, __VA_ARGS__);                                        \
  }
#define AX(width, name, ...)                                                   \
  void name(const Address& dst, XmmRegister src) {                             \
    Emit##width(src, dst, __VA_ARGS__);                                        \
  }
  // We could add movupd here, but movups does the same and is shorter.
  XA(L, movups, 0x10, 0x0F);
  XA(L, movsd, 0x10, 0x0F, 0xF2)
  XA(L, movss, 0x10, 0x0F, 0xF3)
  AX(L, movups, 0x11, 0x0F);
  AX(L, movsd, 0x11, 0x0F, 0xF2)
  AX(L, movss, 0x11, 0x0F, 0xF3)
  XX(L, movhlps, 0x12, 0x0F)
  XX(L, unpcklps, 0x14, 0x0F)
  XX(L, unpcklpd, 0x14, 0x0F, 0x66)
  XX(L, unpckhps, 0x15, 0x0F)
  XX(L, unpckhpd, 0x15, 0x0F, 0x66)
  XX(L, movlhps, 0x16, 0x0F)
  XX(L, movaps, 0x28, 0x0F)
  XX(L, comisd, 0x2F, 0x0F, 0x66)
#define DECLARE_XMM(name, code)                                                \
  XX(L, name##ps, 0x50 + code, 0x0F)                                           \
  XA(L, name##ps, 0x50 + code, 0x0F)                                           \
  AX(L, name##ps, 0x50 + code, 0x0F)                                           \
  XX(L, name##pd, 0x50 + code, 0x0F, 0x66)                                     \
  XA(L, name##pd, 0x50 + code, 0x0F, 0x66)                                     \
  AX(L, name##pd, 0x50 + code, 0x0F, 0x66)                                     \
  XX(L, name##sd, 0x50 + code, 0x0F, 0xF2)                                     \
  XA(L, name##sd, 0x50 + code, 0x0F, 0xF2)                                     \
  AX(L, name##sd, 0x50 + code, 0x0F, 0xF2)                                     \
  XX(L, name##ss, 0x50 + code, 0x0F, 0xF3)                                     \
  XA(L, name##ss, 0x50 + code, 0x0F, 0xF3)                                     \
  AX(L, name##ss, 0x50 + code, 0x0F, 0xF3)
  XMM_ALU_CODES(DECLARE_XMM)
#undef DECLARE_XMM
  XX(L, cvtps2pd, 0x5A, 0x0F)
  XX(L, cvtpd2ps, 0x5A, 0x0F, 0x66)
  XX(L, cvtsd2ss, 0x5A, 0x0F, 0xF2)
  XX(L, cvtss2sd, 0x5A, 0x0F, 0xF3)
  XX(L, pxor, 0xEF, 0x0F, 0x66)
  XX(L, subpl, 0xFA, 0x0F, 0x66)
  XX(L, addpl, 0xFE, 0x0F, 0x66)
#undef XX
#undef AX
#undef XA

#define DECLARE_CMPPS(name, code)                                              \
  void cmpps##name(XmmRegister dst, XmmRegister src) {                         \
    EmitL(dst, src, 0xC2, 0x0F);                                               \
    AssemblerBuffer::EnsureCapacity ensured(&buffer_);                         \
    EmitUint8(code);                                                           \
  }
  XMM_CONDITIONAL_CODES(DECLARE_CMPPS)
#undef DECLARE_CMPPS

#define DECLARE_SIMPLE(name, opcode)                                           \
  void name() { EmitSimple(opcode); }
  X86_ZERO_OPERAND_1_BYTE_INSTRUCTIONS(DECLARE_SIMPLE)
#undef DECLARE_SIMPLE

  void movl(Register dst, const Immediate& imm);
  void movl(const Address& dst, const Immediate& imm);

  void movb(const Address& dst, const Immediate& imm);

  void movw(Register dst, const Address& src);
  void movw(const Address& dst, const Immediate& imm);

  void movq(Register dst, const Immediate& imm);
  void movq(const Address& dst, const Immediate& imm);

  // Destination and source are reversed for some reason.
  void movq(Register dst, XmmRegister src) {
    EmitQ(src, dst, 0x7E, 0x0F, 0x66);
  }
  void movl(Register dst, XmmRegister src) {
    EmitL(src, dst, 0x7E, 0x0F, 0x66);
  }
  void movss(XmmRegister dst, XmmRegister src) {
    EmitL(src, dst, 0x11, 0x0F, 0xF3);
  }
  void movsd(XmmRegister dst, XmmRegister src) {
    EmitL(src, dst, 0x11, 0x0F, 0xF2);
  }

  // Use the reversed operand order and the 0x89 bytecode instead of the
  // obvious 0x88 encoding for this some, because it is expected by gdb64 older
  // than 7.3.1-gg5 when disassembling a function's prologue (movq rbp, rsp)
  // for proper unwinding of Dart frames (use --generate_gdb_symbols and -O0).
  void movq(Register dst, Register src) { EmitQ(src, dst, 0x89); }

  void movq(XmmRegister dst, Register src) {
    EmitQ(dst, src, 0x6E, 0x0F, 0x66);
  }

  void movd(XmmRegister dst, Register src) {
    EmitL(dst, src, 0x6E, 0x0F, 0x66);
  }
  void cvtsi2sdq(XmmRegister dst, Register src) {
    EmitQ(dst, src, 0x2A, 0x0F, 0xF2);
  }
  void cvtsi2sdl(XmmRegister dst, Register src) {
    EmitL(dst, src, 0x2A, 0x0F, 0xF2);
  }
  void cvttsd2siq(Register dst, XmmRegister src) {
    EmitQ(dst, src, 0x2C, 0x0F, 0xF2);
  }
  void cvttsd2sil(Register dst, XmmRegister src) {
    EmitL(dst, src, 0x2C, 0x0F, 0xF2);
  }
  void movmskpd(Register dst, XmmRegister src) {
    EmitL(dst, src, 0x50, 0x0F, 0x66);
  }
  void movmskps(Register dst, XmmRegister src) { EmitL(dst, src, 0x50, 0x0F); }
  void pmovmskb(Register dst, XmmRegister src) {
    EmitL(dst, src, 0xD7, 0x0F, 0x66);
  }

  void btl(Register dst, Register src) { EmitL(src, dst, 0xA3, 0x0F); }
  void btq(Register dst, Register src) { EmitQ(src, dst, 0xA3, 0x0F); }

  void notps(XmmRegister dst, XmmRegister src);
  void negateps(XmmRegister dst, XmmRegister src);
  void absps(XmmRegister dst, XmmRegister src);
  void zerowps(XmmRegister dst, XmmRegister src);

  void set1ps(XmmRegister dst, Register tmp, const Immediate& imm);
  void shufps(XmmRegister dst, XmmRegister src, const Immediate& mask);

  void negatepd(XmmRegister dst, XmmRegister src);
  void abspd(XmmRegister dst, XmmRegister src);
  void shufpd(XmmRegister dst, XmmRegister src, const Immediate& mask);

  enum RoundingMode {
    kRoundToNearest = 0x0,
    kRoundDown = 0x1,
    kRoundUp = 0x2,
    kRoundToZero = 0x3
  };
  void roundsd(XmmRegister dst, XmmRegister src, RoundingMode mode);

  void CompareImmediate(Register reg,
                        const Immediate& imm,
                        OperandSize width = kEightBytes);
  void CompareImmediate(const Address& address,
                        const Immediate& imm,
                        OperandSize width = kEightBytes);
  void CompareImmediate(Register reg,
                        int64_t immediate,
                        OperandSize width = kEightBytes) override {
    return CompareImmediate(reg, Immediate(immediate), width);
  }

  void testl(Register reg, const Immediate& imm) {
    testq(reg, Immediate(imm.value() & 0xFFFFFFFF));
  }
  void testb(const Address& address, const Immediate& imm);
  void testb(const Address& address, Register reg);

  void testq(Register reg, const Immediate& imm);
  void TestImmediate(Register dst,
                     const Immediate& imm,
                     OperandSize width = kEightBytes);

  void AndImmediate(Register dst, const Immediate& imm);
  void AndImmediate(Register dst, int64_t value) override {
    AndImmediate(dst, Immediate(value));
  }
  void AndImmediate(Register dst, Register src, int64_t value) {
    MoveRegister(dst, src);
    AndImmediate(dst, value);
  }
  void AndRegisters(Register dst,
                    Register src1,
                    Register src2 = kNoRegister) override;
  void OrImmediate(Register dst, const Immediate& imm);
  void OrImmediate(Register dst, int64_t value) {
    OrImmediate(dst, Immediate(value));
  }
  void XorImmediate(Register dst, const Immediate& imm);
  void LslImmediate(Register dst, int32_t shift) {
    shlq(dst, Immediate(shift));
  }
  void LslRegister(Register dst, Register shift) override;
  void LsrImmediate(Register dst, int32_t shift) override {
    shrq(dst, Immediate(shift));
  }

  void shldq(Register dst, Register src, Register shifter) {
    ASSERT(shifter == RCX);
    EmitQ(src, dst, 0xA5, 0x0F);
  }
  void shrdq(Register dst, Register src, Register shifter) {
    ASSERT(shifter == RCX);
    EmitQ(src, dst, 0xAD, 0x0F);
  }

#define DECLARE_ALU(op, c)                                                     \
  void op##w(Register dst, Register src) { EmitW(dst, src, c * 8 + 3); }       \
  void op##l(Register dst, Register src) { EmitL(dst, src, c * 8 + 3); }       \
  void op##q(Register dst, Register src) { EmitQ(dst, src, c * 8 + 3); }       \
  void op##w(Register dst, const Address& src) { EmitW(dst, src, c * 8 + 3); } \
  void op##l(Register dst, const Address& src) { EmitL(dst, src, c * 8 + 3); } \
  void op##q(Register dst, const Address& src) { EmitQ(dst, src, c * 8 + 3); } \
  void op##w(const Address& dst, Register src) { EmitW(src, dst, c * 8 + 1); } \
  void op##l(const Address& dst, Register src) { EmitL(src, dst, c * 8 + 1); } \
  void op##q(const Address& dst, Register src) { EmitQ(src, dst, c * 8 + 1); } \
  void op##l(Register dst, const Immediate& imm) { AluL(c, dst, imm); }        \
  void op##q(Register dst, const Immediate& imm) {                             \
    AluQ(c, c * 8 + 3, dst, imm);                                              \
  }                                                                            \
  void op##b(const Address& dst, const Immediate& imm) { AluB(c, dst, imm); }  \
  void op##w(const Address& dst, const Immediate& imm) { AluW(c, dst, imm); }  \
  void op##l(const Address& dst, const Immediate& imm) { AluL(c, dst, imm); }  \
  void op##q(const Address& dst, const Immediate& imm) {                       \
    AluQ(c, c * 8 + 3, dst, imm);                                              \
  }

  X86_ALU_CODES(DECLARE_ALU)

#undef DECLARE_ALU
#undef ALU_OPS

  void cqo();

#define REGULAR_UNARY(name, opcode, modrm)                                     \
  void name##q(Register reg) { EmitUnaryQ(reg, opcode, modrm); }               \
  void name##l(Register reg) { EmitUnaryL(reg, opcode, modrm); }               \
  void name##q(const Address& address) { EmitUnaryQ(address, opcode, modrm); } \
  void name##l(const Address& address) { EmitUnaryL(address, opcode, modrm); }
  REGULAR_UNARY(not, 0xF7, 2)
  REGULAR_UNARY(neg, 0xF7, 3)
  REGULAR_UNARY(mul, 0xF7, 4)
  REGULAR_UNARY(imul, 0xF7, 5)
  REGULAR_UNARY(div, 0xF7, 6)
  REGULAR_UNARY(idiv, 0xF7, 7)
  REGULAR_UNARY(inc, 0xFF, 0)
  REGULAR_UNARY(dec, 0xFF, 1)
#undef REGULAR_UNARY

  void imull(Register reg, const Immediate& imm);

  void imulq(Register dst, const Immediate& imm);
  void MulImmediate(Register reg,
                    const Immediate& imm,
                    OperandSize width = kEightBytes);
  void MulImmediate(Register reg,
                    int64_t imm,
                    OperandSize width = kEightBytes) override {
    MulImmediate(reg, Immediate(imm), width);
  }

  void shll(Register reg, const Immediate& imm);
  void shll(Register operand, Register shifter);
  void shrl(Register reg, const Immediate& imm);
  void shrl(Register operand, Register shifter);
  void sarl(Register reg, const Immediate& imm);
  void sarl(Register operand, Register shifter);
  void shldl(Register dst, Register src, const Immediate& imm);

  void shlq(Register reg, const Immediate& imm);
  void shlq(Register operand, Register shifter);
  void shrq(Register reg, const Immediate& imm);
  void shrq(Register operand, Register shifter);
  void sarq(Register reg, const Immediate& imm);
  void sarq(Register operand, Register shifter);
  void shldq(Register dst, Register src, const Immediate& imm);

  void btq(Register base, int bit);

  void enter(const Immediate& imm);

  void fldl(const Address& src);
  void fstpl(const Address& dst);

  void ffree(intptr_t value);

  // 'size' indicates size in bytes and must be in the range 1..8.
  void nop(int size = 1);

  void j(Condition condition, Label* label, JumpDistance distance = kFarJump);
  void jmp(Register reg) { EmitUnaryL(reg, 0xFF, 4); }
  void jmp(const Address& address_orig) {
    auto address = UpdateAddress(address_orig, RSI);
    EmitUnaryL(address, 0xFF, 4);
  }
  void jmp(Label* label, JumpDistance distance = kFarJump);
  void jmp(const ExternalLabel* label);
  void jmp(const Code& code);

  // Issue memory to memory move through a TMP register.
  // TODO(koda): Assert that these are not used for heap objects.
  void MoveMemoryToMemory(const Address& dst, const Address& src) {
    movq(TMP, src);
    movq(dst, TMP);
  }

  void Exchange(Register reg, const Address& mem) {
    movq(TMP, mem);
    movq(mem, reg);
    movq(reg, TMP);
  }

  void Exchange(const Address& mem1, const Address& mem2) {
    movq(TMP, mem1);
    xorq(TMP, mem2);
    xorq(mem1, TMP);
    xorq(mem2, TMP);
  }

  // Methods for High-level operations and implemented on all architectures.
  void Ret() { ret(); }

  // Sets the return address to [value] as if there was a call.
  // On X64 pushes [value].
  void SetReturnAddress(Register value) {
    PushRegister(value);
  }

  void CompareRegisters(Register a, Register b);
  void CompareObjectRegisters(Register a, Register b) { OBJ(cmp)(a, b); }
  void BranchIf(Condition condition,
                Label* label,
                JumpDistance distance = kFarJump) {
    j(condition, label, distance);
  }
  void BranchIfZero(Register src,
                    Label* label,
                    JumpDistance distance = kFarJump) {
    cmpq(src, Immediate(0));
    j(ZERO, label, distance);
  }
  void BranchIfBit(Register rn,
                   intptr_t bit_number,
                   Condition condition,
                   Label* label,
                   JumpDistance distance = kFarJump) {
    testq(rn, Immediate(1 << bit_number));
    j(condition, label, distance);
  }

  void ExtendValue(Register dst, Register src, OperandSize sz) override;
  void PushRegister(Register r);
  void PopRegister(Register r);

  void PushRegisterPair(Register r0, Register r1) {
    PushRegister(r1);
    PushRegister(r0);
  }
  void PopRegisterPair(Register r0, Register r1) {
    PopRegister(r0);
    PopRegister(r1);
  }

  void PushValueAtOffset(Register base, int32_t offset) {
    pushq(Address(base, offset));
  }

  // Methods for adding/subtracting an immediate value that may be loaded from
  // the constant pool.
  // TODO(koda): Assert that these are not used for heap objects.
  void AddImmediate(Register reg,
                    const Immediate& imm,
                    OperandSize width = kEightBytes);
  void AddImmediate(Register reg,
                    int64_t value,
                    OperandSize width = kEightBytes) {
    AddImmediate(reg, Immediate(value), width);
  }
  void AddRegisters(Register dest, Register src) {
    addq(dest, src);
  }
  // [dest] = [src] << [scale] + [value].
  void AddScaled(Register dest,
                 Register src,
                 ScaleFactor scale,
                 int32_t value) {
    leaq(dest, Address(src, scale, value));
  }
  void AddImmediate(Register dest, Register src, int64_t value);
  void AddImmediate(const Address& address, const Immediate& imm);
  void SubImmediate(Register reg,
                    const Immediate& imm,
                    OperandSize width = kEightBytes);
  void SubImmediate(const Address& address, const Immediate& imm);
  void SubRegisters(Register dest, Register src) {
    subq(dest, src);
  }

  void Drop(intptr_t stack_elements, Register tmp = TMP);

  bool constant_pool_allowed() const { return constant_pool_allowed_; }
  void set_constant_pool_allowed(bool b) { constant_pool_allowed_ = b; }

  // Unlike movq this can affect the flags or use the constant pool.
  void LoadImmediate(Register reg, const Immediate& imm);
  void LoadImmediate(Register reg, int64_t immediate) {
    LoadImmediate(reg, Immediate(immediate));
  }
  void LoadSImmediate(FpuRegister dst, float immediate);
  void LoadDImmediate(FpuRegister dst, double immediate);
  void LoadQImmediate(FpuRegister dst, simd128_value_t immediate);

  void LoadIsolate(Register dst);
  void LoadIsolateGroup(Register dst);
  void LoadDispatchTable(Register dst);
  void LoadObject(Register dst, const Object& obj);
  void LoadUniqueObject(Register dst, const Object& obj);
  void LoadNativeEntry(Register dst,
                       const ExternalLabel* label,
                       ObjectPoolBuilderEntry::Patchability patchable);
  void JmpPatchable(const Code& code, dart::compiler::LikeABI pp);
  void Jmp(const Code& code, dart::compiler::LikeABI pp = PP);
  void J(Condition condition, const Code& code, dart::compiler::LikeABI pp);
  void CallPatchable(const Code& code,
                     CodeEntryKind entry_kind = CodeEntryKind::kNormal);
  void Call(const Code& stub_entry);

  // Emit a call that shares its object pool entries with other calls
  // that have the same equivalence marker.
  void CallWithEquivalence(const Code& code,
                           const Object& equivalence,
                           CodeEntryKind entry_kind = CodeEntryKind::kNormal);

  void Call(Address target) { call(target); }

  // Unaware of write barrier (use StoreInto* methods for storing to objects).
  // TODO(koda): Add StackAddress/HeapAddress types to prevent misuse.
  void StoreObject(const Address& dst, const Object& obj);
  void PushObject(const Object& object);
  void CompareObject(Register reg, const Object& object);

  void LoadCompressed(Register dest, const Address& slot);
  void LoadCompressedSmi(Register dest, const Address& slot) override;

  // Store into a heap object and apply the generational and incremental write
  // barriers. All stores into heap objects must pass through this function or,
  // if the value can be proven either Smi or old-and-premarked, its NoBarrier
  // variants.
  // Preserves object and value registers.
  void StoreIntoObject(Register object,      // Object we are storing into.
                       const Address& dest,  // Where we are storing into.
                       Register value,       // Value we are storing.
                       CanBeSmi can_be_smi = kValueCanBeSmi,
                       MemoryOrder memory_order = kRelaxedNonAtomic) override;
  void StoreIntoObjectOffset(Register object,  // Object we are storing into.
                             int32_t offset,   // Where we are storing into.
                             Register value,   // Value we are storing.
                             CanBeSmi can_be_smi = kValueCanBeSmi,
                             MemoryOrder memory_order = kRelaxedNonAtomic) {
    StoreIntoObject(object, FieldAddress(object, offset), value, can_be_smi,
                    memory_order);
  }
  void StoreCompressedIntoObject(
      Register object,      // Object we are storing into.
      const Address& dest,  // Where we are storing into.
      Register value,       // Value we are storing.
      CanBeSmi can_be_smi = kValueCanBeSmi,
      MemoryOrder memory_order = kRelaxedNonAtomic) override;
  void StoreCompressedIntoObjectOffset(
      Register object,  // Object we are storing into.
      int32_t offset,   // Where we are storing into.
      Register value,   // Value we are storing.
      CanBeSmi can_be_smi = kValueCanBeSmi,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    StoreCompressedIntoObject(object, FieldAddress(object, offset), value,
                              can_be_smi, memory_order);
  }
  void StoreBarrier(Register object,  // Object we are storing into.
                    Register value,   // Value we are storing.
                    CanBeSmi can_be_smi);
  void StoreIntoArray(Register object,  // Object we are storing into.
                      Register slot,    // Where we are storing into.
                      Register value,   // Value we are storing.
                      CanBeSmi can_be_smi = kValueCanBeSmi);
  void StoreCompressedIntoArray(Register object,  // Object we are storing into.
                                Register slot,    // Where we are storing into.
                                Register value,   // Value we are storing.
                                CanBeSmi can_be_smi = kValueCanBeSmi);

  void StoreIntoObjectNoBarrier(
      Register object,
      const Address& dest,
      Register value,
      MemoryOrder memory_order = kRelaxedNonAtomic) override;
  void StoreCompressedIntoObjectNoBarrier(
      Register object,
      const Address& dest,
      Register value,
      MemoryOrder memory_order = kRelaxedNonAtomic) override;
  void StoreIntoObjectNoBarrier(Register object,
                                const Address& dest,
                                const Object& value,
                                MemoryOrder memory_order = kRelaxedNonAtomic);
  void StoreCompressedIntoObjectNoBarrier(
      Register object,
      const Address& dest,
      const Object& value,
      MemoryOrder memory_order = kRelaxedNonAtomic);

  void StoreIntoObjectOffsetNoBarrier(
      Register object,
      int32_t offset,
      Register value,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    StoreIntoObjectNoBarrier(object, FieldAddress(object, offset), value,
                             memory_order);
  }
  void StoreCompressedIntoObjectOffsetNoBarrier(
      Register object,
      int32_t offset,
      Register value,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    StoreCompressedIntoObjectNoBarrier(object, FieldAddress(object, offset),
                                       value, memory_order);
  }
  void StoreIntoObjectOffsetNoBarrier(
      Register object,
      int32_t offset,
      const Object& value,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    StoreIntoObjectNoBarrier(object, FieldAddress(object, offset), value,
                             memory_order);
  }
  void StoreCompressedIntoObjectOffsetNoBarrier(
      Register object,
      int32_t offset,
      const Object& value,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    StoreCompressedIntoObjectNoBarrier(object, FieldAddress(object, offset),
                                       value, memory_order);
  }

  // Stores a non-tagged value into a heap object.
  void StoreInternalPointer(Register object,
                            const Address& dest,
                            Register value);

  // Stores a Smi value into a heap object field that always contains a Smi.
  void StoreIntoSmiField(const Address& dest, Register value);
  void ZeroInitSmiField(const Address& dest);
  void ZeroInitCompressedSmiField(const Address& dest);
  // Increments a Smi field. Leaves flags in same state as an 'addq'.
  void IncrementCompressedSmiField(const Address& dest, int64_t increment);

  void DoubleNegate(XmmRegister dst, XmmRegister src);
  void DoubleAbs(XmmRegister dst, XmmRegister src);

  void LockCmpxchgq(const Address& address_orig, Register reg) {
    ScopedAddressReg address_reg;
    auto address =
        UpdateAddress(address_orig, address_reg, (1 << reg) | (1 << RAX));
    lock();
    cmpxchgq(address, reg);
  }

  void LockCmpxchgl(const Address& address, Register reg) {
    lock();
    cmpxchgl(address, reg);
  }

  void PushRegisters(const RegisterSet& registers);
  void PopRegisters(const RegisterSet& registers);

  void PushRegistersInOrder(std::initializer_list<Register> regs);

  void CheckCodePointer();

  void EnterFrame(intptr_t frame_space);
  void LeaveFrame();
  void ReserveAlignedFrameSpace(intptr_t frame_space);

  // In debug mode, generates code to verify that:
  //   FP + kExitLinkSlotFromFp == SP
  //
  // Triggers breakpoint otherwise.
  // Clobbers RAX.
  void EmitEntryFrameVerification();

  // For non-leaf runtime calls. For leaf runtime calls, use LeafRuntimeScope,
  void CallRuntime(const RuntimeEntry& entry, intptr_t argument_count);

  // Call runtime function. Reserves shadow space on the stack before calling
  // if platform ABI requires that.
  void CallCFunction(Register reg, bool restore_rsp = false);
  void CallCFunction(Address address, bool restore_rsp = false);

  void ExtractClassIdFromTags(Register result, Register tags);
  void ExtractInstanceSizeFromTags(Register result, Register tags);

  void RangeCheck(Register value,
                  Register temp,
                  intptr_t low,
                  intptr_t high,
                  RangeCheckCondition condition,
                  Label* target) override;

  // Loading and comparing classes of objects.
  void LoadClassId(Register result, Register object);
  void LoadClassById(Register result, Register class_id);

  void CompareClassId(Register object,
                      intptr_t class_id,
                      Register scratch = kNoRegister);

  void LoadClassIdMayBeSmi(Register result, Register object);
  void LoadTaggedClassIdMayBeSmi(Register result, Register object);

  void EnsureHasClassIdInDEBUG(intptr_t cid,
                               Register src,
                               Register scratch,
                               bool can_be_null = false) override;

#if defined(DART_COMPRESSED_POINTERS)
  void ExtendNonNegativeSmi(Register dst) override {
    // Zero-extends and is a smaller instruction to output than sign
    // extension (movsxd).
    orl(dst, dst);
  }
#endif

  // CheckClassIs fused with optimistic SmiUntag.
  // Value in the register object is untagged optimistically.
  void SmiUntagOrCheckClass(Register object, intptr_t class_id, Label* smi);

  // Misc. functionality.
  void SmiTag(Register reg) override { OBJ(add)(reg, reg); }

  void SmiUntag(Register reg) { OBJ(sar)(reg, Immediate(kSmiTagSize)); }
  void SmiUntag(Register dst, Register src) {
    if (dst != src) {
      OBJ(mov)(dst, src);
    }
    OBJ(sar)(dst, Immediate(kSmiTagSize));
  }

  void SmiUntagAndSignExtend(Register reg) {
#if !defined(DART_COMPRESSED_POINTERS)
    sarq(reg, Immediate(kSmiTagSize));
#else
    // This is shorter than
    // shlq reg, 32
    // sraq reg, 33
    sarl(reg, Immediate(kSmiTagSize));
    movsxd(reg, reg);
#endif
  }

  void SmiUntagAndSignExtend(Register dst, Register src) {
#if !defined(DART_COMPRESSED_POINTERS)
    if (dst != src) {
      movq(dst, src);
    }
    sarq(dst, Immediate(kSmiTagSize));
#else
    movsxd(dst, src);
    sarq(dst, Immediate(kSmiTagSize));
#endif
  }

  void LoadWordFromBoxOrSmi(Register result, Register value) {
    LoadInt64FromBoxOrSmi(result, value);
  }

  void LoadInt64FromBoxOrSmi(Register result, Register value);

  void BranchIfNotSmi(Register reg,
                      Label* label,
                      JumpDistance distance = kFarJump) {
    testq(reg, Immediate(kSmiTagMask));
    j(NOT_ZERO, label, distance);
  }

  void BranchIfSmi(Register reg,
                   Label* label,
                   JumpDistance distance = kFarJump) override {
    testq(reg, Immediate(kSmiTagMask));
    j(ZERO, label, distance);
  }

  void ArithmeticShiftRightImmediate(Register reg, intptr_t shift) override;
  void CompareWords(Register reg1,
                    Register reg2,
                    intptr_t offset,
                    Register count,
                    Register temp,
                    Label* equals) override;

  void Align(int alignment, intptr_t offset);
  void Bind(Label* label);
  // Unconditional jump to a given label.
  void Jump(Label* label, JumpDistance distance = kFarJump) {
    jmp(label, distance);
  }
  // Unconditional jump to a given address in register.
  void Jump(Register target) {
    jmp(target);
  }
  // Unconditional jump to a given address in memory.
  void Jump(const Address& address) { jmp(address); }

  // Arch-specific LoadFromOffset to choose the right operation for [sz].
  void LoadFromOffset(Register dst,
                      const Address& address,
                      OperandSize sz = kEightBytes) override;
  void LoadFromOffset(Register dst,
                      Register base,
                      int32_t offset,
                      OperandSize sz = kEightBytes) {
    LoadFromOffset(dst, Address(base, offset), sz);
  }
  void LoadField(Register dst, const FieldAddress& address) override {
    LoadField(dst, address, kEightBytes);
  }
  void LoadField(Register dst, const FieldAddress& address, OperandSize sz) {
    LoadFromOffset(dst, address, sz);
  }
  void LoadCompressedField(Register dst, const FieldAddress& address) override {
    LoadCompressed(dst, address);
  }
  void LoadFieldFromOffset(Register dst,
                           Register base,
                           int32_t offset,
                           OperandSize sz = kEightBytes) override {
    LoadFromOffset(dst, FieldAddress(base, offset), sz);
  }
  void LoadCompressedFieldFromOffset(Register dst,
                                     Register base,
                                     int32_t offset) override {
    LoadCompressed(dst, FieldAddress(base, offset));
  }
  void LoadIndexedPayload(Register dst,
                          Register base,
                          int32_t payload_offset,
                          Register index,
                          ScaleFactor scale,
                          OperandSize sz = kEightBytes) {
    LoadFromOffset(dst, FieldAddress(base, index, scale, payload_offset), sz);
  }
  void LoadIndexedCompressed(Register dst,
                             Register base,
                             int32_t offset,
                             Register index) {
    LoadCompressed(
        dst, FieldAddress(base, index, TIMES_COMPRESSED_WORD_SIZE, offset));
  }
  void StoreToOffset(Register src,
                     const Address& address,
                     OperandSize sz = kEightBytes) override;
  void StoreToOffset(Register src,
                     Register base,
                     int32_t offset,
                     OperandSize sz = kEightBytes) {
    StoreToOffset(src, Address(base, offset), sz);
  }
  void StoreFieldToOffset(Register src,
                          Register base,
                          int32_t offset,
                          OperandSize sz = kEightBytes) {
    StoreToOffset(src, FieldAddress(base, offset), sz);
  }
  void StoreZero(const Address& address, Register temp = kNoRegister) {
    movq(address, Immediate(0));
  }
  void LoadFromStack(Register dst, intptr_t depth);
  void StoreToStack(Register src, intptr_t depth);
  void CompareToStack(Register src, intptr_t depth);
  void LoadMemoryValue(Register dst, Register base, int32_t offset) {
    movq(dst, Address(base, offset));
  }
  void LoadCompressedMemoryValue(Register dst, Register base, int32_t offset) {
    OBJ(mov)(dst, Address(base, offset));
  }
  void StoreMemoryValue(Register src, Register base, int32_t offset) {
    movq(Address(base, offset), src);
  }

  void LoadUnboxedSimd128(FpuRegister dst, Register base, int32_t offset) {
    movups(dst, Address(base, offset));
  }
  void StoreUnboxedSimd128(FpuRegister dst, Register base, int32_t offset) {
    movups(Address(base, offset), dst);
  }
  void MoveUnboxedSimd128(FpuRegister dst, FpuRegister src) {
    if (src != dst) {
      movaps(dst, src);
    }
  }

  void LoadUnboxedSingle(FpuRegister dst, Register base, int32_t offset) {
    movss(dst, Address(base, offset));
  }
  void LoadUnboxedDouble(FpuRegister dst, Register base, int32_t offset) {
    movsd(dst, Address(base, offset));
  }
  void StoreUnboxedDouble(FpuRegister src, Register base, int32_t offset) {
    movsd(Address(base, offset), src);
  }
  void MoveUnboxedDouble(FpuRegister dst, FpuRegister src) {
    if (src != dst) {
      movaps(dst, src);
    }
  }

  void MsanUnpoison(Register base, intptr_t length_in_bytes);
  void MsanUnpoison(Register base, Register length_in_bytes);

#if defined(TARGET_USES_THREAD_SANITIZER)
  void TsanLoadAcquire(Address addr);
  void TsanStoreRelease(Address addr);
#endif

  void LoadAcquire(Register dst,
                   Register address,
                   int32_t offset = 0,
                   OperandSize size = kEightBytes) override {
    // On intel loads have load-acquire behavior (i.e. loads are not re-ordered
    // with other loads).
    LoadFromOffset(dst, Address(address, offset), size);
#if defined(TARGET_USES_THREAD_SANITIZER)
    TsanLoadAcquire(Address(address, offset));
#endif
  }
  void LoadAcquireCompressed(Register dst,
                             Register address,
                             int32_t offset = 0) override {
    // On intel loads have load-acquire behavior (i.e. loads are not re-ordered
    // with other loads).
    LoadCompressed(dst, Address(address, offset));
#if defined(TARGET_USES_THREAD_SANITIZER)
    TsanLoadAcquire(Address(address, offset));
#endif
  }
  void StoreRelease(Register src,
                    Register address,
                    int32_t offset = 0) override {
    // On intel stores have store-release behavior (i.e. stores are not
    // re-ordered with other stores).
    movq(Address(address, offset), src);
#if defined(TARGET_USES_THREAD_SANITIZER)
    TsanStoreRelease(Address(address, offset));
#endif
  }
  void StoreReleaseCompressed(Register src,
                              Register address,
                              int32_t offset = 0) {
    // On intel stores have store-release behavior (i.e. stores are not
    // re-ordered with other stores).
    OBJ(mov)(Address(address, offset), src);
#if defined(TARGET_USES_THREAD_SANITIZER)
    TsanStoreRelease(Address(address, offset));
#endif
  }

  void CompareWithMemoryValue(Register value,
                              Address address,
                              OperandSize size = kEightBytes) override {
    ASSERT(size == kEightBytes || size == kFourBytes);
    if (size == kFourBytes) {
      cmpl(value, address);
    } else {
      cmpq(value, address);
    }
  }
  void CompareWithCompressedFieldFromOffset(Register value,
                                            Register base,
                                            int32_t offset) {
    OBJ(cmp)(value, FieldAddress(base, offset));
  }

  void RestoreCodePointer();
  void LoadPoolPointer(
      dart::compiler::LikeABI pp = dart::compiler::LikeABI::PP);

  // Set up a Dart frame on entry with a frame pointer and PC information to
  // enable easy access to the RawInstruction object of code corresponding
  // to this frame.
  // The dart frame layout is as follows:
  //   ....
  //   locals space  <=== RSP
  //   saved PP
  //   code object (used to derive the RawInstruction Object of the dart code)
  //   saved RBP     <=== RBP
  //   ret PC
  //   .....
  // This code sets this up with the sequence:
  //   pushq rbp
  //   movq rbp, rsp
  //   call L
  //   L: <code to adjust saved pc if there is any intrinsification code>
  //   ...
  //   pushq r15
  //   .....
  void EnterDartFrame(
      intptr_t frame_size,
      dart::compiler::LikeABI new_pp = dart::compiler::LikeABI::kNoRegister);
  void LeaveDartFrame();

  // Set up a Dart frame for a function compiled for on-stack replacement.
  // The frame layout is a normal Dart frame, but the frame is partially set
  // up on entry (it is the frame of the unoptimized code).
  void EnterOsrFrame(intptr_t extra_size);

  // Set up a stub frame so that the stack traversal code can easily identify
  // a stub frame.
  // The stub frame layout is as follows:
  //   ....       <=== RSP
  //   pc (used to derive the RawInstruction Object of the stub)
  //   saved RBP  <=== RBP
  //   ret PC
  //   .....
  // This code sets this up with the sequence:
  //   pushq rbp
  //   movq rbp, rsp
  //   pushq immediate(0)
  //   .....
  void EnterStubFrame();
  void LeaveStubFrame();

  // Set up a frame for calling a C function.
  // Automatically save the pinned registers in Dart which are not callee-
  // saved in the native calling convention.
  // Use together with CallCFunction.
  void EnterCFrame(intptr_t frame_space);
  void LeaveCFrame();

  void MonomorphicCheckedEntryJIT();
  void MonomorphicCheckedEntryAOT();
  void BranchOnMonomorphicCheckedEntryJIT(Label* label);

  void CombineHashes(Register dst, Register other) override;
  void FinalizeHashForSize(intptr_t bit_size,
                           Register dst,
                           Register scratch = TMP) override;

  // If allocation tracing for |cid| is enabled, will jump to |trace| label,
  // which will allocate in the runtime where tracing occurs.
  void MaybeTraceAllocation(intptr_t cid,
                            Label* trace,
                            Register temp_reg = kNoRegister,
                            JumpDistance distance = JumpDistance::kFarJump);

  void TryAllocateObject(intptr_t cid,
                         intptr_t instance_size,
                         Label* failure,
                         JumpDistance distance,
                         Register instance_reg,
                         Register temp) override;

  void TryAllocateArray(intptr_t cid,
                        intptr_t instance_size,
                        Label* failure,
                        JumpDistance distance,
                        Register instance,
                        Register end_address,
                        Register temp);

  void CheckAllocationCanary(Register top) {
#if defined(DEBUG)
    Label okay;
    cmpl(Address(top, 0), Immediate(kAllocationCanary));
    j(EQUAL, &okay, Assembler::kNearJump);
    Stop("Allocation canary");
    Bind(&okay);
#endif
  }
  void WriteAllocationCanary(Register top) {
#if defined(DEBUG)
    movl(Address(top, 0), Immediate(kAllocationCanary));
#endif
  }

  // Copy [size] bytes from [src] address to [dst] address.
  // [size] should be a multiple of word size.
  // Clobbers [src], [dst], [size] and [temp] registers.
  // X64 requires fixed registers for memory copying:
  // [src] = RSI, [dst] = RDI, [size] = RCX.
  void CopyMemoryWords(Register src,
                       Register dst,
                       Register size,
                       Register temp = kNoRegister);

  // This emits an PC-relative call of the form "callq *[rip+<offset>]".  The
  // offset is not yet known and needs therefore relocation to the right place
  // before the code can be used.
  //
  // The necessary information for the "linker" (i.e. the relocation
  // information) is stored in [UntaggedCode::static_calls_target_table_]: an
  // entry of the form
  //
  //   (Code::kPcRelativeCall & pc_offset, <target-code>, <target-function>)
  //
  // will be used during relocation to fix the offset.
  //
  // The provided [offset_into_target] will be added to calculate the final
  // destination.  It can be used e.g. for calling into the middle of a
  // function.
  void GenerateUnRelocatedPcRelativeCall(intptr_t offset_into_target = 0);

  // This emits an PC-relative tail call of the form "jmp *[rip+<offset>]".
  //
  // See also above for the pc-relative call.
  void GenerateUnRelocatedPcRelativeTailCall(intptr_t offset_into_target = 0);

  // Debugging and bringup support.
  void Breakpoint() override { int3(); }

  static Address ElementAddressForIntIndex(bool is_external,
                                           intptr_t cid,
                                           intptr_t index_scale,
                                           Register array,
                                           intptr_t index);
  static Address ElementAddressForRegIndex(bool is_external,
                                           intptr_t cid,
                                           intptr_t index_scale,
                                           bool index_unboxed,
                                           Register array,
                                           Register index);

  void LoadStaticFieldAddress(Register address,
                              Register field,
                              Register scratch) {
    LoadCompressedSmi(
        scratch, compiler::FieldAddress(
                     field, target::Field::host_offset_or_field_id_offset()));
    const intptr_t field_table_offset =
        compiler::target::Thread::field_table_values_offset();
    LoadMemoryValue(address, THR, static_cast<int32_t>(field_table_offset));
    static_assert(kSmiTagShift == 1, "adjust scale factor");
    leaq(address, Address(address, scratch, TIMES_HALF_WORD_SIZE, 0));
  }

  void LoadFieldAddressForRegOffset(Register address,
                                    Register instance,
                                    Register offset_in_words_as_smi) {
    static_assert(kSmiTagShift == 1, "adjust scale factor");
    leaq(address, FieldAddress(instance, offset_in_words_as_smi, TIMES_4, 0));
  }

  void LoadCompressedFieldAddressForRegOffset(Register address,
                                              Register instance,
                                              Register offset_in_words_as_smi) {
    static_assert(kSmiTagShift == 1, "adjust scale factor");
    leaq(address, FieldAddress(instance, offset_in_words_as_smi,
                               TIMES_COMPRESSED_HALF_WORD_SIZE, 0));
  }

  void LoadFieldAddressForOffset(Register address,
                                 Register instance,
                                 int32_t offset) override {
    leaq(address, FieldAddress(instance, offset));
  }

  static Address VMTagAddress();

  // On some other platforms, we draw a distinction between safe and unsafe
  // smis.
  static bool IsSafe(const Object& object) { return true; }
  static bool IsSafeSmi(const Object& object) { return target::IsSmi(object); }

  using ScopedAddressReg = std::unique_ptr<MemoryRegister>;

  Address UpdateAddress(const Address& address, Register reg);
  Address UpdateAddress(const Address& address,
                        ScopedAddressReg& reg,
                        dart::RegList busy_regs = 0);

  void pushq(dart::compiler::LikeABI reg) {
    LikeABIRegisters abi_regs(this);
    abi_regs.Push(reg);
  }

  void popq(dart::compiler::LikeABI reg) {
    LikeABIRegisters abi_regs(this);
    abi_regs.Pop(reg);
  }

  void movl(dart::compiler::LikeABI dst, const Address& src) {
    MemoryRegisterProvider scoped(this);
    movl(scoped.Reg(dst), src);
  }

  void movl(const Address& dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    movl(dst, scoped.RegReadOnly(src));
  }

  void movq(dart::compiler::LikeABI dst, const Address& src) {
    MemoryRegisterProvider scoped(this);
    movq(scoped.Reg(dst), src);
  }

  void movq(dart::compiler::LikeABI dst, const Immediate& imm) {
    LikeABIRegisters abi_regs(this);
    movq(abi_regs.address(dst), imm);
  }

  void movq(const Address& dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    movq(dst, scoped.RegReadOnly(src));
  }

  void movq(dart::compiler::LikeABI dst, Register src) {
    LikeABIRegisters abi_regs(this);
    movq(abi_regs.address(dst), src);
  }

  void movq(Register dst, dart::compiler::LikeABI src) {
    LikeABIRegisters abi_regs(this);
    movq(dst, abi_regs.address(src));
  }

  void movq(dart::compiler::LikeABI dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    movq(dst, scoped.RegReadOnly(src));
  }

  void CompareImmediate(dart::compiler::LikeABI reg,
                        int64_t immediate,
                        OperandSize width = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    CompareImmediate(scoped.RegReadOnly(reg), immediate, width);
  }

  void testl(dart::compiler::LikeABI reg, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    testl(scoped.RegReadOnly(reg), imm);
  }

  void testq(dart::compiler::LikeABI reg, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    testq(scoped.RegReadOnly(reg), imm);
  }

  void AndImmediate(dart::compiler::LikeABI dst, int64_t value) {
    MemoryRegisterProvider scoped(this);
    AndImmediate(scoped.Reg(dst), value);
  }

  void AndRegisters(dart::compiler::LikeABI dst,
                    dart::compiler::LikeABI src1,
                    Register src2 = kNoRegister) {
    MemoryRegisterProvider scoped(this, src2);
    AndRegisters(scoped.Reg(dst), scoped.RegReadOnly(src1), src2);
  }

  void OrImmediate(dart::compiler::LikeABI dst, int64_t value) {
    MemoryRegisterProvider scoped(this);
    OrImmediate(scoped.Reg(dst), value);
  }

  void LslImmediate(dart::compiler::LikeABI dst, int32_t shift) {
    MemoryRegisterProvider scoped(this);
    LslImmediate(scoped.Reg(dst), shift);
  }

  void LsrImmediate(dart::compiler::LikeABI dst, int32_t shift) {
    MemoryRegisterProvider scoped(this);
    LsrImmediate(scoped.Reg(dst), shift);
  }

  void xorq(dart::compiler::LikeABI dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    if (dst != src) {
      xorq(scoped.Reg(dst), scoped.RegReadOnly(src));
    } else {
      movq(scoped.Reg(dst), Immediate(0));
    }
  }

  void andq(dart::compiler::LikeABI reg, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    andq(scoped.Reg(reg), imm);
  }

  void subq(dart::compiler::LikeABI dest, Register src) {
    MemoryRegisterProvider scoped(this, src);
    subq(scoped.Reg(dest), src);
  }

  void subq(dart::compiler::LikeABI dest, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    subq(scoped.Reg(dest), scoped.RegReadOnly(src));
  }

  void orq(const Address& dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    orq(dst, scoped.RegReadOnly(src));
  }

  void orq(dart::compiler::LikeABI dst, Register src) {
    MemoryRegisterProvider scoped(this, src);
    orq(scoped.Reg(dst), src);
  }

  void incq(dart::compiler::LikeABI reg) {
    MemoryRegisterProvider scoped(this);
    incq(scoped.Reg(reg));
  }

  void shlq(dart::compiler::LikeABI reg, Register shifter) {
    MemoryRegisterProvider scoped(this, shifter);
    shlq(scoped.Reg(reg), shifter);
  }

  void shlq(dart::compiler::LikeABI reg, dart::compiler::LikeABI shifter) {
    MemoryRegisterProvider scoped(this);
    auto shifter_proxy = scoped.RegReadOnly(shifter, RCX);
    shlq(scoped.Reg(reg), shifter_proxy);
  }

  void shrq(dart::compiler::LikeABI reg, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    shrq(scoped.Reg(reg), imm);
  }

  void CompareRegisters(dart::compiler::LikeABI a, dart::compiler::LikeABI b) {
    MemoryRegisterProvider scoped(this);
    CompareRegisters(scoped.RegReadOnly(a), scoped.RegReadOnly(b));
  }

  void CompareRegisters(dart::compiler::LikeABI a, Register b) {
    MemoryRegisterProvider scoped(this, b);
    CompareRegisters(scoped.RegReadOnly(a), b);
  }

  void CompareObjectRegisters(dart::compiler::LikeABI a, Register b) {
    MemoryRegisterProvider scoped(this, b);
    CompareObjectRegisters(scoped.RegReadOnly(a), b);
  }

  void CompareObjectRegisters(dart::compiler::LikeABI a,
                              dart::compiler::LikeABI b) {
    MemoryRegisterProvider scoped(this);
    CompareObjectRegisters(scoped.RegReadOnly(a), scoped.RegReadOnly(b));
  }

  void BranchIfBit(dart::compiler::LikeABI rn,
                   intptr_t bit_number,
                   Condition condition,
                   Label* label,
                   JumpDistance distance = kFarJump) {
    MemoryRegisterProvider scoped(this);
    testq(scoped.RegReadOnly(rn), Immediate(1 << bit_number));
    j(condition, label, distance);
  }

  void PushRegister(dart::compiler::LikeABI r) { pushq(r); }

  void PopRegister(dart::compiler::LikeABI r) { popq(r); }

  void AddImmediate(dart::compiler::LikeABI reg,
                    int64_t value,
                    OperandSize width = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    AddImmediate(scoped.Reg(reg), value, width);
  }

  void AddRegisters(dart::compiler::LikeABI dest, Register src) {
    MemoryRegisterProvider scoped(this, src);
    AddRegisters(scoped.Reg(dest), src);
  }

  void AddRegisters(Register dest, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this, dest);
    AddRegisters(dest, scoped.RegReadOnly(src));
  }

  void AddRegisters(dart::compiler::LikeABI dest, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    AddRegisters(scoped.Reg(dest), scoped.RegReadOnly(src));
  }

  void AddImmediate(dart::compiler::LikeABI dest, Register src, int64_t value) {
    MemoryRegisterProvider scoped(this, src);
    AddImmediate(scoped.Reg(dest), src, value);
  }

  void AddImmediate(dart::compiler::LikeABI dest,
                    dart::compiler::LikeABI src,
                    int64_t value) {
    MemoryRegisterProvider scoped(this);
    AddImmediate(scoped.Reg(dest), scoped.RegReadOnly(src), value);
  }

  void AddImmediate(const Address& address, int32_t value) {
    AddImmediate(address, Immediate(value));
  }

  void SubRegisters(dart::compiler::LikeABI dest, Register src) {
    MemoryRegisterProvider scoped(this, src);
    SubRegisters(scoped.Reg(dest), src);
  }

  void SubRegisters(dart::compiler::LikeABI dest, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    SubRegisters(scoped.Reg(dest), scoped.RegReadOnly(src));
  }

  void LoadImmediate(dart::compiler::LikeABI reg, int64_t immediate) {
    MemoryRegisterProvider scoped(this);
    LoadImmediate(scoped.Reg(reg), immediate);
  }

  void LoadObject(dart::compiler::LikeABI dst, const Object& obj) {
    MemoryRegisterProvider scoped(this);
    LoadObject(scoped.Reg(dst), obj);
  }

  void LoadUniqueObject(dart::compiler::LikeABI dst, const Object& obj) {
    MemoryRegisterProvider scoped(this);
    LoadUniqueObject(scoped.Reg(dst, RCX), obj);
  }

  void CompareObject(dart::compiler::LikeABI reg, const Object& object) {
    MemoryRegisterProvider scoped(this);
    CompareObject(scoped.RegReadOnly(reg), object);
  }

  void LoadCompressed(dart::compiler::LikeABI dest, const Address& slot) {
    MemoryRegisterProvider scoped(this);
    LoadCompressed(scoped.Reg(dest), slot);
  }

  void StoreCompressedIntoObject(
      Register object,                // Object we are storing into.
      const Address& dest,            // Where we are storing into.
      dart::compiler::LikeABI value,  // Value we are storing.
      CanBeSmi can_be_smi = kValueCanBeSmi,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    MemoryRegisterProvider scoped(this, object);
    StoreCompressedIntoObject(object, dest, scoped.RegReadOnly(value),
                              can_be_smi, memory_order);
  }

  void StoreIntoObjectNoBarrier(Register object,
                                const Address& dest,
                                dart::compiler::LikeABI value,
                                MemoryOrder memory_order = kRelaxedNonAtomic) {
    MemoryRegisterProvider scoped(this, object);
    StoreIntoObjectNoBarrier(object, dest, scoped.RegReadOnly(value),
                             memory_order);
  }

  void StoreCompressedIntoObjectNoBarrier(
      dart::compiler::LikeABI object,
      const Address& dest,
      dart::compiler::LikeABI value,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    MemoryRegisterProvider scoped(this);
    StoreCompressedIntoObjectNoBarrier(scoped.RegReadOnly(object), dest,
                                       scoped.RegReadOnly(value), memory_order);
  }

  void StoreCompressedIntoObjectNoBarrier(
      Register object,
      const Address& dest,
      dart::compiler::LikeABI value,
      MemoryOrder memory_order = kRelaxedNonAtomic) {
    MemoryRegisterProvider scoped(this, object);
    StoreCompressedIntoObjectNoBarrier(object, dest, scoped.RegReadOnly(value),
                                       memory_order);
  }

  void LockCmpxchgq(const Address& address, dart::compiler::LikeABI reg) {
    MemoryRegisterProvider scoped(this);
    LockCmpxchgq(address, scoped.RegReadOnly(reg));
  }

  struct AnyReg {
    AnyReg(Register in_reg) { reg = in_reg; }

    AnyReg(dart::compiler::LikeABI reg) {
      reg_abi = reg;
      is_reg_abi = true;
    }

    Register reg = Register::kNoRegister;
    dart::compiler::LikeABI reg_abi = dart::compiler::LikeABI::kNoRegister;
    bool is_reg_abi = false;
  };

  void PushRegistersInOrder(std::initializer_list<AnyReg> regs) {
    for (AnyReg reg : regs) {
      if (reg.is_reg_abi) {
        PushRegister(reg.reg_abi);
      } else {
        PushRegister(reg.reg);
      }
    }
  }

  void ExtractClassIdFromTags(Register result, dart::compiler::LikeABI tags) {
    MemoryRegisterProvider scoped(this, result);
    ExtractClassIdFromTags(result, scoped.RegReadOnly(tags));
  }

  void ExtractClassIdFromTags(dart::compiler::LikeABI result,
                              dart::compiler::LikeABI tags) {
    MemoryRegisterProvider scoped(this);
    ExtractClassIdFromTags(scoped.Reg(result), scoped.RegReadOnly(tags));
  }

  void ExtractInstanceSizeFromTags(Register result,
                                   dart::compiler::LikeABI tags) {
    MemoryRegisterProvider scoped(this, result);
    ExtractInstanceSizeFromTags(result, scoped.RegReadOnly(tags));
  }

  void ExtractInstanceSizeFromTags(dart::compiler::LikeABI result,
                                   dart::compiler::LikeABI tags) {
    MemoryRegisterProvider scoped(this);
    ExtractInstanceSizeFromTags(scoped.Reg(result), scoped.RegReadOnly(tags));
  }

  void LoadClassId(dart::compiler::LikeABI result, Register object) {
    MemoryRegisterProvider scoped(this, object);
    LoadClassId(scoped.Reg(result), object);
  }

  void LoadClassId(Register result, dart::compiler::LikeABI object) {
    MemoryRegisterProvider scoped(this, result);
    LoadClassId(result, scoped.RegReadOnly(object));
  }

  void LoadClassById(dart::compiler::LikeABI result, Register class_id) {
    MemoryRegisterProvider scoped(this, class_id);
    LoadClassById(scoped.Reg(result), class_id);
  }

  void LoadClassById(Register result, dart::compiler::LikeABI class_id) {
    MemoryRegisterProvider scoped(this, result);
    LoadClassById(result, scoped.RegReadOnly(class_id));
  }

  void LoadClassById(dart::compiler::LikeABI result,
                     dart::compiler::LikeABI class_id) {
    MemoryRegisterProvider scoped(this);
    LoadClassById(scoped.Reg(result), scoped.RegReadOnly(class_id));
  }

  void CompareClassId(
      dart::compiler::LikeABI object,
      intptr_t class_id,
      dart::compiler::LikeABI scratch = dart::compiler::LikeABI::kNoRegister) {
    MemoryRegisterProvider scoped(this);
    CompareClassId(scoped.RegReadOnly(object), class_id,
                   scoped.RegReadOnly(scratch));
  }

  void LoadClassIdMayBeSmi(dart::compiler::LikeABI result, Register object) {
    MemoryRegisterProvider scoped(this, object);
    LoadClassIdMayBeSmi(scoped.Reg(result), object);
  }

  void LoadClassIdMayBeSmi(dart::compiler::LikeABI result,
                           dart::compiler::LikeABI object) {
    MemoryRegisterProvider scoped(this);
    LoadClassIdMayBeSmi(scoped.Reg(result), scoped.RegReadOnly(object));
  }

  void EnsureHasClassIdInDEBUG(intptr_t cid,
                               dart::compiler::LikeABI src,
                               Register scratch,
                               bool can_be_null = false) {
    MemoryRegisterProvider scoped(this, scratch);
    EnsureHasClassIdInDEBUG(cid, scoped.RegReadOnly(src), scratch, can_be_null);
  }

  void EnsureHasClassIdInDEBUG(intptr_t cid,
                               dart::compiler::LikeABI src,
                               dart::compiler::LikeABI scratch,
                               bool can_be_null = false) {
    MemoryRegisterProvider scoped(this);
    EnsureHasClassIdInDEBUG(cid, scoped.RegReadOnly(src),
                            scoped.RegReadOnly(scratch), can_be_null);
  }

  void EnsureHasClassIdInDEBUG(intptr_t cid,
                               Register src,
                               dart::compiler::LikeABI scratch,
                               bool can_be_null = false) {
    MemoryRegisterProvider scoped(this, src);
    EnsureHasClassIdInDEBUG(cid, src, scoped.RegReadOnly(scratch), can_be_null);
  }

  void SmiTag(dart::compiler::LikeABI reg) {
    MemoryRegisterProvider scoped(this);
    SmiTag(scoped.Reg(reg));
  }

  void SmiUntag(dart::compiler::LikeABI reg) {
    MemoryRegisterProvider scoped(this);
    SmiUntag(scoped.Reg(reg));
  }

  void BranchIfSmi(dart::compiler::LikeABI reg,
                   Label* label,
                   JumpDistance distance = kFarJump) {
    MemoryRegisterProvider scoped(this);
    testq(scoped.RegReadOnly(reg), Immediate(kSmiTagMask));
    j(ZERO, label, distance);
  }

  void LoadFromOffset(dart::compiler::LikeABI dst,
                      const Address& address,
                      OperandSize sz = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    LoadFromOffset(scoped.Reg(dst), address, sz);
  }

  void LoadFromOffset(dart::compiler::LikeABI dst,
                      Register base,
                      int32_t offset,
                      OperandSize sz = kEightBytes) {
    MemoryRegisterProvider scoped(this, base);
    LoadFromOffset(scoped.Reg(dst), base, offset, sz);
  }

  void LoadCompressedField(dart::compiler::LikeABI dst,
                           const FieldAddress& address) {
    MemoryRegisterProvider scoped(this);
    LoadCompressedField(scoped.Reg(dst), address);
  }

  void LoadFieldFromOffset(dart::compiler::LikeABI dst,
                           dart::compiler::LikeABI base,
                           int32_t offset,
                           OperandSize sz = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    LoadFieldFromOffset(scoped.Reg(dst), scoped.RegReadOnly(base), offset, sz);
  }

  void LoadCompressedFieldFromOffset(dart::compiler::LikeABI dst,
                                     Register base,
                                     int32_t offset) {
    MemoryRegisterProvider scoped(this, base);
    LoadCompressedFieldFromOffset(scoped.Reg(dst), base, offset);
  }

  void LoadCompressedFieldFromOffset(Register dst,
                                     dart::compiler::LikeABI base,
                                     int32_t offset) {
    MemoryRegisterProvider scoped(this, dst);
    LoadCompressedFieldFromOffset(dst, scoped.RegReadOnly(base), offset);
  }

  void LoadCompressedFieldFromOffset(dart::compiler::LikeABI dst,
                                     dart::compiler::LikeABI base,
                                     int32_t offset) {
    MemoryRegisterProvider scoped(this);
    LoadCompressedFieldFromOffset(scoped.Reg(dst), scoped.RegReadOnly(base),
                                  offset);
  }

  void LoadIndexedCompressed(Register dst,
                             dart::compiler::LikeABI base,
                             int32_t offset,
                             Register index) {
    MemoryRegisterProvider scoped(this, dst, index);
    LoadIndexedCompressed(dst, scoped.RegReadOnly(base), offset, index);
  }

  void LoadIndexedCompressed(Register dst,
                             Register base,
                             int32_t offset,
                             dart::compiler::LikeABI index) {
    MemoryRegisterProvider scoped(this, dst);
    LoadIndexedCompressed(dst, base, offset, scoped.RegReadOnly(index));
  }

  void LoadIndexedCompressed(dart::compiler::LikeABI dst,
                             dart::compiler::LikeABI base,
                             int32_t offset,
                             dart::compiler::LikeABI index) {
    MemoryRegisterProvider scoped(this);
    LoadIndexedCompressed(scoped.Reg(dst), scoped.RegReadOnly(base), offset,
                          scoped.RegReadOnly(index));
  }

  void LoadIndexedCompressed(dart::compiler::LikeABI dst,
                             Register base,
                             int32_t offset,
                             dart::compiler::LikeABI index) {
    MemoryRegisterProvider scoped(this, base);
    LoadIndexedCompressed(scoped.Reg(dst), base, offset,
                          scoped.RegReadOnly(index));
  }

  void StoreToOffset(dart::compiler::LikeABI src,
                     const Address& address,
                     OperandSize sz = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    StoreToOffset(scoped.Reg(src), address, sz);
  }

  void StoreFieldToOffset(dart::compiler::LikeABI src,
                          Register base,
                          int32_t offset,
                          OperandSize sz = kEightBytes) {
    MemoryRegisterProvider scoped(this, base);
    StoreFieldToOffset(scoped.RegReadOnly(src), base, offset, sz);
  }

  void LoadFromStack(dart::compiler::LikeABI dst, intptr_t depth) {
    MemoryRegisterProvider scoped(this);
    LoadFromStack(scoped.Reg(dst), depth);
  }

  void StoreToStack(dart::compiler::LikeABI src, intptr_t depth) {
    MemoryRegisterProvider scoped(this);
    StoreToStack(scoped.RegReadOnly(src), depth);
  }

  void CompareToStack(dart::compiler::LikeABI src, intptr_t depth) {
    MemoryRegisterProvider scoped(this);
    CompareToStack(scoped.Reg(src), depth);
  }

  void LoadMemoryValue(dart::compiler::LikeABI dst,
                       Register base,
                       int32_t offset) {
    MemoryRegisterProvider scoped(this, base);
    LoadMemoryValue(scoped.Reg(dst), base, offset);
  }

  void LoadUnboxedDouble(FpuRegister dst,
                         dart::compiler::LikeABI base,
                         int32_t offset) {
    MemoryRegisterProvider scoped(this);
    LoadUnboxedDouble(dst, scoped.RegReadOnly(base), offset);
  }

  void LoadAcquire(dart::compiler::LikeABI dst,
                   Register address,
                   int32_t offset = 0,
                   OperandSize size = kEightBytes) {
    MemoryRegisterProvider scoped(this, address);
    LoadAcquire(scoped.Reg(dst), address, offset, size);
  }

  void LoadAcquire(dart::compiler::LikeABI dst,
                   dart::compiler::LikeABI address,
                   int32_t offset = 0,
                   OperandSize size = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    LoadAcquire(scoped.Reg(dst), scoped.RegReadOnly(address), offset, size);
  }

  void LoadAcquireCompressed(Register dst,
                             dart::compiler::LikeABI address,
                             int32_t offset = 0) {
    MemoryRegisterProvider scoped(this, dst);
    LoadAcquireCompressed(dst, scoped.RegReadOnly(address), offset);
  }

  void LoadAcquireCompressed(dart::compiler::LikeABI dst,
                             dart::compiler::LikeABI address,
                             int32_t offset = 0) {
    MemoryRegisterProvider scoped(this);
    LoadAcquireCompressed(scoped.Reg(dst), scoped.RegReadOnly(address), offset);
  }

  void cmpl(dart::compiler::LikeABI value, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    cmpl(scoped.RegReadOnly(value), imm);
  }

  void CompareWithMemoryValue(dart::compiler::LikeABI value,
                              Address address,
                              OperandSize size = kEightBytes) {
    MemoryRegisterProvider scoped(this);
    CompareWithMemoryValue(scoped.RegReadOnly(value), address, size);
  }

  void CombineHashes(dart::compiler::LikeABI dst,
                     dart::compiler::LikeABI other) {
    MemoryRegisterProvider scoped(this);
    CombineHashes(scoped.Reg(dst), scoped.RegReadOnly(other));
  }

  void MaybeTraceAllocation(intptr_t cid,
                            Label* trace,
                            dart::compiler::LikeABI temp_reg,
                            JumpDistance distance = JumpDistance::kFarJump) {
    MemoryRegisterProvider scoped(this);
    MaybeTraceAllocation(cid, trace, scoped.RegReadOnly(temp_reg), distance);
  }

  void TryAllocateObject(intptr_t cid,
                         intptr_t instance_size,
                         Label* failure,
                         JumpDistance distance,
                         Register instance_reg,
                         dart::compiler::LikeABI temp) {
    MemoryRegisterProvider scoped(this, instance_reg);
    TryAllocateObject(cid, instance_size, failure, distance, instance_reg,
                      scoped.RegReadOnly(temp));
  }

  void TryAllocateArray(intptr_t cid,
                        intptr_t instance_size,
                        Label* failure,
                        JumpDistance distance,
                        Register instance,
                        dart::compiler::LikeABI end_address,
                        Register temp) {
    MemoryRegisterProvider scoped(this, instance, temp);
    TryAllocateArray(cid, instance_size, failure, distance, instance,
                     scoped.Reg(end_address), temp);
  }

  void CopyMemoryWords(
      dart::compiler::LikeABI src,
      dart::compiler::LikeABI dst,
      dart::compiler::LikeABI size,
      dart::compiler::LikeABI temp = dart::compiler::LikeABI::kNoRegister) {
    MemoryRegisterProvider scoped(this);
    CopyMemoryWords(scoped.RegReadOnly(src), scoped.Reg(dst),
                    scoped.RegReadOnly(size), scoped.RegReadOnly(temp));
  }

  void BreakpointWithId(int id) {
    pushq(RAX);
    movq(RAX, Immediate(id));
    popq(RAX);
    int3();
  }

  void NopWithId(int id) {
    nop();
    pushq(RAX);
    movq(RAX, Immediate(id));
    popq(RAX);
    nop();
  }

  using AssemblerBase::MoveRegister;

  void MoveRegister(Register dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this, dst);
    MoveRegister(dst, scoped.RegReadOnly(src));
  }

  void MoveRegister(dart::compiler::LikeABI dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    MoveRegister(scoped.Reg(dst), scoped.RegReadOnly(src));
  }

  void MoveRegister(dart::compiler::LikeABI dst, Register src) {
    MemoryRegisterProvider scoped(this, src);
    MoveRegister(scoped.Reg(dst), src);
  }

  using AssemblerBase::LoadFromSlot;

  void LoadFromSlot(dart::compiler::LikeABI dst,
                    dart::compiler::LikeABI base,
                    const Slot& slot) {
    MemoryRegisterProvider scoped(this);
    LoadFromSlot(scoped.Reg(dst), scoped.RegReadOnly(base), slot);
  }

  void LoadFromSlot(dart::compiler::LikeABI dst,
                    Register base,
                    const Slot& slot) {
    MemoryRegisterProvider scoped(this, base);
    LoadFromSlot(scoped.Reg(dst), base, slot);
  }

  void LoadFromSlot(Register dst,
                    dart::compiler::LikeABI base,
                    const Slot& slot) {
    MemoryRegisterProvider scoped(this, dst);
    LoadFromSlot(dst, scoped.RegReadOnly(base), slot);
  }

  using AssemblerBase::StoreToSlotNoBarrier;

  void StoreToSlotNoBarrier(dart::compiler::LikeABI src,
                            Register base,
                            const Slot& slot) {
    MemoryRegisterProvider scoped(this, base);
    StoreToSlotNoBarrier(scoped.RegReadOnly(src), base, slot);
  }

  using AssemblerBase::CompareAbstractTypeNullabilityWith;

  void CompareAbstractTypeNullabilityWith(dart::compiler::LikeABI type,
                                          /*Nullability*/ int8_t value,
                                          dart::compiler::LikeABI scratch) {
    MemoryRegisterProvider scoped(this);
    CompareAbstractTypeNullabilityWith(scoped.RegReadOnly(type), value,
                                       scoped.RegReadOnly(scratch));
  }

  using AssemblerBase::FinalizeHash;

  void FinalizeHash(dart::compiler::LikeABI hash, Register scratch = TMP) {
    MemoryRegisterProvider scoped(this, scratch);
    FinalizeHash(scoped.Reg(hash), scratch);
  }

  using AssemblerBase::LoadTypeClassId;

  void LoadTypeClassId(dart::compiler::LikeABI dst,
                       dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    LoadTypeClassId(scoped.Reg(dst), scoped.RegReadOnly(src));
  }

  void leaq(dart::compiler::LikeABI dst, Address src) {
    MemoryRegisterProvider scoped(this);
    leaq(scoped.Reg(dst), src);
  }

  void cmpq(dart::compiler::LikeABI dst, Address src) {
    MemoryRegisterProvider scoped(this);
    cmpq(scoped.RegReadOnly(dst), src);
  }

  void cmpq(dart::compiler::LikeABI dst, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    cmpq(scoped.RegReadOnly(dst), imm);
  }

  void cmpq(dart::compiler::LikeABI dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    cmpq(scoped.RegReadOnly(dst), scoped.RegReadOnly(src));
  }

  void cmpq(dart::compiler::LikeABI dst, Register src) {
    MemoryRegisterProvider scoped(this, src);
    cmpq(scoped.RegReadOnly(dst), src);
  }

  void cmpq(Register dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this, dst);
    cmpq(dst, scoped.RegReadOnly(src));
  }

  void addq(dart::compiler::LikeABI dst, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    addq(scoped.Reg(dst), imm);
  }

  void addq(dart::compiler::LikeABI dst, Register src) {
    MemoryRegisterProvider scoped(this, src);
    addq(scoped.Reg(dst), src);
  }

  void addq(Register dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this, dst);
    addq(dst, scoped.RegReadOnly(src));
  }

  void addq(dart::compiler::LikeABI dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    addq(scoped.Reg(dst), scoped.RegReadOnly(src));
  }

  void testq(dart::compiler::LikeABI dst, dart::compiler::LikeABI src) {
    MemoryRegisterProvider scoped(this);
    testq(scoped.RegReadOnly(dst), scoped.RegReadOnly(src));
  }

  void LoadIsolate(dart::compiler::LikeABI dst) {
    MemoryRegisterProvider scoped(this);
    LoadIsolate(scoped.Reg(dst));
  }

  void LoadTaggedClassIdMayBeSmi(dart::compiler::LikeABI result,
                                 Register object) {
    MemoryRegisterProvider scoped(this, object);
    LoadTaggedClassIdMayBeSmi(scoped.Reg(result), object);
  }

  void LoadTaggedClassIdMayBeSmi(Register result,
                                 dart::compiler::LikeABI object) {
    MemoryRegisterProvider scoped(this, result);
    LoadTaggedClassIdMayBeSmi(result, scoped.RegReadOnly(object));
  }

  void LoadTaggedClassIdMayBeSmi(dart::compiler::LikeABI result,
                                 dart::compiler::LikeABI object) {
    MemoryRegisterProvider scoped(this);
    LoadTaggedClassIdMayBeSmi(scoped.Reg(result), scoped.RegReadOnly(object));
  }

  void movzxw(dart::compiler::LikeABI dst, Address src) {
    MemoryRegisterProvider scoped(this);
    movzxw(scoped.Reg(dst), src);
  }

  void LoadUnboxedSingle(FpuRegister dst,
                         dart::compiler::LikeABI base,
                         int32_t offset) {
    MemoryRegisterProvider scoped(this);
    LoadUnboxedSingle(dst, scoped.RegReadOnly(base), offset);
  }

  void CheckAllocationCanary(dart::compiler::LikeABI top) {
    MemoryRegisterProvider scoped(this);
    CheckAllocationCanary(scoped.RegReadOnly(top));
  }

  void WriteAllocationCanary(dart::compiler::LikeABI top) {
    MemoryRegisterProvider scoped(this);
    WriteAllocationCanary(scoped.RegReadOnly(top));
  }

  void andq(dart::compiler::LikeABI reg, const Address& src) {
    MemoryRegisterProvider scoped(this);
    andq(scoped.Reg(reg), src);
  }

  void shrl(dart::compiler::LikeABI reg, const Immediate& imm) {
    MemoryRegisterProvider scoped(this);
    shrl(scoped.Reg(reg), imm);
  }

 private:
  void LoadWordFromPoolIndex(dart::compiler::LikeABI dst, intptr_t index) {
    MemoryRegisterProvider scoped(this);
    LoadWordFromPoolIndex(scoped.Reg(dst, RCX), index);
  }

  bool constant_pool_allowed_;

  bool CanLoadFromObjectPool(const Object& object) const;
  void LoadObjectHelper(Register dst, const Object& obj, bool is_unique);
  void LoadWordFromPoolIndex(Register dst, intptr_t index);

  void AluL(uint8_t modrm_opcode, Register dst, const Immediate& imm);
  void AluB(uint8_t modrm_opcode, const Address& dst, const Immediate& imm);
  void AluW(uint8_t modrm_opcode, const Address& dst, const Immediate& imm);
  void AluL(uint8_t modrm_opcode, const Address& dst, const Immediate& imm);
  void AluQ(uint8_t modrm_opcode,
            uint8_t opcode,
            Register dst,
            const Immediate& imm);
  void AluQ(uint8_t modrm_opcode,
            uint8_t opcode,
            const Address& dst,
            const Immediate& imm);

  void EmitSimple(int opcode, int opcode2 = -1, int opcode3 = -1);
  void EmitUnaryQ(Register reg, int opcode, int modrm_code);
  void EmitUnaryL(Register reg, int opcode, int modrm_code);
  void EmitUnaryQ(const Address& address, int opcode, int modrm_code);
  void EmitUnaryL(const Address& address, int opcode, int modrm_code);
  // The prefixes are in reverse order due to the rules of default arguments in
  // C++.
  void EmitQ(int reg,
             const Address& address,
             int opcode,
             int prefix2 = -1,
             int prefix1 = -1);
  void EmitL(int reg,
             const Address& address,
             int opcode,
             int prefix2 = -1,
             int prefix1 = -1);
  void EmitW(Register reg,
             const Address& address,
             int opcode,
             int prefix2 = -1,
             int prefix1 = -1);
  void EmitQ(int dst, int src, int opcode, int prefix2 = -1, int prefix1 = -1);
  void EmitL(int dst, int src, int opcode, int prefix2 = -1, int prefix1 = -1);
  void EmitW(Register dst,
             Register src,
             int opcode,
             int prefix2 = -1,
             int prefix1 = -1);
  void EmitB(int reg, const Address& address, int opcode);
  void CmpPS(XmmRegister dst, XmmRegister src, int condition);

  inline void EmitUint8(uint8_t value);
  inline void EmitInt32(int32_t value);
  inline void EmitUInt32(uint32_t value);
  inline void EmitInt64(int64_t value);

  inline void EmitRegisterREX(Register reg,
                              uint8_t rex,
                              bool force_emit = false);
  inline void EmitOperandREX(int rm, const Operand& operand, uint8_t rex);
  inline void EmitRegisterOperand(int rm, int reg);
  inline void EmitFixup(AssemblerFixup* fixup);
  inline void EmitOperandSizeOverride();
  inline void EmitRegRegRex(int reg, int base, uint8_t rex = REX_NONE);
  void EmitOperand(int rm, const Operand& operand);
  void EmitImmediate(const Immediate& imm);
  void EmitComplex(int rm, const Operand& operand, const Immediate& immediate);
  void EmitSignExtendedInt8(int rm,
                            const Operand& operand,
                            const Immediate& immediate);
  void EmitLabel(Label* label, intptr_t instruction_size);
  void EmitLabelLink(Label* label);
  void EmitNearLabelLink(Label* label);

  void EmitGenericShift(bool wide, int rm, Register reg, const Immediate& imm);
  void EmitGenericShift(bool wide, int rm, Register operand, Register shifter);

  enum BarrierFilterMode {
    // Filter falls through into the barrier update code. Target label
    // is a "after-store" label.
    kJumpToNoUpdate,

    // Filter falls through to the "after-store" code. Target label
    // is barrier update code label.
    kJumpToBarrier,
  };

  void StoreIntoArrayBarrier(Register object,
                             Register slot,
                             Register value,
                             CanBeSmi can_be_smi = kValueCanBeSmi);

  // Unaware of write barrier (use StoreInto* methods for storing to objects).
  void MoveImmediate(const Address& dst, const Immediate& imm);

  friend class dart::FlowGraphCompiler;
  std::function<void(Register reg)> generate_invoke_write_barrier_wrapper_;
  std::function<void()> generate_invoke_array_write_barrier_;

  DISALLOW_ALLOCATION();
  DISALLOW_COPY_AND_ASSIGN(Assembler);
};

inline void Assembler::EmitUint8(uint8_t value) {
  buffer_.Emit<uint8_t>(value);
}

inline void Assembler::EmitInt32(int32_t value) {
  buffer_.Emit<int32_t>(value);
}

inline void Assembler::EmitUInt32(uint32_t value) {
  buffer_.Emit<uint32_t>(value);
}

inline void Assembler::EmitInt64(int64_t value) {
  buffer_.Emit<int64_t>(value);
}

inline void Assembler::EmitRegisterREX(Register reg, uint8_t rex, bool force) {
  ASSERT(reg != kNoRegister && reg <= R15);
  ASSERT(rex == REX_NONE || rex == REX_W);
  rex |= (reg > 7 ? REX_B : REX_NONE);
  if (rex != REX_NONE || force) EmitUint8(REX_PREFIX | rex);
}

inline void Assembler::EmitOperandREX(int rm,
                                      const Operand& operand,
                                      uint8_t rex) {
  rex |= (rm > 7 ? REX_R : REX_NONE) | operand.rex();
  if (rex != REX_NONE) EmitUint8(REX_PREFIX | rex);
}

inline void Assembler::EmitRegRegRex(int reg, int base, uint8_t rex) {
  ASSERT(reg != kNoRegister && reg <= R15);
  ASSERT(base != kNoRegister && base <= R15);
  ASSERT(rex == REX_NONE || rex == REX_W);
  if (reg > 7) rex |= REX_R;
  if (base > 7) rex |= REX_B;
  if (rex != REX_NONE) EmitUint8(REX_PREFIX | rex);
}

inline void Assembler::EmitFixup(AssemblerFixup* fixup) {
  buffer_.EmitFixup(fixup);
}

inline void Assembler::EmitOperandSizeOverride() {
  EmitUint8(0x66);
}

}  // namespace compiler
}  // namespace dart

#endif  // RUNTIME_VM_COMPILER_ASSEMBLER_ASSEMBLER_X64_H_
