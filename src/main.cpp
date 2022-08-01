#include "pattern.hpp"
#include "pattern_graph.hpp"
#include "eval.hpp"
#include "values.hpp"
#include <cstdio>
#include "parser.hpp"
#include "t-run.hpp"

void tests()
{
    tests_parser();
    tests_pattern();
    tests_pattern_graph();
    tests_values();
    tests_eval();
}

int main() {
    tests_run();
    return 0;
}
