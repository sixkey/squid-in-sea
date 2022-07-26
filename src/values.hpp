#include <optional>
#include <string>
#include <vector>
#include "kocky.hpp" 
#pragma once
#include <variant> 
#include <memory>
#include <iostream>

#include "ast.hpp"
#include "pattern.hpp"

template< typename evaluable_t >
struct function_path 
{
    std::vector< pattern_ptr > input_patterns;
    pattern_ptr output_pattern;
    evaluable_t evaluable; 

    function_path
        ( std::vector< pattern_ptr > input_patterns 
        , pattern_ptr output_pattern
        , evaluable_t evaluable ) 
        : input_patterns( std::move( input_patterns ) )
        , output_pattern( output_pattern )
        , evaluable( evaluable ) 
    {}
};

template< typename evaluable_t >
struct function_object 
{
    using function_path = function_path< evaluable_t >;

    std::vector< function_path > paths;

    function_object( std::vector< function_path > paths ) : paths( std::move( paths ) ) {};

    bool operator ==( const function_object &o ) const { return true; };

    std::string to_string() const { return "<fun>"; };
};


using value_t    = std::variant< int
                               , bool
                               , function_object< std::string > 
                               , function_object< ast::node_ptr > >;

std::string value_to_string( const value_t &value );

struct object {

    using attrs_t    = std::vector< object >;
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
            return "( " + name + " " + value_to_string( *value ) + " )";
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

