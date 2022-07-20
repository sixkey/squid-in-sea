#include <initializer_list>
#include <iostream>
#include <map>
#include "values.hpp"
#include <optional>

#define PATTERN_DBG_FLAG

#ifdef PATTERN_DBG_FLAG
    #define P_DBG_RET( x, y ) ( std::cout << this->to_string() << " " << o.to_string() << " : " << y << std::endl, x )
#else
    #define P_DBG_RET( x, y ) void
#endif 

using matching_t = std::map< std::string, object >;

struct pattern;
struct variable_pattern;
struct object_pattern;
struct literal_pattern;

using pattern_ptr = std::shared_ptr< pattern >;

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

    virtual bool pattern_match( 
            matching_t & matching, 
            const object & o ) const = 0;

    virtual std::string to_string() const = 0;

    /** A contains B iff and only if every object that matches pattern B also
     *  matches pattern A. **/
        
    virtual bool contains( pattern& p ) const = 0;

    virtual pattern_ptr clone() const = 0;

    virtual ~pattern() {};

};

struct variable_pattern : public pattern {
    
    std::string variable_name;

    variable_pattern( std::string variable_name ) 
        : variable_name( std::move( variable_name ) ) {}

    bool pattern_match( matching_t & matching, const object & o ) const override
    {
        auto [ it, succ ] = matching.insert( { variable_name, o } );
        if ( succ ) 
            return true;
        throw pattern_matching_error( 
            "multiple variables with the same name not inplemented" );
    }

    bool contains( pattern& p ) const override {
        return true;
    }

    pattern_ptr clone() const override
    {
        return std::make_unique< variable_pattern >( *this );
    }

    std::string to_string() const override {
        return variable_name;
    }
};

struct literal_pattern : public pattern {
    
    std::string name;
    value_t value;

    literal_pattern( std::string name, value_t value ) 
        : name ( name )
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

    pattern_ptr clone() const override
    {
        return std::make_unique< literal_pattern >( *this );
    }

    virtual bool contains( pattern& p ) const override;
    
    std::string to_string() const override {
        return name + " " + std::to_string( value );
    }
};

struct object_pattern : public pattern {

    std::string name;
    std::vector< pattern_ptr > patterns;

    object_pattern( 
            std::string name, 
            std::vector< pattern_ptr > patterns 
            ) 
        : name( std::move( name ) ), patterns( std::move( patterns ) ) {}


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

    pattern_ptr clone() const override
    {
        return std::make_unique< object_pattern >( *this );
    }

    virtual bool contains( pattern& p ) const override;

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

void tests();
