#pragma once
#include "CNF.hpp"
#include "BenchParser.hpp"
#include <unordered_set>
#include <algorithm>
#include <stdexcept>
using namespace std;

// --- Gate encoders (Tseitin) ---
// z = NOT x
inline void enc_NOT(CNF &cnf, int z, int x)
{
    cnf.add_clause({-z, -x});
    cnf.add_clause({z, x});
}
// z = BUFF x
inline void enc_BUFF(CNF &cnf, int z, int x)
{
    cnf.add_clause({-z, x});
    cnf.add_clause({z, -x});
}
// z = AND(x1..xn)
inline void enc_AND(CNF &cnf, int z, const vector<int> &xs)
{
    // z -> xi  === (¬z ∨ xi) for all i
    for (int x : xs)
        cnf.add_clause({-z, x});

    // (x1 ∧ ... ∧ xn) -> z  === (¬x1 ∨ ¬x2 ∨ ... ∨ z)
    vector<int> big;
    big.reserve(xs.size() + 1);
    for (int x : xs)
        big.push_back(-x);
    big.push_back(z);
    cnf.add_clause(big);
}

// z = OR(x1..xn)
inline void enc_OR(CNF &cnf, int z, const vector<int> &xs)
{
    for (int x : xs)
        cnf.add_clause({-x, z});
    vector<int> big = xs;
    big.push_back(-z);
    cnf.add_clause(big);
}
// z = XOR(a,b)
inline void enc_XOR2(CNF &cnf, int z, int a, int b)
{
    cnf.add_clause({-a, -b, -z});
    cnf.add_clause({a, b, -z});
    cnf.add_clause({a, -b, z});
    cnf.add_clause({-a, b, z});
}

inline void enc_NAND(CNF &cnf, int z, const vector<int> &xs)
{
    if (xs.empty())
        throw runtime_error("NAND of 0 inputs is undefined");
    if (xs.size() == 1)
    {
        enc_NOT(cnf, z, xs[0]);
        return;
    } // NAND(x) = NOT(x)

    // (xi ∨ z)  for each input
    for (int x : xs)
        cnf.add_clause({x, z});

    // (¬x1 ∨ ¬x2 ∨ ... ∨ ¬xn ∨ ¬z)
    vector<int> big;
    big.reserve(xs.size() + 1);
    for (int x : xs)
        big.push_back(-x);
    big.push_back(-z);
    cnf.add_clause(big);
}

inline void enc_NOR(CNF &cnf, int z, const vector<int> &xs)
{
    if (xs.empty())
        throw runtime_error("NOR of 0 inputs is undefined");
    if (xs.size() == 1)
    {
        enc_NOT(cnf, z, xs[0]);
        return;
    } // NOR(x) = NOT(x)

    // (¬xi ∨ ¬z)  for each input
    for (int x : xs)
        cnf.add_clause({-x, -z});

    // (x1 ∨ x2 ∨ ... ∨ xn ∨ z)
    vector<int> big(xs.begin(), xs.end());
    big.push_back(z);
    cnf.add_clause(big);
}

// Encode one parsed circuit. Returns PO vars in the same order as ckt.outputs.
inline vector<int> encode_circuit_to_cnf(CNF &cnf, PinTable &pt, const Circuit &ckt, const string &prefix)
{
    unordered_set<string> pi(ckt.inputs.begin(), ckt.inputs.end());
    auto pin2var = [&](const string &s) -> int
    {
        if (pi.count(s))
            return pt.get_or_create_pi(cnf, s);
        return pt.get_or_create_net(cnf, prefix + "/" + s);
    };

    vector<int> outs;
    outs.reserve(ckt.outputs.size());
    for (const auto &o : ckt.outputs)
        outs.push_back(pt.get_or_create_net(cnf, prefix + "/" + o));

    for (const auto &g : ckt.gates)
    {
        int z = pt.get_or_create_net(cnf, prefix + "/" + g.out);
        vector<int> xs;
        xs.reserve(g.ins.size());
        for (auto &s : g.ins)
            xs.push_back(pin2var(s));
        if (g.type == "NOT")
            enc_NOT(cnf, z, xs[0]);
        else if (g.type == "BUFF")
            enc_BUFF(cnf, z, xs[0]);
        else if (g.type == "AND")
            enc_AND(cnf, z, xs);
        else if (g.type == "OR")
            enc_OR(cnf, z, xs);
        else if (g.type == "XOR")
            enc_XOR2(cnf, z, xs[0], xs[1]);
        else if (g.type == "NAND")
            enc_NAND(cnf, z, xs);
        else if (g.type == "NOR")
            enc_NOR(cnf, z, xs);
        else
            throw runtime_error("Unsupported gate in encoder: " + g.type);
    }
    return outs;
}

// --- Miter helpers ---
// for XORing the same outputs of two circuits
inline int mk_xor(CNF &cnf, int a, int b)
{
    int z = cnf.new_var();
    enc_XOR2(cnf, z, a, b);
    return z;
}

// for ORing the XORed outputs of the Miter circuit
inline int mk_or_many(CNF &cnf, const vector<int> &xs)
{
    if (xs.empty())
        throw runtime_error("mk_or_many: empty");
    if (xs.size() == 1)
        return xs[0];
    int z = cnf.new_var();
    enc_OR(cnf, z, xs);
    return z;
}
