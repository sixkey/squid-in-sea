#include <assert.h> 

#include "pattern.hpp"

bool contains( const pattern& p, const pattern& q ) {
    return comparator::contains( p, q );
}
