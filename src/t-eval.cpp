#include "ast.hpp"
#include "eval.hpp"
#include "pprint.hpp"
#include "values.hpp"


void tests_eval()
{
    eval< object > e;

    ast::literal< int > a( 3 );

    e.eval_a( a );

    pprint::PrettyPrinter printer;

    printer.print( e.state._values );
}

