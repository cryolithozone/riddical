#include "compiler.h"
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2) {
	std::cout << "Usage: ./riddical <program>" << std::endl;
	return 1;
    }
    std::string file_path(argv[1]);

    Compiler compiler(file_path);

    if (!compiler.read_file()) return 2;

    if (!compiler.compile()) return 3;

    int32_t exit_value = compiler.run();
    
    return exit_value;
}
