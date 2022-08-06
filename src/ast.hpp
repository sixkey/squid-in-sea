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
    struct let_in;

    using ast_node = std::variant< variable
                                 , function_call
                                 , function_def
                                 , literal< int >
                                 , literal< bool >
                                 , let_in >;

    using node_ptr = std::shared_ptr< ast_node >;

    template< typename value_t >
    struct literal_pattern;
    struct variable_pattern;
    struct object_pattern;

    using pattern = std::variant< ast::literal_pattern< int >
                                , ast::literal_pattern< bool >
                                , ast::variable_pattern
                                , ast::object_pattern >;

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

    struct let_in
    {
        identifier_t name;
        node_ptr value;
        node_ptr expression;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Free variables
    ///////////////////////////////////////////////////////////////////////////

    struct ast_free_vars
    {
        using id_set_t = std::set< identifier_t >;

        struct blacklist_t {
            std::vector< std::set< identifier_t > > layers;

            bool contains( const identifier_t& i )
            {
                for ( const auto& layer : layers )
                    if ( layer.find( i ) != layer.end() )
                        return true;
                return false;
            }
        };

        template < typename node_t >
        static id_set_t free_variables( const node_t& n )
        {
            id_set_t vars      = {};
            blacklist_t blacklist = {};
            _free_variables( n, vars, blacklist );
            return vars;
        }

        static void _free_variables( const ast::ast_node& node
                                   , id_set_t& vars
                                   , blacklist_t& blacklist ) {
            std::visit( [&]( const auto& v ) {
                             _free_variables( v, vars, blacklist );
                        }
                      , node );
        }

        template < typename value_t >
        static void _free_variables( const literal< value_t >& literal
                                       , id_set_t& vars
                                       , blacklist_t& blacklist ) {}

        static void _free_variables( const variable& var
                                  , id_set_t& vars
                                  , blacklist_t& blacklist )
        {
            if ( ! blacklist.contains( var.name ) )
                vars.insert( var.name );
        }

        static void _free_variables( const function_call& call
                                   , id_set_t& vars
                                   , blacklist_t& blacklist )
        {
            _free_variables( *call.fun, vars, blacklist );
            for ( const auto& a : call.args )
                _free_variables( *a, vars, blacklist );
        }

        static id_set_t variables_in( const pattern& pattern )
        {
            id_set_t variables;
            _variables_in( pattern, variables );
            return variables;
        }

        static void _variables_in( const pattern& pattern, id_set_t& variables )
        {
            std::visit( [&]( const auto& p ){ _variables_in( p, variables ); }, pattern );
        }

        template < typename value_t >
        static void _variables_in( const literal_pattern< value_t >& pattern
                                 , id_set_t& variables )
        {}

        static void _variables_in( const variable_pattern& pattern
                                 , id_set_t& variables )
        {
            variables.insert( pattern.name );
        }

        static void _variables_in( const object_pattern& object
                                 , id_set_t& variables )
        {
            for ( const auto& child : object.patterns )
                _variables_in( child, variables );
        }

        static void _free_variables( const function_path& fun_path
                                   , id_set_t& vars
                                   , blacklist_t& blacklist )
        {
            id_set_t bound_variables;
            for ( const auto& i_pattern : fun_path.input_patterns )
                _variables_in( i_pattern, bound_variables );
            blacklist.layers.push_back( std::move( bound_variables ) );
            _free_variables( *fun_path.expression, vars, blacklist );
            blacklist.layers.pop_back();
        }

        static void _free_variables( const function_def& def
                                   , id_set_t& vars
                                   , blacklist_t& blacklist )
        {
            for ( const auto& a : def.paths )
                _free_variables( a, vars, blacklist );
        }

        static void _free_variables( const let_in& letin
                                   , id_set_t& vars
                                   , blacklist_t& blacklist )
        {
            _free_variables( *letin.value, vars, blacklist );
            blacklist.layers.push_back( { letin.name } );
            _free_variables( *letin.expression, vars, blacklist );
            blacklist.layers.pop_back();
        }


    };



    ///////////////////////////////////////////////////////////////////////////////
    // Print visitor
    ///////////////////////////////////////////////////////////////////////////////

    struct ast_printer
    {
        pprint::PrettyPrinter printer;

        size_t offset;

        ast_printer( pprint::PrettyPrinter printer ) : printer( printer ) {};

        void indent() {}

        void dedent() {}

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

        void accept( const let_in& l )
        {
            printer.print( "LetIn", l.name );
            indent();
            accept( *l.value );
            accept( *l.expression );
            dedent();
        }
    };

    node_ptr clone( const ast_node& a );
}
