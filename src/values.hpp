#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "kocky.hpp" 
#include "pprint.hpp"
#pragma once
#include <variant> 
#include <memory>
#include <iostream>

#include "ast.hpp"
#include "pattern.hpp"

template< typename evaluable_t_ >
struct function_path 
{
    using evaluable_t = evaluable_t_;

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

template< typename evaluable_t_ >
struct function_object 
{
    using evaluable_t = evaluable_t_;
    using function_path_t = function_path< evaluable_t >;

    std::vector< function_path_t > paths;
    int arity_ = 0;

    function_object( std::vector< function_path_t > paths, int arity ) 
                   : paths( std::move( paths ) ) 
                   , arity_( arity ) {};

    bool operator ==( const function_object &o ) const { return true; };

    std::string to_string() const { return "<fun>"; };

    int arity() const { return arity_; };
};

template < typename types >
struct object {

    using attrs_t    = std::vector< object >;
    using obj_name_t = std::string;
    using value_t    = typename types::value_t;

    obj_name_t name; 
    std::variant< attrs_t, value_t > content;

    object() {}

    object( obj_name_t name, attrs_t attrs ) 
        : name( std::move( name ) )
        , content( std::move( attrs ) ) {}

    object( obj_name_t name, value_t hidden_value ) 
        : name( std::move( name ) )
        , content( std::move( hidden_value ) ) {}

    template< typename T >
    object( T value ) 
        : name( types::template type_name< T >() )
        , content( value ) {};

    bool operator==( object other ) const
    {
        return other.name == name && other.content == content;
    }

    bool omega() const
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
    primitive_t get_value() const 
    {
        return std::get< primitive_t >( std::get< value_t >( content ) );
    }
    
    value_t get_value() const 
    {
        return std::get< value_t >( content );
    }

    const attrs_t& get_attrs() const 
    {
        return std::get< attrs_t >( content );
    }

    int arity() const
    {
        return omega() ? 1 : get_attrs().size();
    }

    std::string value_to_string( const value_t &value ) const {
        std::stringstream ss;
        pprint::PrettyPrinter printer( ss );
        printer.print_inline( value );
        return ss.str();
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

    friend std::ostream& operator<<( std::ostream &out, const object &o ) 
    {
        return out << o.to_string();
    }
};

template< typename value_t >
using matching_t = std::map< identifier_t, object< value_t > >;

template< typename value_t >
bool match( const pattern& p, const object< value_t >& o, matching_t< value_t >& m )
{
    return std::visit( [&]( const auto &v ){ return match( v, o, m ); }, p );
}

template< typename value_t >
bool match( const variable_pattern& p, const object< value_t >& o, matching_t< value_t >& match )
{
    const auto &[ it, succ ] = match.insert( { p.variable_name, o } );
    if ( !succ )
        throw std::runtime_error( "multiple variables with same name not implemented" );
    return true;
}

template< typename value_t >
bool match( const object_pattern& p, const object< value_t >& o, matching_t< value_t >& m ) 
{
    if ( o.name != p.name ) {
        return P_DBG_RET( false, "names do not match" );
    }

    if ( o.omega() ) {
        if ( p.patterns.size() != 1 ) {
            return P_DBG_RET( false, "size does not match" );
        }
        return match( p.patterns[ 0 ], o, m );
    } 
    
    if ( o.get_attrs().size() != p.patterns.size() ) {
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
    for ( int i = 0; i < p.patterns.size(); i++ ) {
        if ( !match( p.patterns[ i ], o.get_attrs()[ i ], m ) ) 
            return false;
    }
    return true;
}

template< typename value_t >
std::optional< matching_t< value_t > > match ( const pattern& p, const object< value_t >& o )
{
    matching_t< value_t > matching;
    return match( p, o, matching ) 
        ? std::move( matching ) 
        : std::optional< matching_t< value_t > >();
}

template < typename value_t, typename T >
bool match( const literal_pattern< T >& p, const object< value_t >& o, matching_t< value_t >& match )
{
    if ( !o.omega() ) {
        return P_DBG_RET( false, "is not an omega object" );
    }
    if ( o.name != p.name ) {
        return P_DBG_RET( false, "names do not match" );
    }
    if ( o.template has_value< T >() && o.template get_value< T >() != p.value ) {
        return P_DBG_RET( false, "values do not match" );
    }
    return true;
}

template < typename value_t, typename evaluable_t >
std::optional< matching_t< value_t > > match
    ( const function_path< evaluable_t >& f_path
    , const std::vector< object< value_t > >& objects )
{
    if ( objects.size() != f_path.input_patterns.size() )
        return {};
    matching_t< value_t > matching; 
    for ( int i = 0; i < objects.size(); i ++ ) 
        if ( ! match( f_path.input_patterns[ i ], objects[ i ], matching ) )
            return {};
    return matching; 
}

template < typename value_t, typename evaluable_t >
std::optional< std::pair< matching_t< value_t >, evaluable_t > > match
    ( const function_object< evaluable_t >& funobj
    , const std::vector< object< value_t > >& objects )
{
    if ( objects.size() != funobj.arity() ) 
        return {};
    for ( const auto& f_path : funobj.paths ) {
        auto res = match( f_path, objects );
        if ( res.has_value() )
            return std::pair{ res.value(), f_path.evaluable };
    }
    return {};
}

void tests_values();
