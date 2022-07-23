#include <optional>
#include <string>
#include <vector>
#pragma once

#include <variant> 
#include <memory>
#include <iostream>

using value_t = int;
using obj_name_t = std::string;

struct object {

    obj_name_t name; 
    std::vector< object > values;
    value_t hidden_value = false;
    bool primitive = false;

    object() {}

    object( obj_name_t name, std::vector< object > values ) 
        : name( std::move( name ) )
        , values( std::move( values ) ) {}

    object( obj_name_t name, value_t hidden_value ) 
        : name( std::move( name ) )
        , hidden_value( std::move( hidden_value ) )
        , primitive( true ) {}

    bool operator==( object other ) const
    {
        return other.name == name 
            && other.values.size() == values.size() 
            && other.primitive == primitive 
            && ( !primitive || ( hidden_value == other.hidden_value ) );
    }

    friend std::ostream& operator<<( std::ostream &out, const object &o ) {
        return out << o.to_string();
    }

    std::string to_string() const {
        if ( primitive ) {
            return "( " + name + " " + std::to_string( hidden_value ) + " )";
        }
        std::string res = "( ";
        for ( auto& v : values ) {
            res.append( " " );
            res.append( v.to_string() );
        }
        return res + " )";
    }
};
