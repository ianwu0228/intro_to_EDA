#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <initializer_list>
#include <ostream>
#include <iostream>
#include <iomanip>
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

// map the given pin names to a special id(number)

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

void print_PinTable(PinTable &pt)
{
    cout << "================ PinTable ================\n";

    cout << "[Primary Inputs]\n";
    if (pt.pi_to_var.empty())
    {
        cout << "  (none)\n";
    }
    else
    {
        for (const auto &p : pt.pi_to_var)
            cout << "  " << setw(20) << left << p.first
                 << " -> var " << p.second << "\n";
    }

    cout << "\n[Internal Nets]\n";
    if (pt.net_to_var.empty())
    {
        cout << "  (none)\n";
    }
    else
    {
        for (const auto &n : pt.net_to_var)
            cout << "  " << setw(20) << left << n.first
                 << " -> var " << n.second << "\n";
    }

    cout << "=========================================\n";
}