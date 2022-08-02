#pragma once

#include <variant>

template < typename L, typename R >
struct either 
{
    std::variant< L, R > v;

    either( L l ) : v ( l ) {}
    either( R r ) : v ( r ) {}

    bool isright()
    {
        return std::holds_alternative< R >( v );
    }

    bool isleft() 
    {
        return std::holds_alternative< L >( v );
    }

    R right()
    {
        return std::get< R >( v );
    }

    L left()
    {
        return std::get< L >( v );
    }

    template < typename fun_r >
    either& then( fun_r f )
    {
        if ( std::holds_alternative< R >( v ) )
            fun_r( std::get< R >( v ) );
        return *this;
    }

    template < typename fun_l >
    either& otherwise( fun_l f )
    {
        if ( std::holds_alternative< L >( v ) )
            fun_r( std::get< L >( v ) );
        return *this;
    }

};
