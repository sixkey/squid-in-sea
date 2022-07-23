#include "pattern_graph.hpp"
#include <variant>
namespace ast {

class node;
using node_ptr = std::shared_ptr< node >;

class node 
{
    public: 
    virtual ~node() = default;
};

class thunk
{
     
};

class thunk_processor 
{
};

class number_literal : public node 
{
    int _value; 

    public:
    number_literal( int value ) : _value( value ) { };
};

class number_thunk : public thunk
{
};

class function_literal : public node
{
    std::vector< pattern_ptr > _input_patterns;
    pattern_ptr _output_pattern;
    node_ptr _expression; 

    public: 
    function_literal
        ( std::vector< pattern_ptr > input_patterns 
        , pattern_ptr output_pattern
        , node_ptr expression ) 
        : _input_patterns( std::move( input_patterns ) )
        , _output_pattern( output_pattern )
        , _expression( expression ) 
    {}
};

class variable : public node
{
    identifier _name;
    
    public:
    variable ( identifier name ) : _name( name ) { };
};

class function_call : public node 
{
    identifier _name;
    std::vector< node_ptr > _arguments;

    public:
    function_call
        ( identifier name
        , std::vector< node_ptr > arguments )
        : _name( name )
        , _arguments( arguments ) 
    {}
};

class declaration : public node 
{
    identifier _name;
    node_ptr _expression; 

    public:
    declaration
        ( identifier name 
        , node_ptr expression )
        : _name( std::move( name ) )
        , _expression( std::move( expression ) ) 
    {}
};

}
