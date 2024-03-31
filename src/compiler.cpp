#include "compiler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

bool Compiler::read_file()
{
    std::ifstream f;
    f.open(file_path);
    
    if (f.fail()) {
	std::cerr << "Error: \n\tCould not open file " << file_path << ": "  << strerror(errno) << std::endl;
	return false;
    }

    std::stringstream buf;
    buf << f.rdbuf();

    source = buf.str();
    return true;
}


// Tokenization

std::string ltrim(std::string str)
{
    std::stringstream res;
    int i = 0;
    for (; i < str.size(); i++) {
	if (!std::isspace(str[i])) {
	    break;
	}
    }

    for (; i < str.size(); i++) {
	res << str[i];
    }

    return res.str();
}

std::string rtrim(std::string str)
{
    std::stringstream res;
    int i = str.size() - 1;
    for (; i > 0; i--) {
	if (!std::isspace(str[i])) {
	    break;
	}
    }
    
    for (int j = 0; j <= i; j++) {
	res << str[j];
    }

    return res.str();
}

std::string trim(std::string str)
{
    return rtrim(ltrim(str));
}

 std::vector<std::string> Compiler::tokenize(std::string src, char delim)
{
    std::vector<std::string> tokens;
    std::stringstream tokens_stream(src);
    std::string token;
    
    while (getline(tokens_stream, token, delim)) {
	token = trim(token);
	if (!token.empty()) {
	    tokens.push_back(trim(token));
	}
    }

    return tokens;
}

// Compilation (more like Parsing but whatever)

std::array<int, 2> Compiler::find_section(std::vector<std::string> tokens, const char* section) {
    std::string section_str(section);
    std::string to_find_start = "section " + section_str + ":";
    std::string to_find_end = "end " + section_str;
    
    std::array<int, 2> idx = {-1, -1};
    
    for (int i = 0; i < tokens.size(); i++) {
	if (tokens[i] == to_find_start) {
	    idx[0] = i + 1;
	}
	else if (tokens[i] == to_find_end) {
	    idx[1] = i - 1;
	}
    }

    return idx;
}

bool overlap(std::array<int, 2> ar1, std::array<int, 2> ar2)
{
    // No overlap:
    // ar1_0 ---------- ar1_1       ar2_0 ------------- ar2_1
    // OR
    // ar2_0 ---------- ar2_1       ar1_0 ------------- ar1_1

    if (!((ar1[0] > ar2[0] && ar1[0] > ar2[1] && ar1[1] > ar2[0] && ar1[1] > ar2[1])
	  || (ar2[0] > ar1[0] && ar2[0] > ar1[1] && ar2[1] > ar1[0] && ar2[1] > ar1[1]))) {
	return true;
    }

    return false;
}

bool is_str_alpha(std::string str) {
    for (char c : str) {
	if (!std::isalpha(c)) return false;
    }
    return true;
}

bool is_number(std::string str) {
    int i = 0;
    if (str[0] == '-') {
	i += 1;
    }
    
    for (; i < str.size(); i++) {
	if (!std::isdigit(str[i])) {
	    return false;
	}
    }

    return true;
}

bool is_ident(std::string str) {
    if (std::isdigit(str[0])) return false;

    for (char c : str) {
	if (!std::isalnum(c)) return false;
    }

    return true;
}

std::vector<int32_t> Compiler::get_value(std::string value)
{
    std::vector<int32_t> result;
    bool is_reading_string = false;
    std::stringstream buf;
    
    for (char c : value) {
	if (c == '"') {
	    is_reading_string = !is_reading_string;
	    continue;
	}

	if (is_reading_string) {
	    result.push_back((int32_t) c);
	} else {
	    if (c == ' ') {
		if (!(trim(buf.str()).size() == 0)) {
		    if (!is_number(buf.str())) {
			std::cout << buf.str() << std::endl;
			std::cerr << file_path << ": Error: \n\tUnexpected token " << buf.str() << "." << std::endl;
			return std::vector<int32_t>();
		    }
		    result.push_back(std::stoi(buf.str()));
		    buf.str(std::string());
		} else {
		    continue;
		}
	    } else {
		buf << c;
	    }
	}
    }

    return result;
}

uint16_t Compiler::push_const_to_mem(std::vector<int32_t> val)
{
    uint16_t ptr = 1;
    uint16_t val_size = val.size();
    uint16_t MEM_SIZE = MEM.cells.size();

    while (ptr < MEM_SIZE) {
	MemCell temp_cell = {false, 0};
	for (int i = 0; i < val_size; i++) {
	    temp_cell = MEM.cells[ptr + i];
	    if (temp_cell.occupied) {
		ptr += val_size;
		break;
	    }
	}

	if (temp_cell.occupied) continue;

	if (ptr > MEM_SIZE - val_size) {
	    return 0;
	}
	
	for (int i = 0; i < val_size; i++) {
	    MEM.cells[ptr + i].value = val[i];
	    MEM.cells[ptr + i].occupied = true;
	}

	return ptr;
    }

    return 0;
}

uint16_t Compiler::alloc_mem(uint16_t val_size)
{
    uint16_t ptr = 1;
    uint16_t MEM_SIZE = MEM.cells.size();

    while (ptr < MEM_SIZE) {
	MemCell temp_cell = {false, 0};
	for (int i = 0; i < val_size; i++) {
	    temp_cell = MEM.cells[ptr + i];
	    if (temp_cell.occupied) {
		ptr += val_size;
		break;
	    }
	}

	if (temp_cell.occupied) continue;

	if (ptr > MEM_SIZE - val_size) {
	    return 0;
	}
	
	for (int i = 0; i < val_size; i++) {
	    MEM.cells[ptr + i].value = 0;
	    MEM.cells[ptr + i].occupied = true;
	}

	return ptr;
    }

    return 0;
}

bool Compiler::update_mem(uint16_t ptr, int32_t val)
{
    MEM.cells[ptr].value = val;
    return true;
}

void Compiler::init_mem()
{
    const MemCell default_cell = {false, 0};
    MEM.cells.fill(default_cell);
    MEM.regs.fill(0);
    return;
}

void Compiler::print_mem(int idx)
{
    for (int i = 0; i < idx; i++) {
	MemCell cell = MEM.cells[i];
	if (cell.occupied) {
	    printf("Cell %i: occupied, holding value %i\n", i, MEM.cells[i].value);
	} else {
	    printf("Cell %i: not occupied\n", i);
	}
    }
    return;
}

bool Compiler::compile()
{
    // Riddical considers there to be only one instruction on each line.
    // Labels are a special kind of instruction with 0 arguments.
    
    std::vector<std::string> tokens = tokenize(source, '\n');
    if (tokens.empty()) {
	std::cerr << "Error: \n\tCould not tokenize source file " << file_path << "." << std::endl;
	return false;
    }

    // Looking for sections
    // Currently Riddical only considers 2 sections in a program:
    // - The Start label: the entry point
    // - The Data label: variable declaration/initialization
    
    std::array<int, 2> start_idx = find_section(tokens, "Start");
    std::array<int, 2> data_idx = find_section(tokens, "Data");
    
    // Checking for correctness
    if (start_idx[0] < 0 || start_idx[1] < 0) {
	std::cerr << file_path << ": Error: \n\tMalformed Start section" << std::endl;
	return false;
    }

    if (data_idx[0] < 0 || data_idx[1] < 0) {
	std::cerr << file_path << ": Error: \n\tMalformed Data section" << std::endl;
	return false;
    }

    if (overlap(start_idx, data_idx)) {
	std::cerr << file_path << ": Error: \n\tSections overlap" << std::endl;
    }

    // Parsing instructions
    for (int i = start_idx[0]; i <= start_idx[1]; i++) {
	if (tokens[i].substr(0, 2) == ";;") continue;

	std::vector<std::string> instr_vec = tokenize(tokens[i], ' ');
	if (!is_str_alpha(instr_vec[0])) {
	    std::cerr << file_path << ", line " << i << ": Error: \n\tUnexpected token " << instr_vec[0] << std::endl;
	    return false;
	}

	Instr instr;
	instr.name = instr_vec[0];
	if (instr_vec.size() == 1 && instr.name[instr.name.size() - 1] != ':') {
	    std::cerr << file_path << ", line " << i << ": Error: \n\tInstruction with no arguments " << instr_vec[0] << std::endl;
	    return false;
	} else {
	    Data empty = {"NULL", 0, 0};
	    instr.args = {empty, empty, empty, empty};
	}

	for (int i = 1; i < instr_vec.size(); i++) {
	    std::string arg(instr_vec[i]);
	    if (is_number(arg)) {		
		instr.args[i - 1] = Data{"LIT", 0, std::stoi(arg)};
	    } else if (is_ident(arg)) {
		instr.args[i - 1] = Data{arg, 0, 0};
	    } else {
		std::cerr << file_path << ", line " << i << ": Error: \n\tUnexpected token " << arg << std::endl;
		return false;
	    }
	}
	instructions.push_back(instr);
    }

    // Parsing data
    init_mem();
    
    for (int i = data_idx[0]; i <= data_idx[1]; i++) {
	if (tokens[i].substr(0, 2) == ";;") continue;

	std::vector<std::string> data_vec = tokenize(tokens[i], ' ');
	if (!is_ident(data_vec[0])) {
	    std::cerr << file_path << ", line " << i << ": Error: \n\tUnexpected number literal " << data_vec[0] << std::endl;
	    return false;
	} else if (data_vec.size() < 2) {
	    std::cerr << file_path << ", line " << i << ": Error: \n\tIncorrect variable initialization" << std::endl;
	    return false;
	}

	std::string name = data_vec[0];
	uint16_t ptr;
	if (data_vec[1] == "=") {
	    std::string value_str("");
	    for (int i = 2; i < data_vec.size(); i++) {
		value_str += data_vec[i] + " ";
	    }
	    std::vector<int32_t> value = get_value(value_str);
	    ptr = push_const_to_mem(value);
	    if (ptr == 0) {
		std::cerr << "Fatal Error: \n\tNo memory" << std::endl;
		exit(4);
	    }
	} else if (data_vec[1] == "var") {
	    if (data_vec.size() != 3) {
		std::cerr << file_path << ": Error: \n\tIncorrect format of variable initialization \n\t(only 1 number must be provided)" << std::endl;
		return false;
	    }
	    if (!is_number(data_vec[2])) {
		std::cerr << file_path << ": Error: \n\tIncorrect variable initializator (must be a number)" << std::endl;
		return false;
	    }
	    uint16_t value_size = std::stoi(data_vec[2]);
	    ptr = alloc_mem(value_size);
	    if (ptr == 0) {
		std::cerr << "Fatal Error: \n\tNo memory" << std::endl;
		exit(4);
	    }
	} else {
	    std::cerr << file_path << ": Error: \n\tUnexpected token " << data_vec[1] << std::endl;
	    return false;
	}

	Data var = {name, ptr, 0};
	data.push_back(var);
    }
    
    return true;
}

// Running the program
uint16_t Compiler::find_var_ptr(std::string var_name)
{
    for (Data var : data) {
	if (var.name == var_name) {
	    return var.ptr;
	}
    }
    return 0;
}

bool Compiler::bin_op(int opcode, Data arg1, Data arg2)
{
    if (arg2.name == "LIT" || arg2.name == "NULL") {
	std::cerr << file_path << ", in Section Start: Error: \t\nSecond argument of add must be a register or a variable"  << std::endl;
	return false;
    }

    int value = 0;
	    
    if (arg1.name == "LIT") {
	value = arg1.lit_value;
    } else {
	value = MEM.cells[arg1.ptr].value;
    }

    switch (opcode) {
    case 2:
	break;
    case 3:
	value += MEM.cells[arg2.ptr].value;
	break;
    case 4:
	value = MEM.cells[arg2.ptr].value - value;
	break;
    case 5:
	value *= MEM.cells[arg2.ptr].value;
	break;
    case 6:
	value = MEM.cells[arg2.ptr].value / value;
	break;
    }

    if (!update_mem(arg2.ptr, value)) {
	std::cerr << file_path << ": Error: \t\nMemory"  << std::endl;
	return false;
    }

    return true;
}

int32_t Compiler::run()
{
    int idx = 0;
    int32_t exit_value = 0;
    bool exit = false;
    while (idx < instructions.size() && !exit) {
	Instr instr = instructions[idx];
	for (int i = 0; i < instr.args.size(); i++) {
	    Data arg = instr.args[i];
	    if (arg.name != "LIT" && arg.name != "NULL") {
		uint16_t ptr = find_var_ptr(arg.name);
		if (ptr == 0) {
		    std::cerr << file_path << ", in section Start: Error \n\tUnknown variable " << arg.name << std::endl;
		    return -1;
		} else {
		    instr.args[i].ptr = ptr;
		}
	    }
	}
	switch (opcodes[instr.name]) {
	case 0: { // exit
	    exit = true;
	    if (instr.args[0].name == "LIT") {
		exit_value = instr.args[0].lit_value;
	    } else {
		exit_value = MEM.cells[instr.args[0].ptr].value;
	    }
	}
	    break;
	case 1: { // write
	    // Write's second parameter signifies if we're printing a single char
	    // or an entire string.
	    // 1 - string
	    // 0 - char
	    // Using 1 with a number literal will simply print the corresponding char
	    // However if you're holding that number literal in a non-zero terminated string...
	    if (instr.args[1].name != "LIT") {
		std::cerr << file_path << ", in section Start: Error: \t\nSecond argument of write must be a number literal 0 or 1" << std::endl;
		return -1;
	    }

	    switch (instr.args[1].lit_value) {
	    case 0:
		if (instr.args[0].name == "LIT") {
		    printf("%c", instr.args[0].lit_value);
		} else {
		    printf("%c", MEM.cells[instr.args[0].ptr].value);
		}
		break;
	    case 1:
		if (instr.args[0].name == "LIT") {
		    printf("%c", instr.args[0].lit_value);
		} else {
		    uint16_t ptr = instr.args[0].ptr;
		    while (MEM.cells[ptr].value != 0) {
			printf("%c", MEM.cells[ptr].value);
			ptr++;
		    }
		}
		break;
	    default:
		std::cerr << file_path << ", in section Start: Error: \t\nSecond argument of write must be a number literal 0 or 1" << std::endl;
		return -1;
	    }
	}
	    break;
	case 2: // move
	case 3: // add
	case 4: // sub
	case 5: // mul
	case 6: // div
	    if (!bin_op(opcodes[instr.name], instr.args[0], instr.args[1])) {
		return false;
	    }
	    break;
	    
	default:
	    std::cerr << file_path << ", in Section Start: Error: \t\nNot implemented: " << instr.name << std::endl;
	    return -2;
	}
	idx++;
    }
    return exit_value;
}
