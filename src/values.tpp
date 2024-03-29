#include "pattern.hpp"
#include "values.hpp" 


template< typename value_t >
void test_pattern ()
{
    using object = object< value_t >;
    using matching_t = matching_t< value_t >;
    using evaluable_t = int;

    auto basic_test = [] ( const pattern &f, const object &o, const matching_t &m ) {
        auto res = match( f, o );
        if ( ! res.has_value() ) { assert( false ); }
        assert( res.value() == m );
    };

    auto fun_test = [] ( const auto &f, const auto &o, const matching_t &m, const auto& e  ) {
        auto res = match( f, o );
        if ( ! res.isright() ) { assert( false ); }
        assert( res.right().first  == m );
        assert( res.right().second == e );
    };

    auto fail_test = [] ( const pattern &p, const object &o ) {
        assert( ! match( p, o ) );
    };

    basic_test( 
        literal_pattern( "Int", 3 ),    
        object( "Int", 3 ), 
        matching_t()
    );

    fail_test(
        literal_pattern( "Int", 3 ),    
        object( "Int", 5 )
    );

    {
        object_pattern p = object_pattern( "Point", { literal_pattern( "Int", 3 ) 
                                                    , literal_pattern( "Int", 5 ) } );

        basic_test(
            p,
            object( "Point", { 
                object( "Int", 3 ), 
                object( "Int", 5 ) 
            } ), 
            matching_t()
        );

        basic_test( 
            object_pattern( "Point", { variable_pattern( "x" )
                                     , variable_pattern( "y" ) } ),
            object( "Point", {
                object( 3 ),
                object( 4 )
            } ), 
            matching_t{ { "x", object( "Int", 3 ) }
            , { "y", object( "Int", 4 ) } } );


        fail_test( 
            p,
            object( "Point", { 
                object( "Int", 3 ), 
                object( "Int", 4 ) 
            } )
        );
    }

    basic_test(
        variable_pattern( "n" ),
        object( "Int", 3 ), 
        matching_t{ { "n", object( "Int", 3 ) } }
    );

    basic_test(
        object_pattern( "Int", { variable_pattern( "n" ) } ),
        object( "Int", 3 ), 
        matching_t{ { "n", object( "Int", 3 ) } }
    );

    object int_1( 1 );
    object int_2( 2 );
    object int_3( 3 );
    object int_4( 4 );
    object int_5( 5 );

    literal_pattern p_int_1( "Int", 1 );
    literal_pattern p_int_2( "Int", 2 );
    literal_pattern p_int_3( "Int", 3 );
    literal_pattern p_int_4( "Int", 4 );
    literal_pattern p_int_5( "Int", 5 );
    object_pattern p_int_a( "Int", { variable_pattern( "a" ) } );
    object_pattern p_int_b( "Int", { variable_pattern( "b" ) } );

    function_object< int > funobj( {
        function_path< int >( { p_int_3, p_int_5 }, variable_pattern( "r" ), 0 ),
        function_path< int >( { p_int_a, p_int_5 }, variable_pattern( "r" ), 1 ),
        function_path< int >( { p_int_3, p_int_b }, variable_pattern( "r" ), 2 ),
        function_path< int >( { p_int_a, p_int_b }, variable_pattern( "r" ), 3 )
    }, 2 );

    fun_test(
        funobj,
        std::vector< object >{ int_3, int_5 }, 
        matching_t(),
        0 
    );

    fun_test(
        funobj,
        std::vector< object >{ int_1, int_5 }, 
        { { "a", int_1 } },
        1 
    );

    fun_test(
        funobj,
        std::vector< object >{ int_3, int_1 }, 
        { { "b", int_1 } },
        2 
    );

    fun_test(
        funobj,
        std::vector< object >{ int_1, int_2 }, 
        { { "a", int_1 } 
        , { "b", int_2 } },
        3 
    );
}

struct test_types_
{
    using fun_obj_t = function_object< int >;
    using value_t = std::variant< int
                                , bool
                                , fun_obj_t >;
    template< typename T > 
    static constexpr const char * type_name() {
        if constexpr ( std::is_same< T, int >::value ) 
            return "Int";
        else if constexpr ( std::is_same< T, bool >::value ) 
            return "Bool";
        else if constexpr ( std::is_same< T, fun_obj_t >::value )
            return "Fun";
        else 
            assert( false );
    }
};

void test_patterns()
{
    test_pattern< test_types_ >();
}

int main()
{
    test_patterns();
}

