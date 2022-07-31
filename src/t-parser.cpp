#include <cassert>
#include <iostream>

#include "parser.hpp"
#include "pprint.hpp"

void test_lex_basic()
{
    lexer l( "f := fun a b -> ( a + b );\n g := fun c d -> a + b;" );
    
    std::vector< lexeme > test_case = {
        { identifier, "f", 0, 0 },
        { sym_assign, ":=", 0, 2 },
        { kw_fun, "fun", 0, 5 },
        { identifier, "a", 0, 9 },
        { identifier, "b", 0, 11 },
        { sym_rarrow, "->", 0, 13 },
        { lpara, "(", 0, 16 },
        { identifier, "a", 0, 18 },
        { op, "+", 0, 20 },
        { identifier, "b", 0, 22 },
        { rpara, ")", 0, 24 },
        { sym_semicolon, ";", 0, 25 },
        { identifier, "g", 1, 1 },
        { sym_assign, ":=", 1, 3 },
        { kw_fun, "fun", 1, 6 },
        { identifier, "c", 1, 10 },
        { identifier, "d", 1, 12 },
        { sym_rarrow, "->", 1, 14 },
        { identifier, "a", 1, 17 },
        { op, "+", 1, 19 },
        { identifier, "b", 1, 21 },
        { sym_semicolon, ";", 1, 22 } 
    };

    for ( const auto& x : test_case )
    {
        assert( x == l.next() );
    }
}

void sandbox()
{
    parser p( "( fun a b -> a + b ) 3 4", 10 ); 
}

void tests_parser()
{
    test_lex_basic();
    sandbox();
}
