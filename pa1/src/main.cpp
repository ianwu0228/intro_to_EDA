#include "BenchParser.hpp"
#include "CNF.hpp"
#include "Tseitin.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>
using namespace std;

static bool outputs_align_by_index = true; // flip to false to align by name

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        cerr << "Usage: ./ec <A.bench> <B.bench> <out.dimacs>\n";
        return 1;
    }
    try
    {
        Circuit A = parse_bench(argv[1]);
        Circuit B = parse_bench(argv[2]);

        if (outputs_align_by_index && A.outputs.size() != B.outputs.size())
        {
            cerr << "Error: different #outputs.\n";
            return 2;
        }

        CNF cnf;
        PinTable pt;

        // Encode both circuits; PIs are shared via PinTable
        vector<int> Aout = encode_circuit_to_cnf(cnf, pt, A, "A");
        vector<int> Bout = encode_circuit_to_cnf(cnf, pt, B, "B");

        // Optional: align by name
        if (!outputs_align_by_index)
        {
            unordered_map<string, int> mapB;
            for (size_t i = 0; i < B.outputs.size(); ++i)
                mapB[B.outputs[i]] = Bout[i];
            vector<int> Bout2;
            Bout2.reserve(A.outputs.size());
            for (auto &name : A.outputs)
            {
                if (!mapB.count(name))
                {
                    cerr << "Missing PO: " << name << "\n";
                    return 3;
                }
                Bout2.push_back(mapB[name]);
            }
            Bout.swap(Bout2);
        }

        // Build miter: di = XOR(Aout[i], Bout[i]); diff = OR(di...); assert diff
        vector<int> diffs;
        diffs.reserve(Aout.size());
        for (size_t i = 0; i < Aout.size(); ++i)
            diffs.push_back(mk_xor(cnf, Aout[i], Bout[i]));
        int diff = mk_or_many(cnf, diffs);
        cnf.add_clause({diff}); // force difference = true

        ofstream ofs(argv[3]);
        if (!ofs)
        {
            cerr << "Cannot open output file.\n";
            return 4;
        }
        cnf.write_dimacs(ofs);
        cout << "Wrote DIMACS to " << argv[3] << "\n";
        cout << "UNSAT => equivalent; SAT => not equivalent.\n";
    }
    catch (const BenchParserError &e)
    {
        cerr << "Parse error: " << e.what() << "\n";
        return 11;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << "\n";
        return 12;
    }
    return 0;
}
