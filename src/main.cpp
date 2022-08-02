#include "pattern.hpp"
#include "pattern_graph.hpp"
#include "eval.hpp"
#include "values.hpp"
#include <cstdio>
#include "parser.hpp"
#include "t-run.hpp"

void tests()
{
    tests_pattern();
    tests_pattern_graph();
    tests_values();
    tests_parser();
    tests_eval();
    tests_run();
}

int main() {
    return 0;
}
