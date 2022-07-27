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
    std::vector< pattern > input_patterns;
    pattern output_pattern;
    evaluable_t evaluable; 

    function_path
        ( std::vector< pattern > input_patterns 
        , pattern output_pattern
        , evaluable_t evaluable ) 
        : input_patterns( std::move( input_patterns ) )
        , output_pattern( output_pattern )
        , evaluable( evaluable ) 
    {}
};

template< typename evaluable_t >
struct function_object 
{
    using function_path_t = function_path< evaluable_t >;

    std::vector< function_path_t > paths;
    int arity_ = 0;

    function_object( std::vector< function_path_t > paths, int arity ) 
                   : paths( std::move( paths ) ) 
                   , arity_( arity ) {};

    bool operator ==( const function_object &o ) const { return true; };

    std::string to_string() const { return "<fun>"; };

    int arity() { return arity_; };
};

struct object {

    using attrs_t    = std::vector< object >;
    using obj_name_t = std::string;
    using fun_obj_t  = function_object< ast::node_ptr >;
    using value_t    = std::variant< int
                                   , function_object< ast::node_ptr > >;

    obj_name_t name; 
    std::variant< attrs_t, value_t > content;

    object() {}

    object( obj_name_t name, attrs_t attrs ) 
        : name( std::move( name ) )
        , content( std::move( attrs ) ) {}

    object( obj_name_t name, value_t hidden_value ) 
        : name( std::move( name ) )
        , content( std::move( hidden_value ) ) {}

    object( int value ) 
        : name( "Int" )
        , content( value ) {};

    object( fun_obj_t fun ) 
        : name( "Fun" )
        , content( fun ) {}; 

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

    template < typename primitive_t >
    bool has_value () const 
    {
        return omega() && std::holds_alternative< primitive_t >( 
                std::get< value_t >( content ) );
    }

    template < typename primitive_t > 
    primitive_t get_value () const 
    {
        return std::get< primitive_t >( std::get< value_t >( content ) );
    }

    std::string value_to_string( const value_t &value ) const {
        return std::visit( [&]( auto &&p ){
            using T = std::decay_t< decltype( p ) >;
            if constexpr ( std::is_same_v< T, int > ) {
                return std::to_string( p );
            } else if constexpr( std::is_same_v< T, bool > ) {
                return std::to_string( p );
            } else if constexpr( std::is_same_v< T, function_object< std::string > > ) {
                return p.to_string();
            } else if constexpr( std::is_same_v< T, function_object< ast::node_ptr > > ) {
                return p.to_string();
            } else if constexpr( std::is_same_v< T, bool > ) {
                return std::to_string( p );
            } else {
                return "Unknown";
            }
        }, value );
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

