#pragma once

#include <variant>
#include <memory>
#include <vector>

#include "pattern.hpp"
#include "pprint.hpp"
#include "types.hpp"

namespace ast {

template< typename value_t >
struct literal;
struct variable;
struct function_call;
struct function_def;

using ast_node = std::variant< variable
                             , function_call
                             , function_def
                             , literal< int >
                             , literal< bool > >;

template< typename value_t >
struct literal_pattern;
struct variable_pattern;
struct object_pattern;

using pattern = std::variant< ast::literal_pattern< int >
                            , ast::literal_pattern< bool >
                            , ast::variable_pattern
                            , ast::object_pattern >;

using node_ptr = std::shared_ptr< ast_node >;

struct ast_printer;

template< typename value_t >
struct literal
{
    value_t value; 
    literal( value_t value ) : value( value ) {};
};

struct variable 
{
    identifier_t name;
    variable ( identifier_t name ) : name( name ) {};
};

struct function_call 
{
    node_ptr fun;
    std::vector< node_ptr > args;
    function_call
        ( node_ptr fun
        , std::vector< node_ptr > arguments )
        : fun( fun )
        , args( arguments ) 
    {}
};

template< typename value_t >
struct literal_pattern
{
    value_t value;
};

struct variable_pattern
{
    identifier_t name;
};

struct object_pattern
{
    identifier_t name;
    std::vector< pattern > patterns;
};

struct function_path
{
    std::vector< pattern > input_patterns;
    pattern output_pattern;
    node_ptr expression;
};

struct function_def
{
    std::vector< function_path > paths;
    int arity;
};

///////////////////////////////////////////////////////////////////////////////
// Print visitor
///////////////////////////////////////////////////////////////////////////////

struct ast_printer 
{
    pprint::PrettyPrinter printer;

    size_t offset;

    void indent()
    {
        printer.indent_offset( offset += 2 );
    }

    void dedent()
    {
        if ( offset > 2 )
            offset -= 2;
        printer.indent_offset( offset );
    }

    void accept( const ast::ast_node& n )
    {
        std::visit( ON_THIS( accept ), n );
    }
    
    template< typename value_t >
    void accept( const literal< value_t >& l )
    {
        printer.print( "Literal", l.value );
    }

    void accept( const variable& v ) 
    {
        printer.print( "Variable", v.name );
    }

    void accept( const function_call& f )
    {
        printer.print( "FunctionCall" );
        indent();
        
        auto accept_on_this = ON_THIS( accept );

        accept( *f.fun );
        for ( const auto& c : f.args )
            accept( *c );
        dedent();
    }

    template< typename value_t > 
    void accept( const literal_pattern< value_t >& l )
    {
        printer.print( "LiteralPattern", l.value );
    }

    void accept( const variable_pattern& v )
    {
        printer.print( "VariablePattern", v.name );
    }

    void accept( const object_pattern& o )
    {
        printer.print( "ObjectPattern", o.name );
        indent();
        for ( const auto& c : o.patterns )
            accept( c );
        dedent();
    }

    void accept( const pattern& p )
    {
        std::visit( ON_THIS( accept ), p );
    }

    void accept( const function_path& p )
    {
        printer.print( "FunctionPath" );
        indent();
        printer.print( "Inputs" );
        indent();
        for ( const auto& pat : p.input_patterns )
            accept( pat );
        dedent();
        accept( p.output_pattern );
        accept( *p.expression );
        dedent();
    }

    void accept( const function_def& d )
    {
        printer.print( "FunctionDef" );
        indent();
        for ( const auto& p : d.paths )
            accept( p );
        dedent();
    }
};


node_ptr clone( const ast_node& a );

}
