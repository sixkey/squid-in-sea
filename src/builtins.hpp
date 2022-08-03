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
        e.state.push_value( object_t( f( a.template get_value< int >()
                                       , b.template get_value< int >() ) ) );
    }

    static void add( eval_t& e ) { int_binary( e, []( int a, int b ){ return a + b; } ); }
    static void sub( eval_t& e ) { int_binary( e, []( int a, int b ){ return a - b; } ); }
    static void mul( eval_t& e ) { int_binary( e, []( int a, int b ){ return a * b; } ); }
    static void div( eval_t& e ) { int_binary( e, []( int a, int b ){ return a / b; } ); }
    static void mod( eval_t& e ) { int_binary( e, []( int a, int b ){ return a % b; } ); }

    static object_t bool_binary_o( evaluable_t e ) {
        using object_t = typename eval_t::object_t;
        function_path< evaluable_t > path( 
                { object_pattern( "Bool", { variable_pattern( "a" ) } )
                , object_pattern( "Bool", { variable_pattern( "b" ) } ) }, 
                variable_pattern( "_" )
                , e );
        return object_t( function_object< evaluable_t >( { std::move( path ) }, 2 ) );
    };

    static void bool_binary( eval_t& e, std::function< bool( bool, bool ) > f )
    {
        object_t a = e.state._store.lookup( "a" );
        object_t b = e.state._store.lookup( "b" );
        e.state.push_value( object_t( f( a.template get_value< bool >()
                                       , b.template get_value< bool >() ) ) );
    }

    static void b_and( eval_t& e ) { bool_binary( e, []( int a, int b ){ return a && b; } ); }
    static void b_or( eval_t& e ) { bool_binary( e, []( int a, int b ){ return a || b; } ); }

    static const inline std::map< std::string, std::pair< builtin_t, builtin_wrapper > > bindings {
        { "__int_add__", { add, int_binary_o } },
        { "__int_sub__", { sub, int_binary_o } },
        { "__int_mul__", { mul, int_binary_o } },
        { "__int_div__", { div, int_binary_o } },
        { "__int_mod__", { mod, int_binary_o } },
        { "__bool_and__", { mod, int_binary_o } },
        { "__bool_or__", { mod, int_binary_o } },
        { "+", { add, int_binary_o } },
        { "-", { sub, int_binary_o } },
        { "*", { mul, int_binary_o } },
        { "/", { div, int_binary_o } },
        { "%", { mod, int_binary_o } },
        { "&&", { b_and, bool_binary_o } },
        { "||", { b_or, bool_binary_o } },
    };

    static void add_builtins( eval_t& e )
    {
        for ( const auto& [ k, v ] : bindings ) {
            e.state._store.bind( k, v.second( v.first ) );
        }
    }
};
