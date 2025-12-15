#include "floorplanner.h"
using namespace std;

void Floorplanner::solve()
{
    if (spec.problemType == 0)
    {
        category0Opt();
    }
    else
    {
        category1Opt();
    }
}

bool Floorplanner::solveCluster(Cluster *c, float targetWidth, float targetHeight)
{
    // Implement the ILP model to minimize height here
    // Use the solver_ object to add variables, constraints, and set the objective
    // After setting up the model, call solver_.optimize() to solve it
    // Update the positions and rotations of modules based on the solution

    std::vector<Module *> clusterModules = c->getSubModules();
    /*TODO: add ILP formulation to solve this module set*/

    /*
    ILP Usage Example (Gurobi)

    1) Add Variables
    solver_.addVariable("REAL", 0.0, 100.0, GRB_CONTINUOUS);  // REAL belongs to (0, 100)
    solver_.addVariable("REAL", 0.0, 2.0, GRB_INTEGER);  // REAL belongs to [0, 2] ( {0,1,2} )
    solver_.addVariable("BOOL", 0.0, 1.0, GRB_BINARY);        // BOOL belongs to {0, 1}

    2) Set Objective
    // Minimize: X + Y
    solver_.setObjective({{"X", 1.0}, {"Y", 1.0}}, 'M');      // 'M' = minimize (implementation dependent)

    3) Add Constraints
    // Y - X ≤ 100
    solver_.addConstraint("c1", {{"Y", 1.0}, {"X", -1.0}}, '<', 100.0);
    // Supported relations: '<' (≤), '>' (≥), '=' (==)

    4) Check if a Module is a Cluster
    // To determine whether a Module* is actually a Cluster*, use isCluster(...)
    if (isCluster(clusterModules[0])) {
        std::cout << "clusterModules[0] is a cluster" << std::endl;
    }

    5) Set Time Limit
    // If you are concerned that the solver might spend too much time on a single sub-problem,
    // set a time limit (in seconds) before calling optimize().
    double seconds = 600.0;                // e.g., 10 minutes
    solver_.setTimeLimit(seconds);
    solver_.optimize();

    Notes:
    - When the time limit is reached, Gurobi may still return a feasible solution (incumbent).
      Before accessing solution values, check if the solution count (SolCount) > 0.
    */

    const int n = static_cast<int>(clusterModules.size());
    // printf("number of cluster modules: %d\n", n);
    if (n == 0)
        return true;

    // set up the variables
    const double W = max(0.0f, targetWidth);
    const double H = max(0.0f, targetHeight);
    const double M = max(W, H);
    // printf("W: %f, H: %f, M: %f\n", W, H, M);

    // setup time limit (optional)
    solver_.setTimeLimit(600.0);

    // setup the ILP variables
    auto vname_x = [&](int i)
    { return std::string("x_") + std::to_string(i); };
    auto vname_y = [&](int i)
    { return std::string("y_") + std::to_string(i); };
    auto vname_r = [&](int i)
    { return std::string("r_") + std::to_string(i); };
    auto vname_p = [&](int i, int j)
    { return std::string("p_") + std::to_string(i) + "_" + std::to_string(j); };
    auto vname_q = [&](int i, int j)
    { return std::string("q_") + std::to_string(i) + "_" + std::to_string(j); };

    // module vars
    for (int i = 0; i < n; ++i)
    {
        solver_.addVariable(vname_x(i), 0.0, W, GRB_CONTINUOUS);
        solver_.addVariable(vname_y(i), 0.0, H, GRB_CONTINUOUS);
        solver_.addVariable(vname_r(i), 0.0, 1.0, GRB_BINARY);
    }

    // pairwise binaries
    for (int i = 0; i < n; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            solver_.addVariable(vname_p(i, j), 0.0, 1.0, GRB_BINARY);
            solver_.addVariable(vname_q(i, j), 0.0, 1.0, GRB_BINARY);
        }
    }

    solver_.addVariable("Y", 0.0, H, GRB_CONTINUOUS);

    // setup objective
    solver_.setObjective({{"Y", 1.0}}, 'M');

    // setup constraints
    //   x_i + w'i <= W
    //   y_i + h'i <= Y
    for (int i = 0; i < n; ++i)
    {
        const int wi = clusterModules[i]->getOrgWidth();
        const int hi = clusterModules[i]->getOrgHeight();

        // x_i + w'i <= W
        // => x_i + (1-r_i)wi + r_i hi <= W
        // => x_i + wi + r_i*(hi - wi) <= W
        solver_.addConstraint(
            "insideW_" + to_string(i),
            {{vname_x(i), 1.0},
             {vname_r(i), static_cast<double>(hi - wi)}},
            '<',
            W - wi);

        // y_i + h'i <= Y
        // => y_i + (1-r_i)hi + r_i wi <= Y
        // => y_i + hi + r_i*(wi - hi) - Y <= 0
        solver_.addConstraint(
            "insideH_" + std::to_string(i),
            {{vname_y(i), 1.0},
             {vname_r(i), static_cast<double>(wi - hi)},
             {"Y", -1.0}},
            '<',
            -static_cast<double>(hi));

        // Optional cap Y <= H (only if targetHeight is meaningful)
        if (H > 0)
        {
            solver_.addConstraint(
                "Ycap",
                {{"Y", 1.0}},
                '<',
                H);
        }
    }

    // four overlap constraints
    for (int i = 0; i < n; ++i)
    {
        const int wi = clusterModules[i]->getOrgWidth();
        const int hi = clusterModules[i]->getOrgHeight();

        for (int j = i + 1; j < n; ++j)
        {
            const int wj = clusterModules[j]->getOrgWidth();
            const int hj = clusterModules[j]->getOrgHeight();

            const std::string pij = vname_p(i, j);
            const std::string qij = vname_q(i, j);

            // (1) x_i + (wi + (hi - wi) r_i) - x_j - M(p_ij + q_ij) <= 0
            solver_.addConstraint(
                "nolap1_" + std::to_string(i) + "_" + std::to_string(j),
                {{vname_x(i), 1.0},
                 {vname_r(i), static_cast<double>(hi - wi)},
                 {vname_x(j), -1.0},
                 {pij, -M},
                 {qij, -M}},
                '<',
                -static_cast<double>(wi));

            // (2) y_i + (hi + (wi - hi) r_i) - y_j - M(1 + p_ij - q_ij) <= 0
            solver_.addConstraint(
                "nolap2_" + std::to_string(i) + "_" + std::to_string(j),
                {{vname_y(i), 1.0},
                 {vname_r(i), static_cast<double>(wi - hi)},
                 {vname_y(j), -1.0},
                 {pij, -M},
                 {qij, M}},
                '<',
                -static_cast<double>(hi) + (M) // accounts for the "+ M*1" on RHS
            );

            // (3) x_j + w'j - x_i - M(1 - p_ij + q_ij) <= 0   (equiv to x_i >= x_j + w'j - M(...))
            // -> x_j + wj + (hj - wj) r_j - x_i - M + M p_ij - M q_ij <= 0
            solver_.addConstraint(
                "nolap3_" + std::to_string(i) + "_" + std::to_string(j),
                {
                    {vname_x(i), 1.0},
                    {vname_x(j), -1.0},
                    {vname_r(j), static_cast<double>(wj - hj)},
                    {vname_p(i, j), -M},
                    {vname_q(i, j), M},
                },
                '>',
                -static_cast<double>(M) + static_cast<double>(wj));

            // (4) y_j + h'j - y_i - M(2 - p_ij - q_ij) <= 0   (equiv to y_i >= y_j + h'j - M(...))
            // -> y_j + hj + (wj - hj) r_j - y_i - 2M + M p_ij + M q_ij <= 0
            solver_.addConstraint(
                "nolap4_" + std::to_string(i) + "_" + std::to_string(j),
                {{vname_y(i), 1.0},
                 {vname_y(j), -1.0},
                 {vname_r(j), static_cast<double>(hj - wj)},
                 {vname_p(i, j), -M},
                 {vname_q(i, j), -M}},
                '>',
                -static_cast<double>(2 * M) + static_cast<double>(hj)

            );
        }
    }

    // After all constraints and the objective are set, solve the model.
    // Do NOT remove the following lines.

    solver_.optimize();
    if (solver_.getStatus() == GRB_INFEASIBLE)
    {
        std::cout << "ILP unsat!" << std::endl;
        solver_.reset();
        return false;
    }
    const int st = solver_.getStatus();
    if (st == GRB_TIME_LIMIT || st == GRB_INTERRUPTED)
    {
        if (solver_.getSolutionCount() == 0)
        {
            std::cout << "Time limit reached with NO feasible solution.\n";
            solver_.reset();
            return false;
        }
    }

    /* TODO: Use solver_.getVariableValue(<name>) to fetch solution values
       and write them back to the corresponding modules/clusters.
       - Example: double x = solver_.getVariableValue("X");
    */
    for (int i = 0; i < n; ++i)
    {
        double xi = solver_.getVariableValue(vname_x(i));
        double yi = solver_.getVariableValue(vname_y(i));
        double ri = solver_.getVariableValue(vname_r(i));
        clusterModules[i]->setPosition(Point(xi, yi));
        clusterModules[i]->setRotate(ri > 0.5);
    }

    solver_.reset(); // DO NOT delete or comment out this line
    return true;
}

float Floorplanner::category0Opt()
{
    // You don't need to modify this function
    // Do NOT remove or reorder the following three lines unless you understand the workflow.
    // It shows how to wrap all modules into a top-level Cluster, and solve it.
    clusters.clear();
    clusters.push_back(std::make_unique<Cluster>(modules));
    float finalHeight = solveCluster(clusters[0].get(), spec.targetWidth, spec.targetHeight);

    // you may try to uncomment the following 4 functions to verify if your cluster level
    // rotate() works
    //  printf("before rotate\n");
    //  clusters[0]->printCluster();
    //  printf("test rotate\n");
    //  clusters[0]->setPosition(Point(2025,1015));
    //  clusters[0]->rotate();
    //  clusters[0]->setPosition(Point(10,2));
    //  clusters[0]->rotate();
    //  printf("after rotate\n");
    //  clusters[0]->printCluster();

    return finalHeight;
}

float Floorplanner::category1Opt()
{

    // TODO: Implement your own heuristic to solve this problem.

    return 0.0;
}