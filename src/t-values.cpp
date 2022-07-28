#include "values.hpp" 

void test_pattern ()
{

    auto basic_test = [] ( const pattern &p, const object &o, const matching_t &m ) {
        matching_t rm = {};
        if ( ! match( p, o, rm ) ) 
        {
            std::cout << std::visit( [&]( const auto &v ){ return v.to_string(); }, p ) 
                      << " " << o.to_string() << std::endl;
            assert( false );
        }
        assert( rm == m );
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
            { { "x", object( "Int", 3 ) }
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
        { { "n", object( "Int", 3 ) } }
    );

    basic_test(
        object_pattern( "Int", { variable_pattern( "n" ) } ),
        object( "Int", 3 ), 
        { { "n", object( "Int", 3 ) } }
    );
}

void tests_values()
{
    test_pattern();
}
