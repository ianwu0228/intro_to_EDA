# Programming Assignment #1: Equivalence Checking

**Course:** Introduction to EDA, Fall 2025  
**Department:** Electrical Engineering, National Taiwan University

This project implements an **equivalence checker** for two combinational circuits described in `.bench` format.  
The program parses both circuits, constructs a **miter circuit**, encodes it into **CNF (DIMACS format)** using **Tseitin transformation**, and outputs a `.dimacs` file for verification by **MiniSat**.

---

## Directory Structure

```
<StudentID_pa1>/
├── bin/                # compiled binary (e.g., ec)
├── src/                # source files
│   ├── main.cpp     # main program for equivalence checking
│   ├── BenchParser.cpp
│   ├── BenchParser.hpp
│   └── (other .cpp/.hpp)
├── Makefile
└── README.md
```

---

## Compilation Instructions

### Requirements

- **C++17 or newer**
- **GNU Make**
- **g++** compiler (tested with g++ 11+)
- **MiniSat** (for SAT solving, optional for verification)

### To compile:

Run the following command in the project root directory:

```bash
make
```

This will:

- Compile all source files under `src/`
- Generate an executable named **`ec`** under the `bin/` directory

If successful, you should see:

```
g++ -std=gnu++17 -O3 -Wall -Wextra -Wpedantic -Isrc src/*.cpp -o bin/ec
```

---

## Execution Instructions

Run your program with:

```bash
./bin/ec [Circuit_A.bench] [Circuit_B.bench] [Output.dimacs]
```

### Example:

```bash
./bin/ec example_A.bench example_B.bench output.dimacs
```

- **Input:**  
  Two `.bench` files describing the combinational circuits.  
  Each contains:

  - A list of primary inputs: `INPUT(...)`
  - A list of primary outputs: `OUTPUT(...)`
  - Gate definitions: `out = GATE(in1, in2, ...)`

- **Output:**  
  A `.dimacs` file representing the CNF clauses for the miter circuit.

---

## Verification using MiniSat

To check equivalence:

```bash
./MiniSat_v1.14_linux output.dimacs result.output
```

- **SATISFIABLE** → Circuits **NOT equivalent** (there exists a counterexample input)
- **UNSATISFIABLE** → Circuits **equivalent**

---

## Example

**Input:**

```
./bin/ec example_A.bench example_B.bench example.dimacs
```

**Output (`example.dimacs`):**

```
p cnf 12 27
1 -5 0
7 -5 0
5 -1 -7 0
...
```

**MiniSat result:**

```
SATISFIABLE
```

⟹ Circuits are not equivalent.

---

## Notes

- Supported gates: `AND`, `NAND`, `OR`, `NOR`, `NOT`, `XOR`, `BUFF`
- `AND`, `OR`, `NAND`, and `NOR` may have multiple inputs.
- Pin names are not restricted to numeric IDs.

---

**Author:** Yi-En Wu  
**Student ID:** B11901188  
**Date:** October 2025
