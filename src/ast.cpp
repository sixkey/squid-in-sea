#include "ast.hpp"

namespace ast {

node_ptr clone( const ast_node& node )
    {
        return std::visit( 
                []( const auto& p ) { return std::make_shared< ast_node >( p ); }
                , node ); 
    }
}
