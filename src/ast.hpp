#pragma once

#include <variant>
#include <memory>
#include <vector>

#include "pattern.hpp"
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
using node_ptr = std::shared_ptr< ast_node >;

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
struct literal_pattern;
struct variable_pattern;
struct object_pattern;

using pattern = std::variant< ast::literal_pattern< int >
                            , ast::variable_pattern
                            , ast::object_pattern >;

template< typename value_t >
struct literal_pattern
{
    identifier_t name;
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

node_ptr clone( const ast_node& a );

}
