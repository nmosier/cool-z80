#include <iostream>
#include <vector>
#include "emit.h"

namespace cool {


	class Routine {
	public:
		Routine(const std::string& label, void (*cgen)(const std::string& label, std::ostream& os)): label_(label), cgen_(cgen) { routines.push_back(this); }
		~Routine() {}
		
		std::ostream& def(std::ostream& os) { if (!defined) { cgen_(label_, os); defined = true; } return os; }
		const std::string& ref() const { return label_; }
		std::ostream& ref(std::ostream& os) const { return os << label_ << std::endl; }
		
		static std::vector<Routine *> routines;
		static Routine *find(const std::string& label) {
			for (Routine *routine : routines) {
				if (routine->label_ == label) {
					return routine;
				}
			}
			return nullptr;
		}
	private:
		bool defined = false;	// this will be changed by def(), so won't get defined twice
		const std::string label_;
		void (*cgen_)(const std::string& label, std::ostream& os);
	};


} // namespace cool