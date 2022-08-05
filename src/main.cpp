#include "ast.hpp"
#include "pattern.hpp"
#include "pattern_graph.hpp"
#include "eval.hpp"
#include "pprint.hpp"
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
        auto printer = ast::ast_printer( pprint::PrettyPrinter( std::cout ) );
        auto expr = p.p_expression();
        
        TRACE( ast::ast_free_vars::free_variables( expr ) );

        // printer.accept( expr );
        e.push( std::move( expr ) );
        e.run();
        TRACE( e.state._values ); 
    } catch( parsing_error &e ) {
        TRACE( p.stack_trace );
        std::cerr << e.what() << std::endl;
    } catch( std::runtime_error e ) {
        std::cerr << e.what() << std::endl;
    }


}
