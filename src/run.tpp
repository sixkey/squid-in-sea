#include "parser.hpp"
#include "eval.hpp"
#include "pattern.hpp"
#include "values.hpp"
#include "builtins.hpp"

void test_run()
{
    using eval_t = eval;
    using builtin_t = std::function< void( eval_t& ) >;
    using object_t = eval_t::object_t;

    parser_str p( { "( fun |- < Int a >   < Int b >  -> a + b "
                    "      |- < Bool a >  < Bool b > -> a && b ) true false" }, 10 );

    p.op_table.insert( { "+"s, { 6, false } } );
    p.op_table.insert( { "-"s, { 6, false } } );
    p.op_table.insert( { "*"s, { 7, false } } );
    p.op_table.insert( { "&&"s, { 2, false } } );

    eval_t e;

    e.state._store.scopes.add_scope();
    builtins< eval_t >::add_builtins( e );
    e.state._store.scopes.add_scope();
    
    e.push( p.p_expression() );

    e.run();

    assert( e.state._values.top() == object_t( false ) );
}

int main()
{
    test_run();
}
