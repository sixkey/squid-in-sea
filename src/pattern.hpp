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

// #define PATTERN_DBG_FLAG

#ifdef PATTERN_DBG_FLAG
    #define P_DBG_RET( x, y ) ( std::cout << this->to_string() << " " << o.to_string() << " : " << y << std::endl, x )
#else
    #define P_DBG_RET( x, y ) ( x )
#endif

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
// PrinterVisitor
///////////////////////////////////////////////////////////////////////////////

struct pattern_printer {

    pprint::PrettyPrinter printer;

    void accept( const pattern& p )
    {
        std::visit( ON_THIS( accept ), p );
    }

    template< typename T >
    void accept( const literal_pattern< T >& l )
    {
        printer.print_inline( "PLiteral", l.name, l.value );
    }

    void accept( const variable_pattern& v )
    {
        printer.print_inline( "PVariable", v.variable_name );
    }

    void accept( const object_pattern& v )
    {
        printer.print_inline( "PObject", v.name );
        for ( const auto& c : v.patterns ) {
            printer.print_inline( "( " );
            accept( c );
            printer.print_inline( ") " );
        }
    }
};

static std::ostream& operator<<( std::ostream& os, const pattern& p )
{
    pattern_printer printer{ pprint::PrettyPrinter( os ) };
    printer.accept( p );
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

struct comparator {

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
};

bool contains( const pattern& p, const pattern& q );

void tests_pattern();
