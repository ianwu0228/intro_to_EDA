#include "BenchParser.hpp"

#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iostream>

using namespace std;

namespace
{
    // Supported gate set (uppercase)
    static const unordered_set<string> kSupported{
        "AND", "NAND", "OR", "NOR", "NOT", "XOR", "BUFF"};

    inline string trim(string s)
    {
        auto notspace = [](int ch)
        { return !isspace(ch); };
        s.erase(s.begin(), find_if(s.begin(), s.end(), notspace));
        s.erase(find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
        return s;
    }

    inline string to_upper(string s)
    {
        transform(s.begin(), s.end(), s.begin(),
                  [](unsigned char c)
                  { return toupper(c); });
        return s;
    }

    // Remove //... or #... comments
    inline string strip_comment(const string &line)
    {
        size_t p1 = line.find("//");
        size_t p2 = line.find('#');
        size_t p = min(p1 == string::npos ? line.size() : p1,
                       p2 == string::npos ? line.size() : p2);
        return line.substr(0, p);
    }

    // Split by comma, trim tokens, drop empties
    vector<string> split_csv(string s)
    {
        vector<string> out;
        string tok;
        istringstream iss(s);
        while (getline(iss, tok, ','))
        {
            tok = trim(tok);
            if (!tok.empty())
                out.push_back(tok);
        }
        return out;
    }

    // Dedup while preserving order
    vector<string> dedup_preserve(const vector<string> &v)
    {
        vector<string> out;
        unordered_set<string> seen;
        out.reserve(v.size());
        for (const auto &x : v)
        {
            if (!seen.count(x))
            {
                seen.insert(x);
                out.push_back(x);
            }
        }
        return out;
    }
}

Circuit parse_bench(const string &path)
{
    ifstream fin(path);
    if (!fin)
        throw BenchParserError("Cannot open .bench file: " + path);

    regex io_re(R"(^(INPUT|OUTPUT)\s*\(\s*([^)]+?)\s*\)\s*$)", regex::icase);
    regex gate_re(
        R"(^\s*([A-Za-z0-9_ \.\[\]\-]+?)\s*=\s*([A-Za-z]+)\s*\(\s*([^)]+?)\s*\)\s*$)");

    Circuit ckt;
    string raw;
    size_t lineno = 0;

    while (getline(fin, raw))
    {
        ++lineno;
        string line = trim(strip_comment(raw));
        if (line.empty())
            continue;

        smatch m;

        // Try INPUT/OUTPUT
        if (regex_match(line, m, io_re))
        {
            string kind = to_upper(m[1].str());
            string name = trim(m[2].str());
            auto names = split_csv(name);
            if (names.empty())
            {
                throw BenchParserError("Empty name in " + kind + " at line " + to_string(lineno));
            }
            if (kind == "INPUT")
            {
                ckt.inputs.insert(ckt.inputs.end(), names.begin(), names.end());
            }
            else
            {
                ckt.outputs.insert(ckt.outputs.end(), names.begin(), names.end());
            }
            continue;
        }

        // Try gate
        if (regex_match(line, m, gate_re))
        {
            string out = trim(m[1].str());
            string gtype = to_upper(trim(m[2].str()));
            vector<string> ins = split_csv(m[3].str());

            if (!kSupported.count(gtype))
            {
                throw BenchParserError("Unsupported gate '" + gtype + "' at line " + to_string(lineno));
            }
            if ((gtype == "NOT" || gtype == "BUFF") && ins.size() != 1)
            {
                throw BenchParserError(gtype + " must have exactly 1 input at line " + to_string(lineno));
            }
            if (gtype == "XOR" && ins.size() != 2)
            {
                throw BenchParserError("XOR must have exactly 2 inputs at line " + to_string(lineno));
            }
            if ((gtype == "AND" || gtype == "NAND" || gtype == "OR" || gtype == "NOR") && ins.size() < 2)
            {
                throw BenchParserError(gtype + " must have at least 2 inputs at line " + to_string(lineno));
            }

            ckt.gates.push_back(Gate{out, gtype, move(ins)});
            continue;
        }

        throw BenchParserError("Unrecognized .bench line at " + to_string(lineno) + ": " + line);
    }

    ckt.inputs = dedup_preserve(ckt.inputs);
    ckt.outputs = dedup_preserve(ckt.outputs);
    return ckt;
}

using namespace std;

void print_circuit(const Circuit &c)
{
    cout << "Inputs  (" << c.inputs.size() << "): ";
    for (auto &s : c.inputs)
        cout << s << " ";
    cout << "\nOutputs (" << c.outputs.size() << "): ";
    for (auto &s : c.outputs)
        cout << s << " ";
    cout << "\nGates   (" << c.gates.size() << ")\n";
    for (const auto &g : c.gates)
    {
        cout << "  " << g.out << " = " << g.type << "(";
        for (size_t i = 0; i < g.ins.size(); ++i)
        {
            cout << g.ins[i] << (i + 1 < g.ins.size() ? ", " : "");
        }
        cout << ")\n";
    }
}