#include "pattern.hpp"
#include "pattern_graph.hpp"
#include "eval.hpp"
#include "values.hpp"
#include <cstdio>
#include <fstream>
#include <ios>
#include <stdexcept>
#include "parser.hpp"
#include "t-run.hpp"
#include "builtins.hpp"

void tests()
{
    tests_pattern();
    tests_pattern_graph();
    tests_values();
    tests_parser();
    tests_eval();
    tests_run();
}

int main( int argc, char** argv ) 
{
    if ( argc == 0 ) {
        tests();
        return 0;
    }
    std::ifstream file( argv[ 1 ] );
    parser< istream_generator< std::ifstream > > p( { std::move( file ) }, 10 );

    p.op_table.insert( { "+",   { 6,    false } } );
    p.op_table.insert( { "-",   { 6,    false } } );
    p.op_table.insert( { "*",   { 7,    false } } );
    p.op_table.insert( { "&&",  { 2,    false } } );

    eval e;

    e.state._store.scopes.add_scope();
    builtins< eval >::add_builtins( e );
    e.state._store.scopes.add_scope();
    
    try {
        e.push( p.p_expression() );
        e.run();
        ast::ast_node node = p.p_expression();
        TRACE( node );
    } catch( parsing_error &e ) {
        std::cerr << e.what() << std::endl;
    } catch( std::runtime_error e ) {
        std::cerr << e.what() << std::endl;
    }


}
