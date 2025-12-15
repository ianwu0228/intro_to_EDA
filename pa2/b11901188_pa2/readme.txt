============================================================
README — Floorplanning PA2
============================================================

This project implements the PA2 fixed-outline floorplanner with two
solvers:

Category 0: ILP-based optimal floorplanner  
Category 1: B*-tree + simulated annealing floorplanner  

Three executables are generated:

    bin/fp              → Main entry point for PA2
    bin/b_fp            → B*-tree floorplanner (Category 1)
    bin/format_exchange → Converts .in + .spec to .block + .nets

============================================================
1. Environment Requirements
============================================================

• Linux environment (tested on Ubuntu/WSL)  
• g++ with C++17 support  
• Gurobi 12.0.3  
• Make  

Directory structure must remain:

    include/
    src/
    bin/
    gurobi1203/linux64/

============================================================
2. How to Compile 
============================================================

All commands **must be executed inside the project root directory**, i.e.,  
the directory containing:

    include/, src/, bin/, makefile

To compile, run:

    $ cd <project_root>
    $ make

This will:
• Build bin/fp  
• Build bin/b_fp  
• Build bin/format_exchange  

To clean:

    $ make clean

============================================================
3. How to Run 
============================================================

The main executable is:

    bin/fp

Usage:

    $ ./bin/fp <input_file> <spec_file> <output_file>

------------------------------------------------------------
Category 0 (ILP)
------------------------------------------------------------

If the spec indicates:

    problem_type = 0

then:

    $ ./bin/fp <infile> <specfile> <outfile>

The ILP solver:
• Parses modules and outline  
• Solves optimally using Gurobi  
• Writes results to <outfile>

------------------------------------------------------------
Category 1 (B*-tree)
------------------------------------------------------------

If the spec indicates:

    problem_type = 1

`fp` automatically performs:

1. Format conversion:
       ./bin/format_exchange <input> <spec> tmp.block tmp.nets

2. B*-tree placement:
       ./bin/b_fp 1 tmp.block tmp.nets <output>

You do NOT need to run these manually.

Example:

    $ ./bin/fp testcase20.in testcase20.spec output20.out

------------------------------------------------------------
Manual Execution (Optional)
------------------------------------------------------------

For debugging:

    $ ./bin/format_exchange in.in in.spec out.block out.nets
    $ ./bin/b_fp 1 out.block out.nets result.out

============================================================
4. Output Format
============================================================

The output file includes:
• Each module’s (x, y) coordinate and rotation  


