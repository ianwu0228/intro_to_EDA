#include "BenchParser.hpp"
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        cerr << "Usage: parse_demo <path/to/circuit.bench>\n";
        return 1;
    }
    try
    {
        Circuit c = parse_bench(argv[1]);
        print_circuit(c);
        }
    catch (const BenchParserError &e)
    {
        cerr << "Parse error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}
