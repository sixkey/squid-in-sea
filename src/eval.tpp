#include "ast.hpp"
#include "eval.hpp"
#include "pattern.hpp"
#include "pprint.hpp"
#include "values.hpp"

void simple_test()
{
    eval e;

    ast::literal< int > int_0( 0 );
    ast::literal< int > int_1( 1 );
    ast::literal< int > int_2( 2 );
    ast::literal< int > int_3( 3 );
    ast::literal< int > int_4( 4 );
    ast::literal< int > int_42( 42 );

    variable_pattern a( "a" );
    variable_pattern b( "b" );

    const auto& const_42 = ast::function_def{ 
        { ast::function_path{ { ast::variable_pattern{ "_" } }
                            , ast::variable_pattern{ "_" } 
                            , ast::clone( int_42 ) } }, 
        1 };

    ast::function_call call_const_42( ast::clone( const_42 )
                                    , { ast::clone( int_0 ) } );
    
    e.push( ast::clone( call_const_42 ) );
    e.run();
}


int main()
{
    simple_test();
}
