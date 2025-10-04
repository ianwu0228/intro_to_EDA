#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <initializer_list>
#include <ostream>
using namespace std;

struct CNF
{
    int var_cnt = 0;
    vector<vector<int>> clauses;

    int new_var() { return ++var_cnt; }
    void add_clause(initializer_list<int> lits) { clauses.emplace_back(lits); }
    void add_clause(const vector<int> &lits) { clauses.push_back(lits); }

    void write_dimacs(ostream &os) const
    {
        os << "p cnf " << var_cnt << " " << clauses.size() << "\n";
        for (const auto &cl : clauses)
        {
            for (int lit : cl)
                os << lit << " ";
            os << "0\n";
        }
    }
};

struct PinTable
{
    unordered_map<string, int> pi_to_var;  // PIs shared across circuits by name
    unordered_map<string, int> net_to_var; // internal nets: prefix/name

    int get_or_create_pi(CNF &cnf, const string &name)
    {
        auto it = pi_to_var.find(name);
        if (it != pi_to_var.end())
            return it->second;
        int v = cnf.new_var();
        pi_to_var[name] = v;
        return v;
    }
    int get_or_create_net(CNF &cnf, const string &scoped_name)
    {
        auto it = net_to_var.find(scoped_name);
        if (it != net_to_var.end())
            return it->second;
        int v = cnf.new_var();
        net_to_var[scoped_name] = v;
        return v;
    }
};
