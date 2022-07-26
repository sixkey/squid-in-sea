#include <assert.h> 

#include "pattern.hpp"

void test_contains()
{
    literal_pattern int_3 = literal_pattern( "Int", 3 );
    literal_pattern int_6 = literal_pattern( "Int", 6 );
    variable_pattern a = variable_pattern( "a" );
    variable_pattern b = variable_pattern( "b" );
    object_pattern int_a = object_pattern( "Int", { a.clone() } );
    object_pattern int_b = object_pattern( "Int", { b.clone() } );
    object_pattern bool_a = object_pattern( "Bool", { a.clone() } );
    object_pattern int_int_a = object_pattern( "Int", { int_a.clone() } );

    assert( a.contains( int_3 ) );
    assert( a.contains( b ) );
    assert( a.contains( int_a ) );
    assert( int_a.contains( int_3 ) );
    assert( ! int_3.contains( int_6 ) );
    assert( int_a.contains( int_b ) );
    assert( int_b.contains( int_a ) );
    assert( int_int_a.contains( int_3 ) );
    assert( int_a.contains( int_int_a ) );
    assert( int_a.contains( int_int_a ) );
    assert( ! bool_a.contains( int_a ) );
    assert( ! int_a.contains( bool_a ) );
}

void tests_pattern()
{
    test_contains();
}
