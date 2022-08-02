#pragma once

#include <string>
#include <variant> 
#include <cassert> 
#include "pprint.hpp"

#define NOT_IMPLEMENTED() throw std::runtime_error("NOT IMPLEMENTED");
#define TODO()            throw std::runtime_error("TODO");

#define ON_THIS( x ) [&]( const auto& v ){ return this->x( v ); }

#define TRACE( x... ) do {\
    pprint::PrettyPrinter printer; \
    printer.print( x ); \
} while( 0 );

namespace kck 
{
    template < typename... Ts >
    std::string to_string( std::variant< Ts... > v )
    {
        return std::visit( [&]( auto &p ){ return std::to_string( p ); }, v );
    }

    template < typename T > 
    std::string stringify( const T& t )
    {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }

    template < typename P, typename Q >
    struct concat_p;

    template < typename... as, typename... bs >
    struct concat_p< std::variant< as... >, std::variant< bs... > > 
    {
        using type = std::variant< as..., bs... >;
    };

    template < typename v, typename... vs >
    struct concat {
        using type = typename concat_p< v, typename concat< vs... >::type >::type;   
    };

    template < typename a, typename b >
    struct concat< a, b > 
    {
        using type = typename concat_p< a, b >::type;
    };

}

