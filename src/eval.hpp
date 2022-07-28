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
        eval.state.pop_value();
    }

    friend std::ostream& operator<<( std::ostream& os, variable v )
    {
        return os << "Variable " << v.id;
    }
};

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

template < typename eval_t, typename evaluable_t > 
struct fun_cleanup
{
    using object_t = typename eval_t::object_t;
    using fun_obj_t = function_object< evaluable_t >;

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

template < typename eval_t, typename evaluable_t >
struct fun_call 
{
    using object_t = typename eval_t::object_t;

    function_object< evaluable_t > fun;
    int arity;

    fun_call( function_object< evaluable_t > fun, int arity ) 
            : fun( fun ), arity( arity ) {}
    
    void call( eval_t& eval, const std::vector< object_t >& values ) 
    {
    }

    void visit( eval_t& eval ) 
    {
        /** Move values from value stack to a bundle for function 
         *  TODO: probably avoidable using an iterator. **/
        std::vector< object_t > values;
        for ( int i = 0; i < arity; i++ )
            values.push_back( eval.state.pop_value() );

        auto result = match< evaluable_t >( fun, values );
        if ( !result.has_value() )
            throw std::runtime_error( "no pattern match" );
        const auto &[ matching, evaluable ] = result.value();

        eval.state.push_cell( fun_cleanup< eval_t, evaluable_t >( std::move( fun ) ) );
        eval.state._store.scopes.add_scope();
        eval.state._store.assign( matching );
        eval.push( evaluable );
    }

    friend std::ostream& operator<<( std::ostream& os, fun_call f )
    {
        return os << "FunCall";
    }
};

template < typename eval_t, typename evaluable_t >
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
        assert( obj.template has_value< function_object< evaluable_t > >() );
        fun_obj_t fun = obj.template get_value< fun_obj_t >();

        int arity = std::max< int >( fun.arity(), args.size() );

        int index = -1;

        if ( arity != args.size() ) {
            index = eval.state._cells.size();
            eval.state.push_cell( fun_args< eval_t, evaluable_t >( {} ) );
        }

        eval.state.push_cell( fun_call< eval_t, evaluable_t >( std::move( fun ), arity ) );
        for ( int i = 0; i < arity; i ++ ) {
            eval.state.push_cell( eval.translator.translate( *args.back() ) );
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

template < typename eval_t, typename evaluable_t >
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
        eval.state.push_cell( fun_args< eval_t, evaluable_t >( std::move( args ) ) );
        eval.state.push_cell( eval.translator.translate( *fun_ast ) );
    }

    friend std::ostream& operator<<( std::ostream& os, const fun_init& f )
    {
        return os << "FunInit";
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
    static eval_cell_t translate( const ast_node_t& n );
};

template < typename eval_t >
class eval_translator< ast::ast_node, eval_t >
{

    using eval_cell_t = typename eval_t::eval_cell_t;
    using ast_node_t = ast::ast_node;
    using object_t = typename eval_t::object_t;

public:

    static eval_cell_t translate( const ast_node_t& n ) 
    {
        return std::visit( [&]( auto &p ){ return accept( p ); }, n );
    }

    template < typename T >
    static eval_cell_t accept( const ast::literal< T > &l ) 
    {
        return literal< eval_t >( l.value ); 
    }

    static eval_cell_t accept( const ast::variable& v ) 
    {
        return variable< eval_t >( v.name );
    }

    static eval_cell_t accept( const ast::function_call& f ) 
    {
        return fun_init< eval_t, ast::node_ptr >( f.fun, f.args );
    }

    template< typename T >
    static literal_pattern< T > tran_pattern( const ast::literal_pattern< T >& p )
    {
        return literal_pattern< T >( p.name, p.value );
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
        return std::visit( [&]( const auto& v ){ return pattern{ eval_translator::tran_pattern( v ) }; }, p );
    }

    static function_path< ast::node_ptr > translate_path( const ast::function_path& p )
    {
        std::vector< pattern > input_patterns;
        for ( const auto& pattern : p.input_patterns ) 
            input_patterns.push_back( tran_pattern( pattern ) );
        const auto& output_pattern = tran_pattern( p.output_pattern );
        return function_path< ast::node_ptr >( input_patterns, output_pattern, p.expression );
    }

    static function_object< ast::node_ptr > translate_fun( const ast::function_def &f )
    {
        std::vector< function_path< ast::node_ptr > > paths;
        for ( const auto& f_path : f.paths )
            paths.push_back( translate_path( f_path ) );
        return function_object< ast::node_ptr >( paths, f.arity );
    }

    static eval_cell_t accept( const ast::function_def& f )
    {
        return literal< eval_t >( object_t( translate_fun( f ) ) );
    }

};

template < typename eval_t >
class eval_translator< ast::node_ptr, eval_t >
{

    using eval_cell_t = typename eval_t::eval_cell_t;
    using ast_node_t = ast::node_ptr;
    using object_t = typename eval_t::object_t;

public:

    static eval_cell_t translate( const ast_node_t& n ) 
    {
        return eval_translator< ast::ast_node, eval_t >::translate( *n );
    }
};

///////////////////////////////////////////////////////////////////////////////
// State
///////////////////////////////////////////////////////////////////////////////

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
            _store[ id.value() ] = std::move( value );
        } else {
            bind( std::move( name ), std::move( value ) );
        }
    }

    void assign( const std::map< identifier_t, object_t >& assignment ) {
        for ( const auto &[ key, val ] : assignment )
            assign( key, val );
    }
    
    object_t lookup( identifier_t name )
    {
        std::optional< store_id > id = scopes.lookup( name );
        if ( !id.has_value() ) {
            throw std::runtime_error( "value not present" );
        }
        return _store[ id.value() ];
    }

    friend std::ostream& operator<<( std::ostream& os, const store& s )
    {
        pprint::PrettyPrinter printer( os );
        printer.print( "Scope" );
        printer.print( s.scopes );
        printer.print( "Store" );
        printer.print( s._store );
        return os;
    }

};

template < typename object_t, typename eval_cell_t >
struct eval_state 
{

    std::stack< object_t > _values;
    std::vector< eval_cell_t > _cells;
    store< object_t > _store;

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
// Scopes
///////////////////////////////////////////////////////////////////////////////

template < typename eval_t >
using misc_cells = std::variant< literal< eval_t > 
                               , variable< eval_t > >;

template < typename eval_t, typename evaluable_t  > 
using function_cells = std::variant< fun_call< eval_t, evaluable_t >
                                   , fun_args< eval_t, evaluable_t > 
                                   , fun_init< eval_t, evaluable_t >
                                   , fun_cleanup< eval_t, evaluable_t > >;

template < typename eval_t >
using eval_cell = typename kck::concat< misc_cells< eval_t >, 
                                        function_cells< eval_t, ast::node_ptr > >::type;

template < typename object_t_ >
struct eval 
{
    using ast_node_t   = ast::ast_node;
    using ast_node_ptr = ast::node_ptr;
    using object_t     = object_t_;
    using eval_cell_t  = eval_cell< eval >;

    eval_state      < object_t,   eval_cell_t >  state;
    eval_translator < ast_node_t, eval >  translator;

    void evaluate()
    {
        pprint::PrettyPrinter printer;
        while( !state._cells.empty() )
        {
            eval_top();
            printer.print( "STEP" );
            printer.print( state._cells );
            printer.print( state._values );
            printer.print( state._store );
        }
    }

    void eval_top()
    {
        eval_cell_t top_cell = state.pop_cell();
        std::visit( [&]( auto &v ){ v.visit( *this ); }, top_cell );
    }

    void push( const eval_cell_t& cell )
    {
        state.push_cell( cell );
    }

    template < typename T > 
    void push( const T& something )
    {
        state.push_cell( eval_translator< T, eval >::translate( something ) );
    }

};

void tests_eval();
