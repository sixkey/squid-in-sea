#include "kocky.hpp"
#include "pprint.hpp"
#include <cstdio>
#include <exception>
#include <istream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include <functional> 
#include <cctype>

#include "ast.hpp"

using namespace std::literals::string_literals;

///////////////////////////////////////////////////////////////////////////////
// General parsing state
///////////////////////////////////////////////////////////////////////////////

struct parsing_error : public std::exception {

    std::string message;

    parsing_error( std::string message ) : message( message ) {};

    char * what()
    {
        return const_cast< char* >( message.c_str() );
    }

};

template < typename stream_t >
struct istream_generator
{
    using value_t = int;

    stream_t stream;

    istream_generator( stream_t stream ) : stream( std::move( stream ) ) {};

    int next()
    {
        int value = stream.get();
        return value;
    }

    int empty()
    {
        return stream.eof();
    }
};

struct string_generator 
{
    using value_t = int;

    std::string content;
    int counter = 0;
    
    string_generator( std::string content ) : content( content ) {};

    int next()
    {
        assert( !empty() );
        return int( content[ counter++ ] );
    }
    
    int empty()
    {
        return counter >= content.size();
    }

};

template < typename generator_t, typename metadata_t >
struct parsing_state
{
    using value_t = typename generator_t::value_t;
    using fn_pred_t = std::function< int( value_t v ) >;
    using pred_t = int(value_t);
    using show_value_t = std::function< std::string( const value_t& ) >;

    bool loaded = 0;
    generator_t generator; 

    value_t current;

    metadata_t meta;
    value_t def; 
    show_value_t show_value;

    parsing_state( generator_t content
                 , metadata_t meta 
                 , value_t def
                 , show_value_t show_value ) 
                 : generator( std::move( content ) )
                 , def( def ) 
                 , meta( std::move( meta ) ) 
                 , show_value( show_value ) {}
                    
    void inc()
    {
        meta.on_inc( std::move( current ) );
        loaded = false;
    }

    void load()
    {
        current = generator.next();
        loaded = true;
    }

    bool empty()
    {
        return generator.empty() && !loaded;
    }

    value_t peek()
    {
        if ( empty() )
            return def;
        if ( !loaded )
            load();
        return current;
    }

    value_t pop()
    {
        value_t value = peek();
        inc();
        return value;
    }


    value_t req_peek()
    {
        if ( empty() )
            throw parsing_error( "required value, but is the end" );
        return peek();
    }

    value_t req_peek( const value_t& value )
    {
        value_t v = req_peek();
        if ( v != value )
            throw parsing_error( "expected '" + show_value( value ) 
                                 + "' but got '" 
                                 + show_value( v ) + "'" );
        return v;
    }

    value_t req_peek( fn_pred_t pred, std::string name = "in predicate" ) 
    {
        value_t v = req_peek();
        if ( ! pred( v ) ) 
            throw parsing_error( "value '" + show_value( v ) + "' is not " + name );
        return v;
    }

    value_t req_peek( pred_t pred, std::string name = "in predicate" ) 
    {
        value_t v = req_peek();
        if ( ! pred( v ) ) 
            throw parsing_error( "value '" + show_value( v ) + "' is not " + name );
        return v;
    }

    value_t req_pop()
    {
        value_t v = req_peek();
        inc();
        return v;
    }

    value_t req_pop( const value_t& exp )
    {
        value_t v = req_peek( exp );
        inc();
        return v;
    }

    value_t req_pop( fn_pred_t pred, std::string name = "in predicate" )
    {
        value_t v = req_peek( pred, name );
        inc();
        return v;
    }

    value_t req_pop( pred_t pred, std::string name = "in predicate" )
    {
        value_t v = req_peek( pred, name );
        inc();
        return v;
    }

    bool holds()
    {
        return !empty();
    }

    bool holds( const value_t& v )
    {
        return peek() == v;
    }

    bool holds( fn_pred_t pred )
    {
        return pred( peek() );
    }

    bool holds( pred_t pred )
    {
        return pred( peek() );
    }

    std::optional< value_t > match_p( bool cond )
    {
        return cond ? req_pop() : std::optional< value_t >();
    }

    std::optional< value_t > match()
    {
        return match_p( holds() );
    }

    std::optional< value_t > match( const value_t& v )
    {
        return match_p( holds( v ) );
    }

    std::optional< value_t > match( fn_pred_t pred )
    {
        return match_p( holds( pred ) );
    }

    std::optional< value_t > match( pred_t pred )
    {
        return match_p( holds( pred ) );
    }


};

///////////////////////////////////////////////////////////////////////////////
// Lexing
///////////////////////////////////////////////////////////////////////////////

enum lex_type
{
    identifier,
    op, 

    literal_number,
    literal_bool,

    sym_semicolon, 
    sym_assign,
    sym_fun_path,
    sym_prod,
    sym_rarrow,

    lbrace,
    rbrace,
    lbrack,
    rbrack,
    lpara,
    rpara,

    kw_fun,
    kw_let,
    kw_if, 
    kw_then,
    kw_else,

    sp_newline, 
    sp_eof
};

const std::map< std::string, lex_type > keywords = {
    { "if",     kw_if },
    { "else",   kw_else },
    { "then",   kw_then },
    { "fun",    kw_fun },
    { "let",    kw_let }, 
    { "true",   literal_bool },
    { "false",  literal_bool },
    { "->",     sym_rarrow },
    { "=>",     sym_prod },
    { ":=",     sym_assign },
    { "|-",     sym_fun_path }
};

const std::map< int, lex_type > sp_chars = {
    { '(',      lpara },
    { ')',      rpara },
    { '[',      lbrack },
    { ']',      rbrack },
    { '{',      lbrace },
    { '}',      rbrace },
    { ';',      sym_semicolon }
};


struct lexeme {
    lex_type type;
    std::string content;
    int row;
    int col;

    friend std::ostream& operator<<(std::ostream &os, const lexeme &o)
    {
        pprint::PrettyPrinter p( os );
        p.quotes(true);
        p.line_terminator("");
        p.print( o.type, o.content, o.row, o.col );
        return os;
    }

    bool operator ==( const lexeme &o ) const
    {
        return o.type == type
            && o.content == content
            && o.row == row
            && o.col == col;
    }
};

static int isspecial( int c ) { 
    switch( c )
    {
        case '+':
        case '-':
        case '*':
        case '/':
        case ':':
        case '=':
        case '<':
        case '>':
        case '?':
        case '$':
        case '.':
        case '|':
        case '&':
            return true;
    }
    return false;
}

static int isidstart( int c ) { return std::isalpha( c ) || c == '_';  }
static int isidchar( int c )  { return isidstart( c ) || std::isdigit( c ); }
static int isopchar( int c )  { return isspecial( c ); }

static std::string show_char( int c ) { return c == EOF ? "eof" : std::string{ char( c ) }; }

static std::string show_lexem( const lexeme& l ) 
{
    std::stringstream ss;
    ss << l;
    return ss.str();
}

struct row_col {

    int newline = '\n';

    int row = 0; 
    int col = 0;

    row_col( int newline ) : newline( newline ) {};

    void on_inc( int last )
    {
        col++;
        if ( last == newline ) {
            row++;
            col = 0;         
        }
    }
};

template < typename generator_t >
struct lexer 
{
    parsing_state< generator_t, row_col > p_state;
    using value_t = lexeme;

    std::string buffer;
    int lex_row = 0;
    int lex_col = 0;

    lexer( generator_t g , int newline = '\n' ) 
         : p_state( std::move ( g ) 
                  , row_col( newline )
                  , EOF
                  , show_char ) {}

    lexeme flush_lex( lex_type t )
    {
        return { t, std::move( buffer ), lex_row, lex_col };
    }

    lexeme get_lit_number()
    {
        buffer.push_back( p_state.req_pop( std::isdigit ) );
        while ( std::optional< int > c = p_state.match( std::isdigit ) )
            buffer.push_back( c.value() );
        return flush_lex( literal_number );
    }

    lexeme flush_identifier( lex_type t )
    {
        const auto& it = keywords.find( buffer );
        if ( it != keywords.end() )
            return flush_lex( it->second );
        return flush_lex( t );
    }

    lexeme get_identifier()
    {
        buffer.push_back( p_state.req_pop( isidstart ) ); 
        while ( std::optional< int > c = p_state.match( isidchar ) )
            buffer.push_back( c.value() );
        return flush_identifier( identifier );
    }

    lexeme get_operator()
    {
        buffer.push_back( p_state.req_pop( isopchar ) ); 
        while ( std::optional< int > c = p_state.match( isopchar ) )
            buffer.push_back( c.value() );
        return flush_identifier( op );
    }

    lexeme get_singleton( int character, lex_type type )
    {
        buffer.push_back( p_state.req_pop( character ) );
        return flush_lex( type );
    }

    lexeme next()
    {
        int c = p_state.peek();

        while ( std::isspace( c ) ) {
            p_state.pop();
            c = p_state.peek();
        }

        if ( c == EOF )
            return flush_lex( sp_eof );


        lex_row = p_state.meta.row;
        lex_col = p_state.meta.col;

        if ( std::isdigit( c ) ) return get_lit_number();                     
        if ( isidstart( c ) )    return get_identifier();
        if ( isopchar( c ) )     return get_operator();

        const auto sp_it = sp_chars.find( c );
        if ( sp_it != sp_chars.end() )
            return get_singleton( c, sp_it->second );

        throw parsing_error( "unknown symbol: '"s
                           + ( c == EOF 
                                ? "EOF" 
                                : std::string{ char( c ) } ) 
                           + "' ("s + std::to_string( c ) + ")" );
    }

    bool empty()
    {
        return p_state.empty();
    }
};

///////////////////////////////////////////////////////////////////////////////
// Parser
///////////////////////////////////////////////////////////////////////////////

template < lex_type t >
static int istype( const lexeme& l ) { return l.type == t; };  
static int isliteral( const lexeme& l ) { return l.type == literal_bool 
                                              || l.type == literal_number; };
static int pat_start( const lexeme& l ) { return l.type == op && l.content == "<"; };
static int pat_end( const lexeme& l ) { return l.type == op && l.content == ">"; };
                        


template < lex_type... ls >
constexpr int isany( const lexeme& l ) { 
    return ( ( l.type == ls ) || ... );
};

template < typename T >
struct meta_unit
{
    void on_inc( T t ) {};
};

template < typename T >
struct meta_trace
{
    void on_inc( T t ) {
        TRACE( t );
    };
};

template < typename generator_t > 
struct parser 
{
    using p_state_t = parsing_state< lexer< generator_t >, meta_unit< lexeme > >;

    // [ priority_layer : ( l_asoc : { op }, r_asoc : { op } ) ]
    // { name : ( prio, asoc ) } 
    std::map< std::string, std::pair< int, bool > > op_table;

    int op_prio_depth = 0;

    p_state_t p_state; 

    parser( generator_t generator, int op_prio_depth )
        : p_state( lexer( std::move( generator ) ) 
                 , meta_unit< lexeme >{}
                 , { sp_eof, "", -1, -1 }
                 , show_lexem ) 
        , op_prio_depth( op_prio_depth ) {}

    std::stack< std::string > stack_trace;

    void tpush( std::string s )
    {
        stack_trace.push( s );
    }

    void tpop()
    {
        // stack_trace.pop();
    }

    template < typename T > 
    T rpop( T t ) { 
        tpop();
        return t; 
    }

    std::string p_identifier()
    {
        tpush( "identifier" );
        return rpop( p_state.req_pop( istype< identifier > ).content );
    }

    ast::variable p_variable()
    {
        tpush( "variable" );
        return rpop( ast::variable( p_identifier() ) );
    }

    int p_number() {
        tpush( "number" );
        lexeme l = p_state.req_pop( istype< literal_number > ); 
        int content = 0; 
        for ( char c : l.content ) {
            content *= 10;
            content += c - '0';
        }
        tpop();
        return content;
    }

    bool p_bool()
    {
        tpush( "bool" );
        lexeme l = p_state.req_pop( istype< literal_bool > );
        return rpop( l.content == "true" );
    }

    template < template < typename T > class wrapper_t, typename R >
    R p_literal_template()
    {
        lexeme l = p_state.req_peek( isliteral, "literal" );

        if ( l.type == literal_number )
            return wrapper_t< int >{ p_number() };
        if ( l.type == literal_bool )
            return wrapper_t< bool >{ p_bool() };

        assert( false );
    }

    ast::ast_node p_literal()
    {
        tpush( "literal" );
        return rpop( std::move ( p_literal_template< ast::literal, ast::ast_node >() ) );
    }
    

    static bool p_atom_start_lex( const lexeme& l ) {
        return isliteral( l ) || isany< lpara, identifier, kw_fun >( l );
    };

    ast::ast_node p_atom()
    {
        tpush( "atom" );
        lexeme l = p_state.req_peek( p_atom_start_lex, "literal, '(', identifier or function definition" );

        if ( l.type == lpara ) {
            p_state.req_pop( istype< lpara > );
            ast::ast_node expr = p_expression();
            p_state.req_pop( istype< rpara > );
            return rpop( std::move( expr ) );
        }
        if ( l.type == identifier )
            return rpop( p_variable() );
        if ( isliteral( l ) )
            return rpop( p_literal() );
        if ( istype< kw_fun >( l ) )
            return rpop( p_fundef() );

        assert( false );
    }

    ast::ast_node p_call()
    {
        tpush( "call" );
        tpush( "call - fun" );
        ast::ast_node fun = p_atom(); 
        tpop();
        std::vector< ast::node_ptr > args;
        tpush( "call - arguments" );
        while ( p_state.holds( p_atom_start_lex ) )
            args.push_back( clone( p_atom() ) );
        tpop();
        return rpop( args.empty() 
                        ? fun 
                        : ast::function_call( ast::clone( fun ), args ) );
    }

    ast::ast_node p_expression( int layer, bool asoc )
    {
        tpush( "expression(" + std::to_string( layer ) + ")");
        if ( layer == op_prio_depth ) {
            return rpop( p_call() );
        }

        int next_layer = asoc ? layer + 1 : layer;
        bool next_asoc = ! asoc;

        std::vector< ast::ast_node > nodes{ p_expression( next_layer, next_asoc ) };
        std::vector< std::string > operators;

        while ( p_state.holds( istype< op > ) )
        {
            const lexeme& op_lexeme = p_state.peek();
            std::string op = op_lexeme.content;
            const auto& ops = op_table.find( op );

            if ( ops == op_table.end() )
                throw parsing_error( "operator '"s + op + "' is unknown"s );

            // Here, invariant of p_expression => operator will be processed 
            // by a parent.
            const auto& [ op_prio, op_asoc ] = ops->second;
            if ( op_prio != layer || op_asoc != asoc ) 
                break;

            p_state.pop();
            operators.push_back( std::move( op ) );
            nodes.push_back( p_expression( next_layer, next_asoc ) );
        }

        // left(-to-right) asociativity 
        if ( ! asoc ) {
            ast::ast_node acc = std::move( nodes[ 0 ] );
            for ( int i = 0; i < operators.size(); i++ )
                acc = ast::function_call( 
                    ast::clone( ast::variable( std::move( operators[ i ] ) ) ), 
                    { ast::clone( std::move( acc ) )
                    , ast::clone( std::move( nodes[ i + 1 ] ) ) } );
            return rpop( std::move( acc ) );
        // right(-to-left ) asociativity 
        } 

        ast::ast_node acc = std::move( nodes[ operators.size() ] );
        for ( int i = operators.size() - 1; i >= 0; i-- ) 
            acc = ast::function_call( 
                ast::clone( ast::variable( std::move( operators[ i ] ) ) ), 
                { ast::clone( std::move( nodes[ i ] ) )
                , ast::clone( std::move( acc ) ) } );
        return rpop( std::move( acc ) );
    }

    ast::ast_node p_expression()
    {
        tpush( "expression" );
        return rpop( p_expression( 0, false ) );
    }

    ast::variable_pattern p_variable_pattern()
    {
        tpush( "variable pattern" );
        return rpop( ast::variable_pattern{ p_identifier() } );
    }

    ast::pattern p_literal_pattern()
    {
        tpush( "literal pattern" );
        return rpop( p_literal_template< ast::literal_pattern, ast::pattern >() );
    }

    ast::object_pattern p_object_pattern()
    {
        tpush( "object pattern" );
        p_state.req_pop( pat_start, "<" );
        std::string identifier = p_identifier();
        std::vector< ast::pattern > children;
        while ( ! p_state.match( pat_end ) )
            children.push_back( p_pattern() );
        return rpop( ast::object_pattern{ identifier, std::move( children ) } );
    }

    static int p_pattern_fch( const lexeme& l ) 
    {
        return istype< identifier >( l ) || pat_start( l ) || isliteral( l );
    }

    ast::pattern p_pattern()
    {
        tpush( "pattern" );
        lexeme l = p_state.req_peek( p_pattern_fch, "identifier, '<' or literal" );

        if ( istype< identifier >( l ) ) 
            return rpop( p_variable_pattern() );
        if ( pat_start( l ) ) 
            return rpop( p_object_pattern() );
        if ( isliteral( l ) )
            return rpop( p_literal_pattern() );
        
        assert( false );
    }

    ast::function_path p_funpath()
    {
        tpush( "function path" );
        p_state.req_pop( istype< sym_fun_path >, "|-" );
    
        std::vector< ast::pattern > patterns{ p_pattern() };

        tpush( "function path - patterns" );
        while ( ! p_state.match( istype< sym_rarrow > ) )
            patterns.push_back( p_pattern() );
        tpop();

        tpush( "function path - expression" );
        ast::ast_node expr = p_expression();
        tpop();
        
        return rpop( ast::function_path{ std::move( patterns )
                                       , ast::variable_pattern{ "_" }
                                       , ast::clone( expr ) } );
    }

    ast::function_def p_fundef()
    {
        tpush( "function definition" );
        p_state.req_pop( istype< kw_fun > );

        std::vector< ast::function_path > paths;

        int arity = 0;
        
        while ( p_state.holds( istype< sym_fun_path > ) ) {
            ast::function_path p = p_funpath();

            if ( ! paths.empty() && p.input_patterns.size() != arity ) 
                throw parsing_error( "the number of arguments does not match" );
            arity = p.input_patterns.size();

            paths.push_back( std::move( p ) );
        }

        if ( paths.empty() )
            throw parsing_error( "there are no function paths" );

        return rpop( ast::function_def{ std::move( paths ), arity } );
    }

};

using lexer_str = lexer< string_generator >;
using parser_str = parser< string_generator >;

void tests_parser();
