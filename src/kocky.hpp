#pragma once

#include <string>
#include <variant> 
#include <cassert> 

#define NOT_IMPLEMENTED() throw std::runtime_error("NOT IMPLEMENTED");
#define TODO()            throw std::runtime_error("TODO");

namespace kck 
{
    template < typename... Ts >
    std::string to_string( std::variant< Ts... > v )
    {
        return std::visit( [&]( auto &p ){ return std::to_string( p ); }, v );
    }

    template < typename P, typename Q > 
    struct concat;

    template < typename... as, typename... bs >
    struct concat< std::variant< as... >, std::variant< bs... > > 
    {
        using type = std::variant< as..., bs... >;
    };
}

