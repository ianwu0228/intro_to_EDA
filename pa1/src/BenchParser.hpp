#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_set>

using namespace std;

struct Gate
{
    string out;
    string type;        // AND, NAND, OR, NOR, NOT, XOR, BUFF
    vector<string> ins; // input pin names (order preserved)
};

struct Circuit
{
    vector<string> inputs;  // deduped, order-preserving
    vector<string> outputs; // deduped, order-preserving
    vector<Gate> gates;
};

class BenchParserError : public runtime_error
{
public:
    using runtime_error::runtime_error;
};

Circuit parse_bench(const string &path);
void print_circuit(const Circuit &c);