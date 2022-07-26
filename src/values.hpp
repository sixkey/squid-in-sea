#include <optional>
#include <string>
#include <vector>
#include "kocky.hpp" 
#pragma once
#include <variant> 
#include <memory>
#include <iostream>

struct function_object 
{
    bool operator ==( const function_object &o ) const { return true; };

    std::string to_string() { return "<fun>"; };
};

struct object {

    using attrs_t = std::vector< object >;
    using value_t    = std::variant< int >;
    using obj_name_t = std::string;
 
    obj_name_t name; 
    std::variant< attrs_t, value_t > content;

    object() {}

    object( obj_name_t name, attrs_t attrs ) 
        : name( std::move( name ) )
        , content( std::move( attrs ) ) {}

    object( obj_name_t name, value_t hidden_value ) 
        : name( std::move( name ) )
        , content( std::move( hidden_value ) ) {}

    bool operator==( object other ) const
    {
        return other.name == name && other.content == content;
    }

    friend std::ostream& operator<<( std::ostream &out, const object &o ) 
    {
        return out << o.to_string();
    }

    bool omega () const
    {
        return std::holds_alternative< value_t >( content );
    }

    std::string to_string() const {
        if ( const value_t *value = std::get_if< value_t >( &content ) ) {
            return "( " + name + " " + kck::to_string( *value ) + " )";
        } 
        if ( const attrs_t *attrs = std::get_if< attrs_t >( &content ) ) {
            std::string res = "( ";
            for ( auto& v : *attrs ) {
                res.append( " " );
                res.append( v.to_string() );
            }
            return res + " )";
        } 
        assert( false );
    }
};
