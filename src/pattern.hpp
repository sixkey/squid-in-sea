#pragma once

#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <memory>
#include <variant>
#include <vector> 

#include "kocky.hpp"
#include "types.hpp"

// #define PATTERN_DBG_FLAG

#ifdef PATTERN_DBG_FLAG
    #define P_DBG_RET( x, y ) ( std::cout << this->to_string() << " " << o.to_string() << " : " << y << std::endl, x )
#else
    #define P_DBG_RET( x, y ) ( x )
#endif 

struct variable_pattern;
struct object_pattern;
struct literal_pattern;

using pattern = std::variant< variable_pattern, literal_pattern, object_pattern >;
using pattern_value_t = std::variant< int, bool >;

enum pattern_type {
    p_literal, 
    p_variable, 
    p_object
};

class pattern_matching_error : std::exception {

private:

    char * _message;

public:

    pattern_matching_error( const char * message ) {
        message = _message; 
    }

    char * what () {
        return _message;
    }

};

struct variable_pattern {
    
    identifier variable_name;

    variable_pattern( identifier variable_name ) 
        : variable_name( std::move( variable_name ) ) {}

    std::string to_string() const {
        return variable_name;
    }
};


struct literal_pattern {
    
    identifier name;
    pattern_value_t value;

    literal_pattern( identifier name, pattern_value_t value ) 
        : name ( name )
        , value ( value ) {}

    std::string to_string() const {
        return name + " " + std::visit( [&]( auto &p ){ return std::to_string( p ); }, value );
    }
};

struct object_pattern {

    identifier name;
    std::vector< pattern > patterns;

    object_pattern
        ( identifier name
        , std::vector< pattern > patterns ) 
        : name( std::move( name ) ), patterns( std::move( patterns ) ) {}

    std::string to_string() const
    {
        std::string res = "( " + name;
        for ( auto& v : patterns ) {
            res.append( " " );
            res.append( std::visit( [&]( auto &p ){ return p.to_string(); }, v ) );
        }
        res.append( " )" );
        return res;
    }
};

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

    static bool contains( const variable_pattern& a, const literal_pattern& b ) 
    {   
        return true; 
    };

    static bool contains( const variable_pattern& a, const object_pattern& b ) 
    {   
        return true; 
    };

    static bool contains( const literal_pattern& a, const variable_pattern& b ) {
        return false;
    }

    static bool contains( const literal_pattern& a, const literal_pattern& b ) {
        return a.name == b.name 
            && a.value == b.value;
    }

    static bool contains( const literal_pattern& a, const object_pattern& b ) {
        return a.name == b.name
            && b.patterns.size() == 1
            && contains( pattern( a ), b.patterns[ 0 ] );
    }

    static bool contains( const object_pattern& a, const literal_pattern& b ) {
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
            if ( !contains( a.patterns[ i ], b.patterns[ i ] ) ) 
                return false;
        return true;
    }
};

bool contains( const pattern& p, const pattern& q );

void tests_pattern();
