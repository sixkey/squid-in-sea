#pragma once

#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <memory>
#include <variant>
#include <vector>

#include "kocky.hpp"
#include "pprint.hpp"
#include "types.hpp"

struct variable_pattern;
struct object_pattern;
template< typename value_t >
struct literal_pattern;

using pattern = std::variant< variable_pattern
                            , literal_pattern< int >
                            , literal_pattern< bool >
                            , object_pattern >;

struct variable_pattern {

    identifier_t variable_name;

    variable_pattern( identifier_t variable_name )
        : variable_name( std::move( variable_name ) ) {}
};

template< typename value_t >
struct literal_pattern {

    identifier_t name;
    value_t value;

    literal_pattern( identifier_t name, value_t value )
        : name ( name )
        , value ( value ) {}
};

struct object_pattern {

    identifier_t name;
    std::vector< pattern > patterns;

    object_pattern
        ( identifier_t name
        , std::vector< pattern > patterns )
        : name( std::move( name ) ), patterns( std::move( patterns ) ) {}
};

///////////////////////////////////////////////////////////////////////////////
// Show
///////////////////////////////////////////////////////////////////////////////

static std::ostream& operator<<( std::ostream& os, const pattern& p );

static std::ostream& operator<<( std::ostream& os, const variable_pattern& p )
{
    return os << "Variable " << p.variable_name;
}

template < typename T >
static std::ostream& operator<<( std::ostream& os, const literal_pattern< T >& p )
{
    return os << "Literal " << p.name << " " << p.value;
}

static std::ostream& operator<<( std::ostream& os, const object_pattern& p )
{
    os << "Object " << p.name;
    for ( const auto& c : p.patterns )
        os << " (" << c << " )";
    return os;
}

static std::ostream& operator<<( std::ostream& os, const pattern& p )
{
    std::visit( [&]( const auto& v ){ os << v; }, p );
    return os;
}

static std::string get_name( const variable_pattern& v )
{
    assert( false );
}

template < typename T >
static std::string get_name( const literal_pattern< T >& v )
{
    return v.name;
}

static std::string get_name( const object_pattern& v )
{
    return v.name;
}

static std::string get_name( const pattern& p )
{
    return std::visit( [] ( const auto& v ){ return get_name( v ); }, p );
}

///////////////////////////////////////////////////////////////////////////////
// Order on patterns
///////////////////////////////////////////////////////////////////////////////

static bool contains( const pattern& p, const pattern& q );

static bool contains( const variable_pattern& a, const variable_pattern& b )
{
    return true;
};

template< typename T >
static bool contains( const variable_pattern& a, const literal_pattern< T >& b )
{
    return true;
};

static bool contains( const variable_pattern& a, const object_pattern& b )
{
    return true;
};

template< typename T >
static bool contains( const literal_pattern< T >& a, const variable_pattern& b ) {
    return false;
}

template< typename P, typename Q >
static bool contains( const literal_pattern< P >& a, const literal_pattern< Q >& b ) {
    return false;
}

template< typename T >
static bool contains( const literal_pattern< T >& a, const literal_pattern< T >& b ) {
    return a.name == b.name
        && a.value == b.value;
}

template< typename T >
static bool contains( const literal_pattern< T >& a, const object_pattern& b ) {
    return a.name == b.name
        && b.patterns.size() == 1
        && contains( pattern( a ), b.patterns[ 0 ] );
}

template< typename T >
static bool contains( const object_pattern& a, const literal_pattern< T >& b ) {
    return a.name == b.name
        && a.patterns.size() == 1
        && contains( a.patterns[ 0 ], pattern( b ) );
}

static bool contains( const object_pattern& a, const variable_pattern& b ) {
    return false;
}

static bool contains( const object_pattern& a, const object_pattern& b ) {
    if ( a.name != b.name || a.patterns.size() != b.patterns.size() )
        return false;
    for ( int i = 0; i < a.patterns.size(); i++ )
        if ( ! contains( a.patterns[ i ], b.patterns[ i ] ) )
            return false;
    return true;
}

static bool contains( const pattern& p, const pattern& q )
{
    return std::visit(
        [&]( auto &l ){
            return std::visit(
                [&]( auto &r ){
                    return contains( l, r );
                }, q ); }
            , p );
};
