#include "pattern_graph.hpp"
#include "pattern.hpp"
#include <cassert> 
#include <iostream>
#include "pprint.hpp"

void tests_pattern_graph()
{
    pprint::PrettyPrinter printer;

    pattern_graph g;  

    variable_pattern a = variable_pattern( "a" );
    variable_pattern b = variable_pattern( "b" );
    object_pattern meter_a = object_pattern( "Meter", { a.clone() } );
    object_pattern mile_b = object_pattern( "Mile", { b.clone() } );
    object_pattern mile_mile_b = object_pattern( "Mile", { mile_b.clone() } );

    g.add_edge( meter_a, mile_b,      "meters_to_miles" );
    g.add_edge( mile_b,  meter_a,     "miles_to_meters" );
    g.add_edge( mile_b,  mile_mile_b, "nest_mile" );
    g.add_edge( a,       mile_b,      "wrap_mile" );

    int D = 10000;

    auto path_meter_mile = g.get_path( meter_a, mile_b, D );
    assert( path_meter_mile == id_path_t{ "meters_to_miles" } );

    auto path_mile_meter = g.get_path( mile_b, meter_a, D );
    assert( path_mile_meter == id_path_t{ "miles_to_meters" } );

    auto path_id = g.get_path( mile_b, mile_b, D ); 
    assert( path_id == id_path_t{} );

    auto path_nested_mile = g.get_path( meter_a, mile_mile_b, D );
    assert( ( path_nested_mile == id_path_t{ "meters_to_miles", "nest_mile" } ) );

    auto nest_a = g.get_path( a, mile_mile_b, D );
    assert( ( nest_a == id_path_t{ "wrap_mile", "nest_mile" } ) );

    // for ( auto &a : g.reachable( a, D, 10 ) ) 
    //    printer.print( a.first->to_string(), a.second );
}

