#pragma once 
#include <stdint.h>
#include <string>
#include <array>
#include <vector>
#include <map>

struct MemCell
{
    bool occupied;
    int32_t value;
};

struct Memory
{
    // 2^16 - 1
    std::array<MemCell, 65535> cells;
    // 16 standard registers + 4 system registers 
    std::array<int32_t, 16> regs;
    std::array<int32_t, 4> sys_regs;
};

struct Data
{
    std::string name;
    uint16_t ptr;
    int32_t lit_value;
};

struct Instr
{
    std::string name;
    std::array<Data, 4> args;
};

class Compiler
{
public:
    std::string file_path;
    std::string source;

    std::vector<Instr> instructions;
    std::vector<Data> data;

    Memory MEM;

    std::map<std::string, int> opcodes;

    Compiler(std::string file) {
	file_path = file;
	MEM = {0, 0};

	int iota = 0;
	opcodes = {
	    {"exit", iota++},  // 0
	    {"write", iota++}, // 1
	    {"move", iota++},  // 2
	    {"add", iota++},   // 3
	    {"sub", iota++},   // 4
	    {"mul", iota++},   // 5
	    {"div", iota++},   // 6
	};
	
    };
    bool read_file();
    bool compile();
    std::vector<std::string> tokenize(std::string src, char delim);
    std::array<int, 2> find_section(std::vector<std::string> tokens, const char* section);
    std::vector<int32_t> get_value(std::string value);
    uint16_t push_const_to_mem(std::vector<int32_t> val);
    uint16_t alloc_mem(uint16_t val_size);
    bool update_mem(uint16_t ptr, int32_t val);
    void init_mem();
    void print_mem(int idx);
    uint16_t find_var_ptr(std::string var_name);
    bool bin_op(int opcode, Data arg1, Data arg2);
    int32_t run();
};
