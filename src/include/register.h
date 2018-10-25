// register.h
// header for register class

#include <iostream>
#include <cassert>


#ifndef REGISTER_H
#define REGISTER_H

namespace cool {

class Register;
bool operator!=(const Register& lhs, const Register& rhs);

// register classes
class Register {
 public:
  Register(const char *name): name_(name) {}
  ~Register() {}
  virtual std::size_t size() const { return -1; }
  const char *name() const { return name_; }
  
  friend std::ostream& operator<<(std::ostream& os, const Register& reg);
  friend bool operator==(const Register& lhs, const Register& rhs);
 private:
  const char *name_;
};

class Register8: public Register {
 public:
  Register8(const char *name): Register(name) {}
  ~Register8() {}
  std::size_t size() const override { return 1; }
};

class Register16: public Register {
 public:
  Register16(const char *name, const Register8& high, const Register8& low): Register(name), high_(high), low_(low) {}
  ~Register16() {}
  std::size_t size() const override { return 2; }
  const Register8& high() const { return high_; }
  const Register8& low() const { return low_; }
 private:
  const Register8& high_, low_;
};

class Register16X: public Register16 {
 public:
  Register16X(const char *name, const Register8& high, const Register8& low): Register16(name, high, low) {}
  ~Register16X() {}
};

// now define registers
extern const Register8 rNIL;
extern const Register8 rA;
extern const Register8 &ACC;
extern const Register8 rB;
extern const Register8 rC;
extern const Register8 rD;
extern const Register8 rE;
extern const Register8 rH;
extern const Register8 rL;
extern const Register8 rIXH;
extern const Register8 rIXL;
extern const Register8 rIYH;
extern const Register8 rIYL;
 
extern const Register16 rHL;
extern const Register16 &ARG0;
extern const Register16 rBC;
extern const Register16 rDE;
extern const Register16 rSP;
extern const Register16& SP;
extern const Register16X rIX;
extern const Register16X &SELF;
extern const Register16X rIY;
extern const Register16X &FP;

////

typedef const char * Flag;
struct Flags {
	static constexpr Flag C = "c";
	static constexpr Flag NC = "nc";
	static constexpr Flag Z = "z";
	static constexpr Flag NZ = "nz";
	static constexpr Flag none = nullptr;
};

// memory locations (i.e. addresses/pointers/etc)
class MemoryLocation {
 public:
  MemoryLocation() {}
  ~MemoryLocation() {}
  virtual std::ostream& print(std::ostream& os) const { return os; }
  virtual std::size_t size() const { return 2; }
  
  enum Kind { NONE, ABS, PTR, PTR_OFF };
  virtual Kind kind() const { throw "only specializations of MemoryLocation are allowed"; return NONE; }
  friend std::ostream& operator<<(std::ostream& os, const MemoryLocation& loc);
  virtual MemoryLocation &operator[](int offset) const { throw std::string("cannot subscript MemoryLocation base class"); }
 
  MemoryLocation *copy() const;
 private:
};

class AbsoluteAddress: public MemoryLocation {
 public:
  AbsoluteAddress(): label_("$0000"), offset_(0) {} // default constructor, defaults to nullptr
  AbsoluteAddress(std::string label, int16_t offset = 0): label_(label), offset_(offset) {}
  ~AbsoluteAddress() {}
  std::size_t size() const override { return 0; }	// variable size
  std::ostream& print(std::ostream& os) const override;
  Kind kind() const override { return ABS; }
  AbsoluteAddress &advanced(int16_t d) const;
  MemoryLocation &operator[](int d) const override { assert (((int16_t) d) == d); return advanced((int16_t) d); }
  friend bool operator<(const AbsoluteAddress& lhs, const AbsoluteAddress& rhs);
  int16_t offset() const { return offset_; }
  friend class CgenNode;
 private:
  std::string label_;
  const int16_t offset_;
};


class RegisterPointer: public MemoryLocation {
 public:
  RegisterPointer(const Register16& reg): reg_(reg) {}
  ~RegisterPointer() {}
  std::size_t size() const override { return 1; }
  std::ostream& print(std::ostream& os) const override;
  Kind kind() const override { return PTR; }
  const Register16& reg() const { return reg_; }
  MemoryLocation &operator[](int d) const override { throw "register pointer cannot be indexed"; }
 private:
  const Register16& reg_;
};

class RegisterPointerOffset: public RegisterPointer {
 public:
  RegisterPointerOffset(const Register16X& reg, uint8_t offset): RegisterPointer(reg), reg_(reg), offset_(offset) {}
  RegisterPointerOffset(const RegisterPointerOffset& old):  RegisterPointer(old.reg()), reg_(old.reg()), offset_(old.offset()) {}
  ~RegisterPointerOffset() {}
  std::ostream& print(std::ostream& os) const override;
  std::size_t size() const override { return 1; }
  Kind kind() const override { return PTR_OFF; }
  const Register16X& reg() const { return reg_; }
  int8_t offset() const { return offset_; }
  RegisterPointerOffset &advanced(uint8_t d) const;
  MemoryLocation &operator[](int d) const override { assert (((uint8_t) d) == d); return advanced((uint8_t) d); }
 private:
  const Register16X& reg_;
  const uint8_t offset_;
};

// values
class Value {
 public:
  Value() {}
  ~Value() {}
  
  virtual std::size_t size() const { return -1; }
  enum Kind { NONE, IMM, LBL, REG, MEM };
  virtual Kind kind() const { throw "only usage of specializations of Value are allowed"; return NONE; }
//   virtual bool compatible(const Value& val) const { return this.size() == val.size() || val.size() == 0; }
  static bool compatible(const Value& src, const Value& dst);
  friend std::ostream& operator<<(std::ostream& os, const Value& val);
  virtual std::ostream& print(std::ostream& os) const { return os; }
  virtual const Register *reg() const { return nullptr; }
};

template <class T, class U>
class ImmediateValue: public Value {
 public:
  ImmediateValue(T imm): imm_(imm) { assert (sizeof(T) == sizeof(U)); }
  ImmediateValue(U imm): imm_(*((T*) &imm)) { assert (sizeof(T) == sizeof(U)); }
  ~ImmediateValue() {}
  std::size_t size() const override { return sizeof(T); }
  Kind kind() const override { return IMM; }
  std::ostream& print(std::ostream& os) const override {
	os << std::to_string(imm_);
	return os;
  }
 private:
  const T imm_;
};
typedef ImmediateValue<uint8_t, int8_t> Immediate8;
typedef ImmediateValue<uint16_t, int16_t> Immediate16;

class LabelValue: public Value {
 public:
 	LabelValue(const char *label): label_(std::string(label)) {}
 	LabelValue(const std::string& label): label_(label) {}
 	~LabelValue() {}
 	std::size_t size() const override { return 2; }
 	Kind kind() const override { return LBL; }
 	std::ostream& print(std::ostream& os) const override;
 private:
 	const std::string label_;
};

class RegisterValue: public Value {
 public:
  RegisterValue(const Register& reg): reg_(reg) {}
  ~RegisterValue() {}
  std::size_t size() const override;
  Kind kind() const override { return REG; }
  const Register *reg() const override { return &reg_; }
  std::ostream& print(std::ostream& os) const override;
 private:
  const Register& reg_;
};
 
class MemoryValue: public Value {
 public:
  MemoryValue(const MemoryLocation& loc): loc_(loc) {}
  ~MemoryValue() {}
  std::size_t size() const override;
  Kind kind() const override { return MEM; }
  const MemoryLocation& loc() const { return loc_; }
  std::ostream& print(std::ostream& os) const override;  
  const Register *reg() const override;
  MemoryValue operator[](int16_t offset) const { return MemoryValue(loc_[offset]); }
 private:
  const MemoryLocation& loc_;
};

}

#endif