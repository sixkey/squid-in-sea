#pragma once

#include <string>
#include <variant> 
#include <cassert> 

#define NOT_IMPLEMENTED() throw std::runtime_error("NOT IMPLEMENTED");

namespace kck 
{
    template < typename... Ts >
    std::string to_string( std::variant< Ts... > v )
    {
        return std::visit( [&]( auto &p ){ return std::to_string( p ); }, v );
    }
}
