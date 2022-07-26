#include "values.hpp"

std::string value_to_string( const value_t &value )
{
    return std::visit( [&]( auto &&p ){
        using T = std::decay_t< decltype( p ) >;
        if constexpr ( std::is_same_v< T, int > ) {
            return std::to_string( p );
        } else if constexpr( std::is_same_v< T, bool > ) {
            return std::to_string( p );
        } else if constexpr( std::is_same_v< T, function_object< std::string > > ) {
            return p.to_string();
        } else if constexpr( std::is_same_v< T, function_object< ast::node_ptr > > ) {
            return p.to_string();
        } else if constexpr( std::is_same_v< T, bool > ) {
            return std::to_string( p );
        } else {
            return "Unknown";
        }
    }, value );
}
