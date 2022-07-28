#include "pprint.hpp"
#include <map>
#include <ostream>
#include <vector>

template < typename identifier_t, typename store_id >
struct scopestack
{
    using scope = std::map< identifier_t, store_id >;
    std::vector< scope > scopes;

    void add_scope()
    {
        scopes.push_back( {} );
    }

    void pop_scope()
    {
        scopes.pop_back();
    }
    
    void bind( identifier_t name, store_id id )
    {
        assert( !scopes.empty() );
        scope& top = scopes.back();
        top.insert_or_assign( name, id );
    }

    std::optional< store_id > lookup( identifier_t name )
    {
        for ( const scope& c : scopes ) {
            const auto &it = c.find( name );
            if ( it != c.end() )
                return it->second;
        }
        return {};
    }

    friend std::ostream& operator<<( std::ostream& os, const scopestack& s )
    {
        pprint::PrettyPrinter printer( os );
        for( const auto& s : s.scopes )
        {
            printer.print( s );
        }
        return os;
    }
};
