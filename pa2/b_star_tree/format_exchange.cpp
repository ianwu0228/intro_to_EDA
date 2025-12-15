// convert_pa_to_bstar.cpp
// Usage: ./format_exchange 100.spec 100.in 100.block 100.nets
// If you omit args, defaults are: 100.spec 100.in 100.block 100.nets

#include <bits/stdc++.h>
using namespace std;

static bool parseSpec(const string& specPath, long long &outW, long long &outH) {
    ifstream fin(specPath);
    if (!fin) {
        cerr << "Error: cannot open spec file: " << specPath << "\n";
        return false;
    }
    // Try to find two integers on a line that likely represent outline W H.
    // Accept lines like: "OUTLINE  2000 1500" or "Outline: 2000 1500" or just "2000 1500".
    string line;
    while (getline(fin, line)) {
        // Remove comments (if any)
        auto posHash = line.find('#');
        if (posHash != string::npos) line = line.substr(0, posHash);

        // Extract all integers in the line
        stringstream ss(line);
        vector<long long> nums;
        string tok;
        while (ss >> tok) {
            // keep only ints
            bool ok = true;
            int sign = 1;
            size_t i = 0;
            if (tok.size() && (tok[0] == '+' || tok[0] == '-')) { sign = (tok[0] == '-') ? -1 : 1; i = 1; }
            if (i >= tok.size()) ok = false;
            for (; i < tok.size() && ok; ++i) ok = isdigit(static_cast<unsigned char>(tok[i]));
            if (ok) {
                long long v = stoll(tok);
                nums.push_back(v);
            }
        }
        if (nums.size() >= 2) {
            outW = nums[0];
            outH = nums[1];
            return true;
        }
    }
    cerr << "Error: failed to find outline W H in spec file.\n";
    return false;
}

struct BlockRec {
    int id;
    long long w, h;
};

static bool parseIn(const string& inPath, vector<BlockRec>& blocks) {
    ifstream fin(inPath);
    if (!fin) {
        cerr << "Error: cannot open in file: " << inPath << "\n";
        return false;
    }

    // Expect something like:
    // MODULE_SIZE 100
    // ID   W   H
    // 0    16  70
    // 1    91  3
    // ...
    int N = -1;
    string line;

    // Find MODULE_SIZE
    bool foundSize = false;
    while (getline(fin, line)) {
        if (line.find("MODULE_SIZE") != string::npos) {
            stringstream ss(line);
            string kw; ss >> kw; // MODULE_SIZE
            if (!(ss >> N) || N <= 0) {
                // Maybe the number is not on the same line, try next tokens
                // (Fallback: scan more tokens right after)
                string rest;
                if (ss >> rest) {
                    try {
                        N = stoi(rest);
                    } catch (...) { }
                }
            }
            if (N > 0) { foundSize = true; break; }
        }
    }
    if (!foundSize || N <= 0) {
        cerr << "Error: cannot parse MODULE_SIZE from " << inPath << "\n";
        return false;
    }

    // Skip possible header line (e.g., "ID  W  H")
    streampos afterSizePos = fin.tellg();
    if (getline(fin, line)) {
        // if the next line is not data, it's a header; otherwise rewind one line
        // we’ll check if it contains three ints; if not, assume header
        stringstream ss(line);
        long long a,b,c;
        if (!(ss >> a >> b >> c)) {
            // header; continue reading data from subsequent lines
        } else {
            // it's data; push and continue with remaining N-1 lines
            blocks.push_back(BlockRec{(int)a, b, c});
        }
    }

    // Read until we have N blocks
    while ((int)blocks.size() < N && getline(fin, line)) {
        if (line.empty()) continue;
        // filter comments
        auto posHash = line.find('#');
        if (posHash != string::npos) line = line.substr(0, posHash);
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;

        stringstream ss(line);
        int id; long long w, h;
        if (ss >> id >> w >> h) {
            blocks.push_back(BlockRec{id, w, h});
        }
    }

    if ((int)blocks.size() != N) {
        cerr << "Warning: expected " << N << " modules, parsed " << blocks.size() << ".\n";
        return false;
    }

    // Sort by id to have deterministic order (optional)
    sort(blocks.begin(), blocks.end(), [](const BlockRec& a, const BlockRec& b){
        return a.id < b.id;
    });

    return true;
}

static bool writeBlock(const string& outPath,
                       long long outlineW, long long outlineH,
                       const vector<BlockRec>& blocks) {
    ofstream fout(outPath);
    if (!fout) {
        cerr << "Error: cannot open block output file: " << outPath << "\n";
        return false;
    }
    // B*-tree friendly format (common variant):
    // Outline : W H
    // NumBlocks : N
    // NumTerminals : 0
    // b0  w  h
    // b1  w  h
    // ...
    fout << "Outline: " << outlineW << " " << outlineH << "\n";
    fout << "NumBlocks: " << blocks.size() << "\n";
    fout << "NumTerminals: 0\n\n";
    for (auto &b : blocks) {
        // Name blocks as b<id>, dimensions w h
        fout << b.id << " " << b.w << " " << b.h << "\n";
    }
    return true;
}

static bool writeNets(const string& outPath) {
    ofstream fout(outPath);
    if (!fout) {
        cerr << "Error: cannot open nets output file: " << outPath << "\n";
        return false;
    }
    // No nets/terminals in this PA:
    // NumNets : 0
    // NumPins : 0
    fout << "NumNets: 0\n";
    fout << "NumPins: 0\n";
    return true;
}

int main(int argc, char** argv) {
    string specPath  = (argc > 1 ? argv[1] : "100.spec");
    string inPath    = (argc > 2 ? argv[2] : "100.in");
    string blockPath = (argc > 3 ? argv[3] : "100.block");
    string netsPath  = (argc > 4 ? argv[4] : "100.nets");

    long long W = -1, H = -1;
    vector<BlockRec> blocks;

    if (!parseSpec(specPath, W, H)) return 1;
    if (!parseIn(inPath, blocks)) return 1;
    if (!writeBlock(blockPath, W, H, blocks)) return 1;
    if (!writeNets(netsPath)) return 1;

    cerr << "Wrote: " << blockPath << " (Outline " << W << "x" << H
         << ", NumBlocks " << blocks.size() << ", NumTerminals 0)\n";
    cerr << "Wrote: " << netsPath  << " (NumNets 0, NumPins 0)\n";
    return 0;
}
