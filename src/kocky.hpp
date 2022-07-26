#include <string>
#include <variant> 
#include <cassert> 

namespace kck 
{
    template < typename... Ts >
    std::string to_string( std::variant< Ts... > v )
    {
        std::visit( [&]( auto &p ){ return std::to_string( p ); }, v );
    }
}
