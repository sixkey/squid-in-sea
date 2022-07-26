#pragma once

#include <map>
#include <stdexcept>
#include <vector>
#include "values.hpp"
#include "pattern.hpp"
#include <cassert>

struct edge {
    pattern_ptr a; 
    pattern_ptr b; 
    identifier id;
};

using edges_t   = std::multimap< identifier, edge >;
using id_path_t = std::vector< identifier >;

class pattern_graph {

    edges_t _edges;
    std::vector< edge > variable_edges;

    public:

    pattern_graph() = default;

    void add_edge( const pattern &p, const pattern &q, identifier id ) {
        edge e = edge{ p.clone(), q.clone(), std::move( id ) };

        if ( p.type == p_variable ) {
            variable_edges.push_back( e );
            return;
        }

        identifier obj_name = p.get_name();
        _edges.insert( { obj_name, e } );
    }

    private:

    template< typename T >
    bool traverse_go
        ( const pattern &current, int max_depth
        , std::vector< identifier > &id_path, T& data
        , bool (*on_pattern)( const pattern&, T&, const std::vector< identifier > ) )
    {
        if ( id_path.size() >= max_depth ) 
            return false;

        if ( on_pattern( current, data, id_path ) ) 
            return true;

        if ( current.type != p_variable ) {
            auto ran = _edges.equal_range( current.get_name() );
            for ( auto i = ran.first; i != ran.second; i ++ ) {
                auto e = i->second;
                if ( contains( *e.a, current ) ) {
                    id_path.push_back( e.id );
                    if ( traverse_go( *e.b, max_depth
                                    , id_path, data, on_pattern ) ) 
                        return true;
                    id_path.pop_back();
                }
            }
        }

        for ( const edge& e : variable_edges ) {
            if ( contains( *e.a, current ) ) {
                id_path.push_back( e.id );
                if ( traverse_go( *e.b, max_depth
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
        , bool (*on_pattern)( const pattern&, T&, const std::vector< identifier > ) )
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

    std::optional< std::vector< identifier > > get_path 
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
                                        std::pair < pattern_ptr
                                                  , id_path_t > > >;
    static bool reachable_step( const pattern& current
                       , reachable_bundle_t &data
                       , id_path_t id_path ) 
    {
        auto &[ max_patterns, patterns ] = data;
        patterns.push_back( { current.clone(), id_path } );
        if ( patterns.size() == max_patterns )
            return true;
        return false;
    }

    /** Returns a vector v with at most <max_patterns> patterns s.t. for every pattern 
     *  q in v there exists a function f s.t. f a = b and a is in p and b is in q. **/
    std::vector< std::pair< pattern_ptr, id_path_t > > reachable 
        ( const pattern &p, int max_depth, int max_patterns )
    {
        reachable_bundle_t bundle = { max_patterns, {} };
        traverse< reachable_bundle_t >( 
                p, max_depth, 
                bundle, reachable_step );
        return std::move( bundle.second );
    }

};

void tests_pattern_graph();
