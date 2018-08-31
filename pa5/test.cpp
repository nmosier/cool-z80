#include <iostream>

class Value {
public:
  Value() {}
  virtual void print(std::ostream& os) const {}
};

class IntValue: public Value {
public:
  IntValue(int val): val_(val) {}
  void print(std::ostream& os) const override { os << val_; } 
private:
  int val_;
};

class VariableValue: public Value {
public:
  VariableValue(const std::string& name): name_(name) {}
  void print(std::ostream& os) const override { os << name_; }
private:
  const std::string name_;
};

void emit_add(const Value& lhs, const Value& rhs, std::ostream& os) {
  lhs.print(os);
  os << " + ";
  rhs.print(os);
  os << std::endl;
}

template <class ValueType>
void emit_add(const ValueType& lhs, const ValueType& rhs, std::ostream &os) {
  lhs.print(os);
  os << " + ";
  rhs.print(os);
  os << std::endl;
}

int main() {
  // all these work
  emit_add<IntValue>(12, 13, std::cout); // implicit constructors
  emit_add<VariableValue>(std::string("x"), std::string("y"), std::cout); // implicit constructors
  emit_add(VariableValue(std::string("x")), IntValue(1), std::cout); // implicit upcasting

  // this doesn't
  emit_add(std::string("x"), 13, std::cout); // implicit constor + implicit upcasting

  return -1;
}
