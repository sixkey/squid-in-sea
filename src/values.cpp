#include "values.hpp"

bool match( const pattern& p, const object& o, matching_t& m )
{
    return std::visit( [&]( const auto &v ){ return match( v, o, m ); }, p );
}

bool match( const variable_pattern& p, const object& o, matching_t& match )
{
    const auto &[ it, succ ] = match.insert( { p.variable_name, o } );
    if ( !succ )
        throw std::runtime_error( "multiple variables with same name not implemented" );
    return true;
}

bool match( const object_pattern& p, const object& o, matching_t& m )
{
    if ( o.name != p.name ) {
        return P_DBG_RET( false, "names do not match" );
    }

    if ( o.omega() ) {
        if ( p.patterns.size() != 1 ) {
            return P_DBG_RET( false, "size does not match" );
        }
        return match( p.patterns[ 0 ], o, m );
    } 
    
    if ( o.get_attrs().size() != p.patterns.size() ) {
        return P_DBG_RET( false, "size does not match" );
    }

    /**
     * Primitive objects have one value, which is empty and it serves as a
     * "self loop". This means, that you can match primitive objects
     * indefinitely as 'n' in Int n is the Int n itself. 
     *  
     * 3 <> < Int n > => { n : 3 }
     * 3 <> < Int < Int n > > => { n : 3 }
     * 3 <> < Int < Int < Int n > > > => { n : 3 }
     *
     * **/
    for ( int i = 0; i < p.patterns.size(); i++ ) {
        if ( !match( p.patterns[ i ], o.get_attrs()[ i ], m ) ) 
            return false;
    }
    return true;
}

std::optional< matching_t > match ( const pattern& p, const object& o )
{
    matching_t matching;
    return match( p, o, matching ) 
        ? std::move( matching ) 
        : std::optional< matching_t >();
}
