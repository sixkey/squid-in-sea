#include <assert.h> 

#include "pattern.hpp"
#include "kocky.hpp"

void test_contains()
{
    literal_pattern int_3 = literal_pattern( "Int", 3 );
    literal_pattern int_6 = literal_pattern( "Int", 6 );
    variable_pattern a = variable_pattern( "a" );
    variable_pattern b = variable_pattern( "b" );
    object_pattern int_a = object_pattern( "Int", { a } );
    object_pattern int_b = object_pattern( "Int", { b } );
    object_pattern bool_a = object_pattern( "Bool", { a } );
    object_pattern int_int_a = object_pattern( "Int", { int_a } );

    assert( contains( a, int_3 ) );
    assert( contains( a, b ) );
    assert( contains( a, int_a ) );
    assert( contains( int_a, int_3 ) );
    assert( ! contains( int_3, int_6 ) );
    assert( contains( int_a, int_b ) );
    assert( contains( int_b, int_a ) );
    assert( contains( int_int_a, int_3 ) );
    assert( contains( int_a, int_int_a ) );
    assert( contains( int_a, int_int_a ) );
    assert( ! contains( bool_a, int_a ) );
    assert( ! contains( int_a, bool_a ) );
}

void tests_pattern()
{
    test_contains();
}
