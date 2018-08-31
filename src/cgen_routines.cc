#include "cgen_routines.h"
#include "routines.cc"

namespace cool {

std::vector<Routine *> Routine::routines; // needs to be initialized out-of-line (ok?)

const Routine MUL_DE_A("MUL_DE_A", [](const std::string& label, std::ostream& os){
	const std::string label_loop = label + "_loop";
	const std::string label_skip = label + "_skip";
	
	os << label << LABEL;
	
	os << LD << "hl,0" << std::endl;
	os << LD << "b,8" << std::endl;
	os << label_loop << LABEL;
	os << ADD << "hl,hl" << std::endl;
	os << "\trlca" << std::endl;
	os << JR << "nc," << label_skip << std::endl;
	os << ADD << "hl,de" << std::endl;
	os << label_skip << LABEL;
	os << "\tdjnz\t" << label_loop << std::endl;
	os << RET << std::endl;
});

const Routine MUL_C_D("MUL_C_D", [](const std::string& label, std::ostream& os){
	// result is in ACC
	const std::string label_loop = label + "_loop";
	const std::string label_skip = label + "_skip";
	
	os << label << LABEL;
	
	os << XOR << "a" << std::endl;
	os << LD << "b,8" << std::endl;
	os << label_loop << LABEL;
	os << ADD << "a,a" << std::endl;
	os << JR << "nc," << label_skip << std::endl;
	os << label_skip << LABEL;
	os << DJNZ << label_loop << std::endl;
	os << RET << std::endl;
});

const Routine DIV_HL_D("DIV_HL_D", [](const std::string& label, std::ostream& os){
	// remainer is in ACC
	// quotient is in HL
	const std::string label_loop = label + "_loop";
	const std::string label_overflow = label + "_overflow";
	const std::string label_skip = label + "_skip";
	
	os << label << LABEL;
	
	os << XOR << "a" << std::endl;
	os << LD << "b,16" << std::endl;
	os << label_loop << LABEL;
	os << ADD << "hl,hl" << std::endl;
	os << RLA << std::endl;
	os << JR << "c," << label_skip << std::endl;
	os << label_overflow << LABEL;
	os << SUB << "d" << std::endl;
	os << INC << "l" << std::endl;
	os << label_skip << LABEL;
	os << DJNZ << label_loop << std::endl;
	os << RET << std::endl;
});

const Routine DIV_HL_DE("DIV_HL_DE", [](const std::string& label, std::ostream& os){
	const std::string l_loop = label + "_loop";
	const std::string l_overflow = label + "_overflow";
	const std::string l_skip = label + "_skip";
	
	os << label << LABEL;
	
	os << XOR << "a" << std::endl;
	os << LD << "bc,0" << std::endl;
	os << PUSH << "bc" << std::endl;
	os << LD << "b,16" << std::endl;
	os << l_loop << LABEL;
	os << ADD << "hl,hl" << std::endl;
	os << EX << "(sp),hl" << std::endl;
	os << RL << "l" << std::endl;
	os << RL << "h" << std::endl;
	os << JR << "c," << l_overflow << std::endl;
	os << LD << "a,h" << std::endl;
	os << CP << "d" << std::endl;
	os << JR << "c," << l_skip << std::endl;
	os << LD << "a,l" << std::endl;
	os << CP << "e" << std::endl;
	os << JR << "c," << l_skip << std::endl;
	os << l_overflow << LABEL;
	os << SCF << std::endl;
	os << CCF << std::endl;
	os << SBC << "hl,de" << std::endl;
	os << EX << "(sp),hl" << std::endl;
	os << INC << "hl" << std::endl;
	os << DJNZ << l_loop << std::endl;
	os << RET << std::endl;
	os << l_skip << LABEL;
	os << EX << "(sp),hl" << std::endl;
	os << DJNZ << l_loop << std::endl;
	os << RET << std::endl;
});


const Routine MALLOC("_malloc", [](const std::string& label, std::ostream& os) {
	os << std::string(reinterpret_cast<char *>(memory_z80)) << std::endl;
});

/// OBJECT METHODS
const Routine Object_Copy("Object.copy", [](const std::string& label, std::ostream& os) {
	os << label << LABEL;
	
	os << LD << "e,(hl)" << std::endl;
	os << INC << "hl" << std::endl;
	os << LD << "d,(hl)" << std::endl;
	os << EX << "de,hl" << std::endl;
	os << INC << "hl" << std::endl;
	os << INC << "hl" << std::endl;
	os << LD << "e,(hl)" << std::endl;
	os << INC << "hl" << std::endl;
	os << LD << "d,(hl)" << std::endl;
	os << EX << "de,hl" << std::endl;
	os << CALL << "_malloc" << std::endl;
	os << RET << std::endl;
});


}