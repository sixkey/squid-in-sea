#include <assert.h> 

#include "pattern.hpp"

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

bool contains( const pattern& p, const pattern& q ) {
    return p.contains( q );
}
