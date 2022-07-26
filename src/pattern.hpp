#pragma once

#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <memory>
#include <variant>
#include <vector> 

#include "types.hpp"
#include "kocky.hpp"

// #define PATTERN_DBG_FLAG

#ifdef PATTERN_DBG_FLAG
    #define P_DBG_RET( x, y ) ( std::cout << this->to_string() << " " << o.to_string() << " : " << y << std::endl, x )
#else
    #define P_DBG_RET( x, y ) ( x )
#endif 

struct pattern;
struct variable_pattern;
struct object_pattern;
struct literal_pattern;

using pattern_ptr = std::shared_ptr< pattern >;
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

struct pattern {
    /**
     * :person_facepalming:
     *              -- tesla
     * **/
    pattern_type type;

    pattern ( pattern_type type ) : type ( type ) {};

    virtual ~pattern() = default;

    /** A contains B iff and only if every object that matches pattern B also
     *  matches pattern A. **/
        
    virtual bool contains( const pattern& p ) const = 0;

    virtual identifier get_name() const = 0;

    virtual pattern_ptr clone() const = 0;

    virtual std::string to_string() const = 0;

};

struct variable_pattern : public pattern {
    
    identifier variable_name;

    variable_pattern( std::string variable_name ) 
        : pattern( p_variable )
        , variable_name( std::move( variable_name ) ) {}

    bool contains( const pattern& p ) const override {
        return true;
    }

    identifier get_name() const override {
        throw std::runtime_error( "variable pattern does not have a name" );
    }

    pattern_ptr clone() const override
    {
        return std::make_shared< variable_pattern >( *this );
    }

    std::string to_string() const override {
        return variable_name;
    }
};


struct literal_pattern : public pattern {
    
    identifier name;
    pattern_value_t value;

    literal_pattern( identifier name, pattern_value_t value ) 
        : pattern( p_literal )
        , name ( name )
        , value ( value ) {}

    identifier get_name() const override {
        return name;
    }

    pattern_ptr clone() const override
    {
        return std::make_shared< literal_pattern >( *this );
    }

    virtual bool contains( const pattern& p ) const override;
    
    std::string to_string() const override {
        return name + " " + kck::to_string( value );
    }
};

struct object_pattern : public pattern {

    identifier name;
    std::vector< pattern_ptr > patterns;

    object_pattern
        ( identifier name
        , std::vector< pattern_ptr > patterns ) 
        : pattern( p_object )
        , name( std::move( name ) ), patterns( std::move( patterns ) ) {}

    identifier get_name() const override {
        return name;
    }

    pattern_ptr clone() const override
    {
        return std::make_shared< object_pattern >( *this );
    }

    virtual bool contains( const pattern& p ) const override;

    std::string to_string() const override {
        std::string res = "( " + name;
        for ( auto& v : patterns ) {
            res.append( " " );
            res.append( v->to_string() );
        }
        res.append( " )" );
        return res;
    }

};

void tests_pattern();
