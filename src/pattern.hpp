#pragma once

#include <initializer_list>
#include <iostream>
#include <map>
#include "values.hpp"
#include <optional>

// #define PATTERN_DBG_FLAG

#ifdef PATTERN_DBG_FLAG
    #define P_DBG_RET( x, y ) ( std::cout << this->to_string() << " " << o.to_string() << " : " << y << std::endl, x )
#else
    #define P_DBG_RET( x, y ) ( x )
#endif 

using matching_t = std::map< std::string, object >;

struct pattern;
struct variable_pattern;
struct object_pattern;
struct literal_pattern;

using pattern_ptr = std::shared_ptr< pattern >;

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

    virtual ~pattern() {};

    
    virtual bool pattern_match( 
            matching_t & matching, 
            const object & o ) const = 0;

    /** A contains B iff and only if every object that matches pattern B also
     *  matches pattern A. **/
        
    virtual bool contains( const pattern& p ) const = 0;

    
    virtual obj_name_t get_name() const = 0;


    virtual pattern_ptr clone() const = 0;

    virtual std::string to_string() const = 0;


};

struct variable_pattern : public pattern {
    
    std::string variable_name;

    variable_pattern( std::string variable_name ) 
        : pattern( p_variable )
        , variable_name( std::move( variable_name ) ) {}

    bool pattern_match( matching_t & matching, const object & o ) const override
    {
        auto [ it, succ ] = matching.insert( { variable_name, o } );
        if ( succ ) 
            return true;
        throw pattern_matching_error( 
            "multiple variables with the same name not inplemented" );
    }

    bool contains( const pattern& p ) const override {
        return true;
    }


    obj_name_t get_name() const override {
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
    
    obj_name_t name;
    value_t value;

    literal_pattern( obj_name_t name, value_t value ) 
        : pattern( p_literal )
        , name ( name )
        , value ( value ) {}

    bool pattern_match( matching_t & matching, const object & o ) const override {
        if ( !o.primitive ) {
            return P_DBG_RET( false, "is not a primitive" );
        }
        if ( o.name != name ) {
            return P_DBG_RET( false, "names do not match" );
        }
        if ( o.hidden_value != value ) {
            return P_DBG_RET( false, "values do not match" );
        }
        return true;
    }


    obj_name_t get_name() const override {
        return name;
    }


    pattern_ptr clone() const override
    {
        return std::make_shared< literal_pattern >( *this );
    }

    virtual bool contains( const pattern& p ) const override;
    
    std::string to_string() const override {
        return name + " " + std::to_string( value );
    }
};

struct object_pattern : public pattern {

    obj_name_t name;
    std::vector< pattern_ptr > patterns;

    object_pattern( 
            obj_name_t name, 
            std::vector< pattern_ptr > patterns 
            ) 
        : pattern( p_object )
        , name( std::move( name ) ), patterns( std::move( patterns ) ) {}


    bool pattern_match( matching_t & matching, const object & o ) const override {
        if ( o.name != name ) {
            return P_DBG_RET( false, "names do not match" );
        }
        if ( o.primitive ) {
            if ( patterns.size() != 1 ) {
                return P_DBG_RET( false, "size does not match" );
            }
            return patterns[ 0 ]->pattern_match( matching, o );
        } 
        if ( o.values.size() != patterns.size() ) {
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
        for ( int i = 0; i < patterns.size(); i++ ) {
            if ( ! patterns[ i ]->pattern_match( matching, o.values[ i ] ) )
                return false;
        }
        return true;
    }


    obj_name_t get_name() const override {
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
