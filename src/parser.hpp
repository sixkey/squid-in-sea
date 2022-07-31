#include "pprint.hpp"
#include <cstdio>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include <functional> 
#include <cctype>

#include "ast.hpp"

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

struct string_generator 
{
    using value_t = int;

    std::string content;
    int counter = 0;
    
    string_generator( std::string content ) : content( content ) {};

    int next()
    {
        assert( !empty() );
        return content[ counter++ ];
    }
    
    int empty()
    {
        return counter >= content.size();
    }

};

template < typename generator_t >
struct parsing_state
{
    using value_t = typename generator_t::value_t;
    using fn_pred_t = std::function< int( value_t v ) >;
    using pred_t = int(value_t);
    using show_value_t = std::function< std::string( const value_t& ) >;
    using on_inc_t = std::function< void( value_t v ) >;

    bool loaded;
    generator_t generator; 

    value_t current;

    on_inc_t on_inc;
    value_t def; 
    show_value_t show_value;

    parsing_state( generator_t content
                 , value_t def
                 , on_inc_t on_inc
                 , show_value_t show_value ) 
                 : generator( std::move( content ) )
                 , on_inc( on_inc )
                 , def( def ) 
                 , show_value( show_value ) {}
                    
    void inc()
    {
        on_inc( std::move( current ) );
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

    value_t match()
    {
        return pop();
    }

    std::optional< value_t > match( const value_t& v )
    {
        return peek() == v ? req_pop() : std::optional< value_t >();
    }

    std::optional< value_t > match( fn_pred_t pred )
    {
        return pred( peek() ) ? req_pop() : std::optional< value_t >();
    }

    std::optional< value_t > match( pred_t pred )
    {
        return pred( peek() ) ? req_pop() : std::optional< value_t >();
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
    { ":=",     sym_assign }
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

struct lexer 
{

    parsing_state< string_generator > p_state;
    using value_t = lexeme;

    std::string buffer;
    int lex_row = 0;
    int lex_col = 0;

    int newline = '\n';

    int row = 0; 
    int col = 0;

    lexer( std::string input, int newline = '\n' ) 
         : p_state( { input }
                  , EOF
                  , nullptr
                  , show_char )
         , newline( newline ) {
            p_state.on_inc = [&]( int c ){ this->on_inc( c ); };
         }

    void on_inc( int last )
    {
        col++;
        if ( last == newline ) {
            row++;
            col = 0;         
        }
    }

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

        lex_row = row;
        lex_col = col;

        if ( std::isdigit( c ) ) return get_lit_number();                     
        if ( isidstart( c ) )    return get_identifier();
        if ( isopchar( c ) )     return get_operator();

        const auto sp_it = sp_chars.find( c );
        if ( sp_it != sp_chars.end() )
            return get_singleton( c, sp_it->second );

        throw parsing_error( std::string( "unknown symbol: '" ) 
                           + ( c == EOF 
                                ? "EOF" 
                                : std::string{ char( c ) } ) 
                           + "'" );
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

struct operator_data 
{
    bool asoc;     
};

struct parser 
{
    using p_state_t = parsing_state< lexer >;

    std::vector< std::map< std::string, operator_data > > operators;

    p_state_t p_state; 

    parser( std::string content, int op_prio_depth )
        : p_state( lexer( std::move( content ) ) 
                 , { sp_eof, "", -1, -1 }
                 , nullptr
                 , show_lexem ) 
    {
        p_state.on_inc = [&]( lexeme l ){ this->on_inc( std::move( l ) ); };

        for( int i = 0; i < op_prio_depth; i++ )
            operators.emplace_back();
    }

    void on_inc( lexeme l ) 
    {}

    ast::variable p_variable()
    {
        auto id = p_state.req_pop( istype< identifier > );
        return ast::variable( std::move( id.content ) );
    }

    ast::literal< int > p_number()
    {
        lexeme l = p_state.req_pop( istype< literal_number > ); 
        int content = 0; 
        for ( char c : l.content ) {
            content *= 10;
            content += c - '0';
        }
        return ast::literal< int >( content );
    }

    ast::literal< bool > p_bool()
    {
        lexeme l = p_state.req_pop( istype< literal_bool > );
        return l.content == "true" 
            ? ast::literal< bool >( true )
            : ast::literal< bool >( false );
    }

    ast::ast_node p_literal()
    {
        lexeme l = p_state.req_peek( isliteral );

        if ( l.type == literal_number )
            return p_number();
        if ( l.type == literal_bool )
            return p_bool();

        assert( false );
    }

    ast::ast_node p_atom()
    {
        lexeme l = p_state.req_peek();

        if ( l.type == lpara ) {
            p_state.req_pop( istype< lpara > );
            ast::ast_node expr = p_expression();
            p_state.req_pop( istype< rpara > );
            return std::move( expr );
        }
        if ( l.type == identifier )
            return p_variable();
        if ( isliteral( l ) )
            return p_literal();

        assert( false );
    }

    ast::ast_node p_expression()
    {
        std::vector< ast::ast_node > atoms;

        // TODO: expression parsing
    }

};

void tests_parser();
