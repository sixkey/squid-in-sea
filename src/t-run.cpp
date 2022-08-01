#include "parser.hpp"
#include "eval.hpp"
#include "pattern.hpp"
#include "values.hpp"

template < typename eval_t >
struct builtins 
{
    using object_t = typename eval_t::object_t;
    using builtin_t = typename eval_t::types::builtin_t;
    using builtin_wrapper = std::function< object_t( builtin_t ) >;
    using evaluable_t = typename eval_t::types::evaluable_t;

    static object_t int_binary_o( evaluable_t e ) {
        using object_t = typename eval_t::object_t;
        function_path< evaluable_t > path( 
                { object_pattern( "Int", { variable_pattern( "a" ) } )
                , object_pattern( "Int", { variable_pattern( "b" ) } ) }, 
                variable_pattern( "_" )
                , e );
        return object_t( function_object< evaluable_t >( { std::move( path ) }, 2 ) );
    };

    static void int_binary( eval_t& e, std::function< int( int, int ) > f )
    {
        object_t a = e.state._store.lookup( "a" );
        object_t b = e.state._store.lookup( "b" );
        assert( a.omega() );
        assert( b.omega() );
        e.state.push_value( object_t( f( a.template get_value< int >()
                                       , b.template get_value< int >() ) ) );
    }

    static void add( eval_t& e ) { int_binary( e, []( int a, int b ){ return a + b; } ); }
    static void sub( eval_t& e ) { int_binary( e, []( int a, int b ){ return a - b; } ); }
    static void mul( eval_t& e ) { int_binary( e, []( int a, int b ){ return a * b; } ); }
    static void div( eval_t& e ) { int_binary( e, []( int a, int b ){ return a / b; } ); }
    static void mod( eval_t& e ) { int_binary( e, []( int a, int b ){ return a % b; } ); }

    static const inline std::map< std::string, std::pair< builtin_t, builtin_wrapper > > bindings {
        { "__int_add__", { add, int_binary_o } },
        { "__int_sub__", { sub, int_binary_o } },
        { "__int_mul__", { mul, int_binary_o } },
        { "__int_div__", { div, int_binary_o } },
        { "__int_mod__", { mod, int_binary_o } }
    };

    static void add_builtins( eval_t& e )
    {
        for ( const auto& [ k, v ] : bindings ) {
            e.state._store.bind( k, v.second( v.first ) );
        }
    }
};

void test_run()
{
    using eval_t = eval;
    using builtin_t = std::function< void( eval_t& ) >;

    parser p( "__int_add__ 3 4", 10 );

    p.op_table.insert( { "+"s, { 6, false } } );

    eval_t e;

    e.state._store.scopes.add_scope();
    builtins< eval_t >::add_builtins( e );
    e.state._store.scopes.add_scope();
    
    e.push( p.p_expression() );

    e.run();

    TRACE( e.state._values );
}

void tests_run()
{
    test_run();
}
