#include "pattern.hpp"
#include <assert.h>

std::ostream& operator<<( std::ostream &out, const matching_t &m ) {
    out << "{\n";
    for ( auto [ key, val ] : m ) {
        out << " " << key << ": " << val << ",\n";
    }
    out << "}";
    return out;
}


bool literal_pattern::contains( const pattern& p ) const
{
    if ( auto v = dynamic_cast< const variable_pattern* > ( &p ); v != nullptr ) {
        return false;
    }
    if ( auto v = dynamic_cast< const literal_pattern* > ( &p ); v != nullptr ) {
        return v->name == name && v->value == value;
    }
    if ( auto v = dynamic_cast< const object_pattern* > ( &p ); v != nullptr )
    {
        if ( v->name == name && v->patterns.size() == 1 ) {
            return p.contains( *v->patterns[ 0 ] );
        }
        return false;
    }
    assert( false );
}

bool object_pattern::contains( const pattern& p ) const
{
    if ( auto v = dynamic_cast< const variable_pattern* > ( &p ); v != nullptr ) {
        return false;
    }
    if ( auto v = dynamic_cast< const literal_pattern* > ( &p ); v != nullptr ) {
        return v->name == name 
            && patterns.size() == 1 
            && patterns[ 0 ]->contains( p );
    }
    if ( auto v = dynamic_cast< const object_pattern* > ( &p ); v != nullptr ) {
        if ( v->name == name && v->patterns.size() == patterns.size() ) {
            for ( int i = 0; i < patterns.size(); i++ )
                if ( ! patterns[ i ]->contains( *v->patterns[ i ] ) ) 
                    return false;
            return true;
        }
        return false;
    }
    assert( false );
}


void test_pattern ()
{

    auto basic_test = [] ( const pattern &p, const object &o, const matching_t &m ) {
        matching_t rm = {};
        if ( !p.pattern_match( rm, o ) ) 
        {
            std::cout << p.to_string() << " " << o.to_string() << std::endl;
            assert( false );
        }
        assert( rm == m );
    };

    auto fail_test = [] ( const pattern &p, const object &o ) {
        matching_t rm = {};
        assert( !p.pattern_match( rm, o ) );
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
        std::vector< pattern_ptr > patterns;
        patterns.push_back( std::make_unique< literal_pattern >( "Int", 3 ) );
        patterns.push_back( std::make_unique< literal_pattern >( "Int", 5 ) );
        object_pattern p = object_pattern( "Point", std::move( patterns ) );

        basic_test(
            p,
            object( "Point", { 
                object( "Int", 3 ), 
                object( "Int", 5 ) 
            } ), 
            matching_t()
        );

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

    { 
        std::vector< pattern_ptr > patterns;
        patterns.push_back( std::make_unique< variable_pattern >( "n" ) );

        basic_test(
            object_pattern( "Int", std::move( patterns ) ),
            object( "Int", 3 ), 
            { { "n", object( "Int", 3 ) } }
        );
    }
}

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
    test_pattern();
    test_contains();
}
