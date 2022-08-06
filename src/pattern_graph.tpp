#include "pattern_graph.hpp"
#include "pattern.hpp"
#include <cassert> 
#include <iostream>
#include "pprint.hpp"

void test_pattern_graph()
{
    pprint::PrettyPrinter printer;

    pattern_graph g;  

    variable_pattern a = variable_pattern( "a" );
    variable_pattern b = variable_pattern( "b" );
    object_pattern meter_a = object_pattern( "Meter", { a } );
    object_pattern mile_b = object_pattern( "Mile", { b } );
    object_pattern mile_mile_b = object_pattern( "Mile", { mile_b } );
    object_pattern wrapper_a = object_pattern( "Wrapper", { a } );
    object_pattern wrapper_wr_a = object_pattern( "Wrapper", { wrapper_a } );

    literal_pattern< int > p3 = literal_pattern< int >( "Int", 3 );
    literal_pattern< int > p6 = literal_pattern< int >( "Int", 4 );

    g.add_edge( meter_a, mile_b,      "meters_to_miles" );
    g.add_edge( mile_b,  meter_a,     "miles_to_meters" );
    g.add_edge( mile_b,  mile_mile_b, "nest_mile" );
    g.add_edge( a,       wrapper_a,   "wrap" );
    g.add_edge( p3,      p6,          "mul" );

    int D = 4;

    auto path_meter_mile = g.get_path( meter_a, mile_b, D );

    assert( path_meter_mile == id_path_t{ "meters_to_miles" } );

    auto path_mile_meter = g.get_path( mile_b, meter_a, D );
    assert( path_mile_meter == id_path_t{ "miles_to_meters" } );

    auto path_id = g.get_path( mile_b, mile_b, D ); 
    assert( path_id == id_path_t{} );

    auto path_nested_mile = g.get_path( meter_a, mile_mile_b, D );
    assert( ( path_nested_mile == id_path_t{ "meters_to_miles", "nest_mile" } ) );

    auto path_multiply = g.get_path( p3, p6, D );
    assert( ( path_multiply == id_path_t{ "mul" } ) );


    // TODO: Unify on patterns

    // auto nest_a = g.get_path( wrapper_a, wrapper_wr_a, D );
    // assert( ( nest_a == id_path_t{ "wrap" } ) );

    // for ( auto &a : g.reachable( a, D, 10 ) ) 
    //    printer.print( a.first->to_string(), a.second );
}

int main()
{
    test_pattern_graph();
}
