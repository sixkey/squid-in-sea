#pragma once

#include <map>
#include <stdexcept>
#include <variant>
#include <vector>
#include "values.hpp"
#include "pattern.hpp"
#include <cassert>

struct edge {
    pattern a; 
    pattern b; 
    identifier_t id;

    friend std::ostream& operator<<( std::ostream& os, const edge& e )
    {
        return os << e.id << " : " << e.a << " -> " << e.b;
    }
};

using edges_t   = std::multimap< identifier_t, edge >;
using id_path_t = std::vector< identifier_t >;

class pattern_graph {

    edges_t _edges;
    std::vector< edge > variable_edges;

    public:

    pattern_graph() = default;

    void add_edge( const pattern &p, const pattern &q, identifier_t id ) {
        std::visit( [&]( auto &v ){ this->add_edge( v, q, std::move( id ) ); }, p );
    }

    void add_edge( const variable_pattern &p, const pattern &q, identifier_t id ) {
        variable_edges.push_back( { p, q, std::move( id ) } );
    }

    void add_edge( const object_pattern &p, const pattern &q, identifier_t id ) {
        _edges.insert( { p.name, { p, q, std::move( id ) } } );
    }

    template < typename T > 
    void add_edge( const literal_pattern< T > &p, const pattern &q, identifier_t id ) {
        _edges.insert( { p.name, { p, q, std::move( id ) } } );
    }

    private:

    template< typename T >
    bool traverse_go
        ( const pattern &current, int max_depth
        , std::vector< identifier_t > &id_path, T& data
        , bool (*on_pattern)( const pattern&, T&, const std::vector< identifier_t > ) )
    {
        if ( id_path.size() >= max_depth ) 
            return false;

        if ( on_pattern( current, data, id_path ) ) 
            return true;

        if ( ! std::holds_alternative< variable_pattern >( current ) ) {
            auto ran = _edges.equal_range( get_name( current ) );
            for ( auto i = ran.first; i != ran.second; i ++ ) {
                auto e = i->second;
                if ( contains( e.a, current ) ) {
                    id_path.push_back( e.id );
                    if ( traverse_go( e.b, max_depth
                                    , id_path, data, on_pattern ) ) 
                        return true;
                    id_path.pop_back();
                }
            }
        }

        for ( const edge& e : variable_edges ) {
            if ( contains( e.a, current ) ) {
                id_path.push_back( e.id );
                if ( traverse_go( e.b, max_depth
                                , id_path, data, on_pattern ) )
                    return true;
                id_path.pop_back();
            }
        }
        
        return false;
    }

    template< typename T >
    std::pair< bool, id_path_t > traverse 
        ( const pattern &current, int max_depth, T& data
        , bool (*on_pattern)( const pattern&, T&, const std::vector< identifier_t > ) )
    {
        id_path_t id_path;
        bool res = traverse_go( current, max_depth, id_path, data, on_pattern );
        return { res, std::move( id_path ) };
    }

    public:


    static bool get_path_step( const pattern& current
                      , const pattern& destination
                      , id_path_t id_path ) 
    {
        return contains( destination, current );
    }

    std::optional< std::vector< identifier_t > > get_path 
        ( const pattern &p, const pattern &q, int max_depth )
    {
        for ( int i = 1; i < max_depth; i++ ) {
            auto [ succ, id_path ] = 
                traverse< const pattern & >( p, i, q, get_path_step );
            if ( succ ) 
                return id_path;
        }
        return {};
    }

    using reachable_bundle_t = std::pair< int, 
                                    std::vector< 
                                        std::pair < pattern
                                                  , id_path_t > > >;
    static bool reachable_step( const pattern& current
                       , reachable_bundle_t &data
                       , id_path_t id_path ) 
    {
        auto &[ max_patterns, patterns ] = data;
        patterns.push_back( { current, id_path } );
        if ( patterns.size() == max_patterns )
            return true;
        return false;
    }

    /** Returns a vector v with at most <max_patterns> patterns s.t. for every pattern 
     *  q in v there exists a function f s.t. f a = b and a is in p and b is in q. **/
    std::vector< std::pair< pattern, id_path_t > > reachable 
        ( const pattern &p, int max_depth, int max_patterns )
    {
        reachable_bundle_t bundle = { max_patterns, {} };
        traverse< reachable_bundle_t >( 
                p, max_depth, 
                bundle, reachable_step );
        return std::move( bundle.second );
    }

};
