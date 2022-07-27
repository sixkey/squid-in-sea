#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <map>
#include <stack>

#include "kocky.hpp"
#include "values.hpp"
#include "scopestack.hpp" 
#include "ast.hpp"

template < typename eval_t >
struct literal 
{
    using object_t = typename eval_t::object_t;

    object_t value;

    literal( object_t value ) : value( value ) {};

    void visit( eval_t& eval ) {
        eval.state.push_value( value );
        eval.state.pop_cell();
    }
};

template < typename eval_t >
struct variable 
{
    using object_t = typename eval_t::object_t;

    identifier_t id;  

    variable( identifier_t id ) : id( id ) {};

    void visit( eval_t& eval ) {
        object_t value = eval.state.store.lookup( id );
        eval.state.push_value( std::move( value ) );
        eval.state.pop_value();
        eval.state.pop_cell();
    }
};

template < typename eval_t >
struct fun_call 
{
    using object_t = typename eval_t::object_t;
    using fun_obj_t = typename object_t::fun_obj_t;

    fun_obj_t fun;
    int arity;

    fun_call( fun_obj_t fun, int arity ) : fun( fun ), arity( arity ) {}

    object_t call( eval_t& eval, const std::vector< object_t >& values ) 
    {
        NOT_IMPLEMENTED();
    }

    void visit( eval_t& eval ) 
    {
        std::vector< object_t > values;
        for ( int i = 0; i < arity; i++ )
            values.push_back( eval.state.pop_value() );

        object_t value = call( eval, std::move( values ) );

        values.push_back( value );
        eval.state.pop_cell();
    }
};

template < typename eval_t >
struct fun_args 
{
    using object_t   = typename eval_t::object_t;
    using ast_node_ptr = typename eval_t::ast_node_ptr;
    using fun_obj_t  = typename object_t::fun_obj_t;

    std::vector< ast_node_ptr > args;

    fun_args( std::vector< ast_node_ptr > args ) : args( args ) {};

    void visit( eval_t& eval )
    {
        object_t obj = eval.state.pop_value();
        assert( obj.template has_value< fun_obj_t>() );
        fun_obj_t fun = obj.template get_value< fun_obj_t >();

        int arity = std::max< int >( fun.arity(), args.size() );

        eval.state.push_cell( fun_call< eval_t >( std::move( fun ), arity ) );

        for ( int i = 0; i < arity; i ++ ) {
            args.pop_back();
            eval.state.push_cell( eval.translator.translate( *args.back() ) );
        }

        /** If not, there are still arguments to be processed
         *  this cell will wait for another function object **/
        if ( args.size() == 0 )
            eval.state.pop_cell();
    }
};

template < typename eval_t >
struct fun_init 
{
    using ast_node_ptr = typename eval_t::ast_node_ptr;

    ast_node_ptr fun_ast;
    std::vector< ast_node_ptr > args;

    fun_init
        ( ast_node_ptr fun_ast
        , std::vector< ast_node_ptr > args )
        : fun_ast( fun_ast )
        , args( args ) {};


    void visit( eval_t& eval ) {
        eval.state.push_cell( fun_args< eval_t >( std::move( args ) ) );
        eval.state.push_cell( eval.translator.translate( *fun_ast ) );
    }
};


template < typename ast_node_t, typename eval_t >
class eval_translator
{
    using eval_cell_t = typename eval_t::eval_cell_t;

public:
    eval_cell_t translate( const ast_node_t& n ) const
    {
        NOT_IMPLEMENTED();
    }
};

template < typename eval_t >
using eval_cell = std::variant< 
    literal< eval_t >,
    variable< eval_t >,
    fun_call< eval_t >,
    fun_args< eval_t >, 
    fun_init< eval_t > >;

template < typename eval_t >
class eval_translator< ast::ast_node, eval_t >
{
    using eval_cell_t = typename eval_t::eval_cell_t;
    using ast_node_t = ast::ast_node;
    using object_t = typename eval_t::object_t;

public:

    eval_cell_t translate( const ast_node_t& n ) const
    {
        return std::visit( [&]( auto &p ){ return this->accept( p ); }, n );
    }

    template < typename T >
    eval_cell_t accept( const ast::literal< T > &l ) const
    {
        return literal< eval_t >( l.value ); 
    }

    eval_cell_t accept( const ast::variable& v ) const
    {
        return variable< eval_t >( v.name );
    }

    eval_cell_t accept( const ast::function_call& f ) const
    {
        return fun_init< eval_t >( f.fun, f.args );
    }

};

template < typename object_t > 
struct store 
{
    using store_id = int;

    scopestack< identifier_t, store_id > scopes;
    std::vector< object_t > _store;

    void bind( identifier_t name, object_t value )
    {
        scopes.bind( std::move( name ), _store.size() );
        _store.push_back( std::move( value ) );
    }

    void assign( identifier_t name, object_t value )
    {
        std::optional< store_id > id = scopes.lookup( name );
        if ( id.has_value() ) {
            _store[ id ] = std::move( value );
        } else {
            bind( name, value );
        }
    }
    
    object_t lookup( identifier_t name )
    {
        std::optional< store_id > id = scopes.lookup( name );
        if ( !id.has_value() ) {
            throw std::runtime_error( "value not present" );
        }
        return _store[ id.value() ];
    }

};

template < typename object_t, typename eval_cell_t >
struct eval_state 
{

    std::stack< object_t > _values;
    std::stack< eval_cell_t > _cells;
    store< object_t > store;

    void push_cell( const eval_cell_t c )
    {
        _cells.push( c );
    }

    eval_cell_t pop_cell()
    {
        assert( !_cells.empty() );
        eval_cell_t res = std::move( _cells.top() );
        _cells.pop();
        return std::move( res );
    }
    
    void push_value( object_t o )
    {
        _values.push( std::move( o ) ); 
    }

    object_t pop_value()
    {
        assert( !_values.empty() );
        object_t res = std::move( _values.top() );
        _values.pop();
        return std::move( res );
    }

};

template < typename object_t_ >
struct eval 
{
    using ast_node_t   = ast::ast_node;
    using ast_node_ptr = ast::node_ptr;
    using object_t     = object_t_;
    using eval_cell_t  = eval_cell< eval >;

    eval_state      < object_t,   eval_cell_t >  state;
    eval_translator < ast_node_t, eval >  translator;

    void eval_top()
    {
        std::visit( [&]( auto &v ){ v.visit( *this ); }, state._cells.top() );
    }
    void eval_c( const eval_cell_t cell )
    {
        state.push_cell( cell );
        eval_top();
    }

    void eval_a( const ast_node_t node )
    {
        eval_c( translator.translate( node ) );
    }
};

void tests_eval();
