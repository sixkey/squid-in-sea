#pragma once

#include <variant>
#include <memory>
#include <vector>

#include "types.hpp"

namespace ast {

template< typename value_t >
struct literal;
struct variable;
struct function_call;

using ast_node = std::variant< literal< int >, variable, function_call >;
using node_ptr = std::shared_ptr< ast_node >;

template< typename value_t >
struct literal
{
    value_t value; 
    literal( value_t value ) : value( value ) {};
};

struct variable 
{
    identifier name;
    variable ( identifier name ) : name( name ) {};
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

}
