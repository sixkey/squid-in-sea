#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <map>
#include <stack>

#include "kocky.hpp"
#include "pattern.hpp"
#include "pprint.hpp"
#include "types.hpp"
#include "values.hpp"
#include "scopestack.hpp"
#include "ast.hpp"

using namespace std::literals::string_literals;

template < typename eval_t >
struct literal
{
    using object_t = typename eval_t::object_t;

    object_t value;

    literal( object_t value ) : value( value ) {};

    void visit( eval_t& eval ) {
        eval.state.push_value( value );
    }

    friend std::ostream& operator<<( std::ostream& os, literal l )
    {
        return os << "Literal " << l.value;
    }
};

template < typename eval_t >
struct variable
{
    using object_t = typename eval_t::object_t;

    identifier_t id;

    variable( identifier_t id ) : id( id ) {};

    void visit( eval_t& eval ) {
        object_t value = eval.state._store.lookup( id );
        eval.state.push_value( std::move( value ) );
    }

    friend std::ostream& operator<<( std::ostream& os, variable v )
    {
        return os << "Variable " << v.id;
    }
};


///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

template < typename eval_t >
struct fun_cleanup
{
    using object_t  = typename eval_t::object_t;
    using fun_obj_t = typename eval_t::types::fun_obj_t;

    fun_obj_t fun;

    fun_cleanup( fun_obj_t fun ) : fun( fun ) {}

    void visit( eval_t& eval )
    {
        // TODO: check output pattern
        eval.state._store.scopes.pop_scope();
    }

    friend std::ostream& operator<<( std::ostream& os, fun_cleanup f )
    {
        return os << "FunCleanup";
    }
};

template < typename eval_t >
struct fun_call
{
    using evaluable_t = typename eval_t::types::evaluable_t;
    using value_t     = typename eval_t::types::value_t;
    using fun_obj_t   = typename eval_t::types::fun_obj_t;
    using object_t    = typename eval_t::object_t;

    fun_obj_t fun;
    int arity;

    fun_call( fun_obj_t fun, int arity )
            : fun( fun ), arity( arity ) {}

    void visit( eval_t& eval )
    {
        /** Move values from value stack to a bundle for function
         *  TODO: probably avoidable using an iterator. **/
        std::vector< object_t > values;
        for ( int i = 0; i < arity; i++ )
            values.push_back( eval.state.pop_value() );

        auto result = match( fun, values );
        if ( result.isleft() )
            throw std::runtime_error( "no pattern match: "s + result.left() );
        const auto &[ matching, evaluable ] = result.right();

        eval.state.push_cell( fun_cleanup< eval_t >( std::move( fun ) ) );
        eval.state._store.scopes.add_scope();
        eval.state._store.bind( matching );
        eval.evaluate( evaluable );
    }

    friend std::ostream& operator<<( std::ostream& os, fun_call f )
    {
        return os << "FunCall";
    }
};

template < typename eval_t >
struct fun_args
{
    using object_t     = typename eval_t::object_t;
    using fun_obj_t    = typename eval_t::types::fun_obj_t;
    using evaluable_t  = typename eval_t::types::evaluable_t;
    using ast_node_ptr = typename eval_t::ast_node_ptr;

    // arguments s.t. the last in the vector should be evaluated first
    std::vector< ast_node_ptr > args;

    fun_args( std::vector< ast_node_ptr > args ) : args( args ) {};

    void visit( eval_t& eval )
    {
        object_t obj = eval.state.pop_value();

        assert( obj.template has_value< fun_obj_t >() );

        fun_obj_t fun = obj.template get_value< fun_obj_t >();

        int arity = std::min< int >( fun.arity(), args.size() );

        int index = -1;

        if ( arity != args.size() ) {
            index = eval.state._cells.size();
            eval.state.push_cell( fun_args< eval_t >( {} ) );
        }

        eval.state.push_cell( fun_call< eval_t >( std::move( fun ), arity ) );
        for ( int i = 0; i < arity; i ++ ) {
            eval.push( args.back() );
            args.pop_back();
        }

        if ( index != -1 )
        {
            std::get< fun_args >( eval.state._cells[ index ] ).args = std::move( args );
        }
    }

    friend std::ostream& operator<<( std::ostream& os, fun_args f )
    {
        return os << "FunArgs " << f.args.size();
    }
};

template < typename eval_t >
struct fun_init
{
    using evaluable_t  = typename eval_t::types::evaluable_t;
    using fun_obj_t    = typename eval_t::types::fun_obj_t;
    using ast_node_ptr = typename eval_t::ast_node_ptr;

    ast_node_ptr fun_ast;

    // arguments s.t. the last in the vector should be evaluated first
    std::vector< ast_node_ptr > args;

    fun_init
        ( ast_node_ptr fun_ast
        , std::vector< ast_node_ptr > args )
        : fun_ast( fun_ast )
        , args( args ) {};


    void visit( eval_t& eval ) {
        eval.state.push_cell( fun_args< eval_t >( std::move( args ) ) );
        eval.push( fun_ast );
    }

    friend std::ostream& operator<<( std::ostream& os, const fun_init& f )
    {
        return os << "FunInit";
    }
};

template < typename eval_t >
struct scope_pop
{
    void visit( eval_t& eval ) {
        eval.state._store.scopes.pop_scope();
    }
};

template < typename eval_t >
struct let_in_bind
{
    using ast_node_ptr = typename eval_t::ast_node_ptr;
    using object_t = typename eval_t::object_t;

    pattern pat;
    ast_node_ptr expression;

    let_in_bind
        ( pattern pat
        , ast_node_ptr expression )
        : pat( std::move( pat ) )
        , expression( std::move( expression ) ) {}

    void visit( eval_t& eval ) {
        object_t value = eval.state.pop_value();
        eval.state._store.scopes.add_scope();

        const auto& matching = match( pat, value );
        if ( ! matching.has_value() )
            throw std::runtime_error( "variable does not match pattern" );
        eval.state._store.assign( matching.value() );
        eval.state.push_cell( scope_pop< eval_t >() );
        eval.push( expression );
    }
};

template < typename eval_t >
struct let_in_init
{
    using ast_node_ptr = typename eval_t::ast_node_ptr;

    pattern pat;
    ast_node_ptr value;
    ast_node_ptr expression;

    let_in_init
        ( pattern pat
        , ast_node_ptr value
        , ast_node_ptr expression )
        : pat( std::move( pat ) )
        , value( std::move( value ) )
        , expression( std::move( expression ) ) {}

    void visit( eval_t& eval ) {
        eval.state.push_cell( let_in_bind< eval_t >( std::move( pat ), std::move( expression )) );
        eval.push( value );
    }
};

///////////////////////////////////////////////////////////////////////////////
// Translators
///////////////////////////////////////////////////////////////////////////////

template < typename ast_node_t, typename eval_t >
class eval_translator
{
    using eval_cell_t = typename eval_t::eval_cell_t;

public:
    static eval_cell_t translate( const ast_node_t& n, eval_t& eval );
};

template < typename eval_t >
class eval_translator< ast::ast_node, eval_t >
{

    using eval_cell_t = typename eval_t::eval_cell_t;
    using ast_node_t = ast::ast_node;
    using object_t = typename eval_t::object_t;
    using evaluable_t = typename eval_t::types::evaluable_t;

public:

    static eval_cell_t translate( const ast_node_t& n, eval_t& eval )
    {
        return std::visit( [&]( auto &p ){ return accept( p, eval ); }, n );
    }

    template < typename T >
    static eval_cell_t accept( const ast::literal< T > &l, eval_t& eval )
    {
        return literal< eval_t >( l.value );
    }

    static eval_cell_t accept( const ast::variable& v, eval_t& eval )
    {
        return variable< eval_t >( v.name );
    }

    static eval_cell_t accept( const ast::function_call& f, eval_t& eval )
    {
        auto arguments = f.args;
        std::reverse( arguments.begin(), arguments.end() );
        return fun_init< eval_t >( f.fun, arguments );
    }

    template< typename T >
    static literal_pattern< T > tran_pattern( const ast::literal_pattern< T >& p )
    {
        return literal_pattern< T >( eval_t::types::template type_name<T>(), p.value );
    }

    static variable_pattern tran_pattern( const ast::variable_pattern& p )
    {
        return variable_pattern( p.name );
    }

    static object_pattern tran_pattern( const ast::object_pattern& p )
    {
        std::vector< pattern > patterns;
        for ( const auto& child : p.patterns )
            patterns.push_back( eval_translator::tran_pattern( child ) );
        return object_pattern( p.name, std::move( patterns ) );
    }

    static pattern tran_pattern( const ast::pattern& p )
    {
        return std::visit( [&]( const auto& v ){
            return pattern{ eval_translator::tran_pattern( v ) };
        }, p );
    }

    static function_path< evaluable_t > translate_path( const ast::function_path& p
                                                      , eval_t& eval )
    {
        std::vector< pattern > input_patterns;
        for ( const auto& pattern : p.input_patterns )
            input_patterns.push_back( tran_pattern( pattern ) );
        const auto& output_pattern = tran_pattern( p.output_pattern );

        using closure_t = typename eval_t::types::closure_t;

        auto free_variables = ast::ast_free_vars::free_variables( p );
        auto bindings = eval.state._store.scopes.lookup( free_variables );

        if ( bindings.isleft() )
            throw std::runtime_error( "variable '" + bindings.left() + "' not bound" );

        closure_t closure = { p.expression, bindings.right() };

        return function_path< evaluable_t >(
                input_patterns,
                output_pattern,
                evaluable_t{ closure } );
    }

    static function_object< evaluable_t > translate_fun( const ast::function_def &f
                                                       , eval_t& eval )
    {
        std::vector< function_path< evaluable_t > > paths;
        for ( const auto& f_path : f.paths )
            paths.push_back( translate_path( *f_path, eval ) );
        return { paths, f.arity };
    }

    static eval_cell_t accept( const ast::function_def& f, eval_t& eval )
    {
        return literal< eval_t >( object_t( translate_fun( f, eval ) ) );
    }

    static eval_cell_t accept( const ast::let_in& l, eval_t& eval )
    {
        return let_in_init< eval_t >( tran_pattern( l.pat ), l.value, l.expression );
    }

};

template < typename eval_t >
class eval_translator< ast::node_ptr, eval_t >
{

    using eval_cell_t = typename eval_t::eval_cell_t;
    using ast_node_t = ast::node_ptr;
    using object_t = typename eval_t::object_t;

public:

    static eval_cell_t translate( const ast_node_t& n, eval_t& eval )
    {
        return eval_translator< ast::ast_node, eval_t >::translate( *n, eval );
    }
};

///////////////////////////////////////////////////////////////////////////////
// State
///////////////////////////////////////////////////////////////////////////////

template < typename object_t >
struct store
{
    using store_id = int;
    using bindings_tree_t = bindings_tree< identifier_t, store_id >;
    using bindings_t = bindings_tree_t::bindings_t;

    bindings_tree_t scopes;
    std::vector< object_t > _store;


    void bind( identifier_t name, store_id id )
    {
        scopes.bind( std::move( name ), id );
    }

    void bind( const identifier_t& name, object_t value )
    {
        bind( name, _store.size() );
        _store.push_back( std::move( value ) );
    }

    template < typename assigned_t >
    void bind( const std::map< identifier_t, assigned_t >& assignment ) {
        for ( const auto &[ key, val ] : assignment )
            bind( key, val );
    }

    void assign( identifier_t name, object_t value )
    {
        std::optional< store_id > id = scopes.lookup( name );
        if ( id.has_value() ) {
            _store[ id.value() ] = std::move( value );
        } else {
            bind( std::move( name ), std::move( value ) );
        }
    }

    void assign( identifier_t name, store_id id )
    {
        bind( name, id );
    }

    template < typename assigned_t >
    void assign( const std::map< identifier_t, assigned_t >& assignment ) {
        for ( const auto &[ key, val ] : assignment )
            assign( key, val );
    }

    object_t lookup( identifier_t name )
    {
        std::optional< store_id > id = scopes.lookup( name );
        if ( !id.has_value() ) {
            throw std::runtime_error( "variable '"s + name + "' not bound" );
        }
        return _store[ id.value() ];
    }

    friend std::ostream& operator<<( std::ostream& os, const store& s )
    {
        pprint::PrettyPrinter printer( os );
        printer.print( "Scope" );
        os << s.scopes;
        printer.print( "Store" );
        printer.print( s._store );
        return os;
    }

    std::map< identifier_t, object_t > dereference( 
        const std::map< identifier_t, store_id >& bindings )
    {
        std::map< identifier_t, object_t > result;
        for ( const auto &[ key, value ] : bindings )
            result.insert( { key, _store[ value ] } );
        return result;
    }

    std::vector< std::map< identifier_t, object_t > > dereference( 
        const std::vector< std::map< identifier_t, store_id > > bindings )
    {
        std::vector< std::map< identifier_t, object_t > > result;
        for ( const auto &binding : bindings )
            result.push_back( std::move( dereference( binding ) ) );
        return result;
    }

};

template < typename object_t, typename eval_cell_t >
struct eval_state
{

    std::stack< object_t > _values;
    std::vector< eval_cell_t > _cells;
    using store_t = store< object_t >;
    store_t _store;

    void push_cell( const eval_cell_t c )
    {
        _cells.push_back( c );
    }

    eval_cell_t pop_cell()
    {
        assert( !_cells.empty() );
        eval_cell_t res = std::move( _cells.back() );
        _cells.pop_back();
        return std::move( res );
    }

    eval_cell_t& cells_top()
    {
        assert( !_cells.empty() );
        return _cells.back();
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

///////////////////////////////////////////////////////////////////////////////
// Cells
///////////////////////////////////////////////////////////////////////////////

template < typename eval_t >
using eval_cell = std::variant< literal< eval_t >
                              , variable< eval_t >
                              , fun_call< eval_t >
                              , fun_args< eval_t >
                              , fun_init< eval_t >
                              , fun_cleanup< eval_t >
                              , let_in_init< eval_t >
                              , let_in_bind< eval_t >
                              , scope_pop< eval_t >
                              >;

template < typename eval_t >
using builtin = std::function< void( eval_t& ) >;


///////////////////////////////////////////////////////////////////////////////
// Types
///////////////////////////////////////////////////////////////////////////////

template < typename evaluable_t, typename identifier_t, typename store_id >
struct closure
{
    evaluable_t evaluable;
    std::map< identifier_t, store_id > bindings;
};

template < typename eval_t >
struct types_
{
    using builtin_t = builtin< eval_t >;
    using closure_t = closure< ast::node_ptr, identifier_t, int >;
    using evaluable_t = std::variant< closure_t, builtin_t >;
    using fun_obj_t = function_object< evaluable_t >;
    using value_t = std::variant< int
                                , bool
                                , fun_obj_t >;
    template< typename T >
    static constexpr const char * type_name() {
        if constexpr ( std::is_same< T, int >::value )
            return "Int";
        else if constexpr ( std::is_same< T, bool >::value )
            return "Bool";
        else if constexpr ( std::is_same< T, fun_obj_t >::value )
            return "Fun";
        else
            assert( false );
    }
};

///////////////////////////////////////////////////////////////////////////////
// Evaluator
///////////////////////////////////////////////////////////////////////////////

struct eval
{
    using types = types_< eval >;
    using object_t = object< types >;
    using eval_cell_t = eval_cell< eval >;
    using ast_node_ptr = ast::node_ptr;

    using eval_state_t = eval_state< object_t, eval_cell_t >;

    eval_state_t state;

    eval_translator < ast::ast_node, eval > translator;

    bool debug_mode = false;

    using bindings_t = eval_state_t::store_t::bindings_t;

    void run()
    {
        pprint::PrettyPrinter printer;
        while( !state._cells.empty() )
        {
            if ( debug_mode )  {
                std::cin.get();
                printer.print( "step" );
                printer.print( "cells" );
                printer.print( state._cells );
                printer.print( "values" );
                printer.print( state._values );
                // printer.print( state._store.dereference( state._store.scopes.head->bindings ) );
            }
            run_top();
        }
    }

    void run_top()
    {
        eval_cell_t top_cell = state.pop_cell();
        std::visit( [&]( auto &v ){ v.visit( *this ); }, top_cell );
    }

    void evaluate( types::evaluable_t ev )
    {
        std::visit( ON_THIS( evaluate ), ev );
    }

    /** Adds the content of the closure to the current scope meaning the wrapper
     *  around the closure needs to make sure it is fine. **/
    void evaluate( types::closure_t c )
    {
        state._store.assign( c.bindings );
        push( c.evaluable );
    }

    void evaluate( types::builtin_t f )
    {
        f( *this );
    }

    void push( const eval_cell_t& cell )
    {
        state.push_cell( cell );
    }

    template < typename T >
    void push( const T& something )
    {
        state.push_cell( eval_translator< T, eval >::translate( something, *this ) );
    }

};
