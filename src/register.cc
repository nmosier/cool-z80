#include "register.h"
#include <iostream>
#include <functional>
#include "utilities.h"

namespace cool {

// now define registers
const Register8 rNIL = (const char *) nullptr;
const Register8 rA = "a";
const Register8 &ACC = rA;	// ACC is alias for register A
const Register8 rB = "b";
const Register8 rC = "c";
const Register8 rD = "d";
const Register8 rE = "e";
const Register8 rH = "h";
const Register8 rL = "l";
const Register8 rIXH = "ixh";
const Register8 rIXL = "ixl";
const Register8 rIYH = "iyh";
const Register8 rIYL = "iyl";

const Register16 rHL("hl", rH, rL);
const Register16 &ARG0 = rHL;
const Register16 rBC("bc", rB, rC);
const Register16 rDE("de", rD, rE);
const Register16 rSP("sp", rNIL, rNIL);
const Register16& SP = rSP;
const Register16X rIX("ix", rIXH, rIXL);
const Register16X &SELF = rIX;
const Register16X rIY("iy", rIYH, rIYL);
const Register16X &FP = rIY;





// register
std::ostream& operator<<(std::ostream& os, const Register& reg) {
	os << reg.name_;
	return os;
}

bool operator==(const Register& lhs, const Register& rhs) {
	return lhs.name_ == rhs.name_;
}
bool operator!=(const Register& lhs, const Register& rhs) {
	return !(lhs == rhs);
}

// memory location
std::ostream& operator<<(std::ostream& os, const MemoryLocation& loc) {
	return loc.print(os);
}

std::ostream& AbsoluteAddress::print(std::ostream& os) const {
	os << label_;
	if (offset_ != 0) {
		os << "+" << offset_;
	}
	return os;
}

AbsoluteAddress &AbsoluteAddress::advanced(int16_t d) const {
	assert (((int) offset_) + ((int) d) == (int) (offset_ + d));
	return *(new AbsoluteAddress(label_, offset_ + d));
}

bool operator<(const AbsoluteAddress& lhs, const AbsoluteAddress& rhs) {
	if (lhs.label_ != rhs.label_) {
		std::cerr << "different labels " << lhs.label_ << " and " << rhs.label_ << " cannot be compared." << std::endl;
      print_backtrace();
		throw "different labels cannot be compared.";
	}
	return lhs.offset_ < rhs.offset_;
}

std::ostream& RegisterPointer::print(std::ostream& os) const {
	os << reg_;
	return os;
}

std::ostream& RegisterPointerOffset::print(std::ostream& os) const {
	os << reg_ << "+" << std::to_string(offset_);
	//if (offset_ >= 0) os << "+" << std::to_string(offset_);
	//else os << "-" << std::to_string(-offset_);
	return os;
}

RegisterPointerOffset &RegisterPointerOffset::advanced(uint8_t d) const {
	const uint8_t this_offset = offset();
	const int new_offset = ((int) this_offset) + ((int) d);
	assert ((uint8_t) new_offset == new_offset);
	return *(new RegisterPointerOffset(reg(), (uint8_t) this_offset + d));
}

// values
// template <class T, class U>
// std::size_t ImmediateValue<T,U>::size() const {
// 	return sizeof(T);
// }
std::size_t RegisterValue::size() const {
	return reg_.size();
}
std::size_t MemoryValue::size() const {
	return 0; // flexible size, depending on other operands
}


std::ostream& operator<<(std::ostream& os, const Value& val) {
	return val.print(os);
}

// template <class T, class U>
// std::ostream& ImmediateValue<T,U>::print(std::ostream& os) const {
// 	os << imm_;
// 	return os;
// }

std::ostream& LabelValue::print(std::ostream& os) const {
	os << label_;
	return os;
}

std::ostream& RegisterValue::print(std::ostream& os) const {
	os << reg_;
	return os;
}

std::ostream& MemoryValue::print(std::ostream& os) const {
	os << "(" << loc_ << ")";
	return os;
}

const Register *MemoryValue::reg() const {
	const Register *result;
	switch (loc().kind()) {
	case MemoryLocation::Kind::ABS:
		result = nullptr;
		break;
	case MemoryLocation::Kind::PTR:
	case MemoryLocation::Kind::PTR_OFF:
		result = &(((const RegisterPointer&) loc()).reg());
		break;
	default:
		result = nullptr;
		break;
	}
	return result;
}

bool operator<(const Value::Kind lhs, const Value::Kind rhs) {
	const int lhs_ = *((const int *) &lhs);
	const int rhs_ = *((const int *) &rhs);
	return lhs_ < rhs_;
}



MemoryLocation *MemoryLocation::copy() const {
  	switch (kind()) {
  	case NONE:
  		return new MemoryLocation();
  	case ABS:
  		return new AbsoluteAddress(*((AbsoluteAddress *) this));
  	case PTR:
  		return new RegisterPointer(*((RegisterPointer *) this));
  	case PTR_OFF:
  		return new RegisterPointerOffset(*((RegisterPointerOffset *) this));
  	default:
  		return nullptr;
  	}
  }


}
