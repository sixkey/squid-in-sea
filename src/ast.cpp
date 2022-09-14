#include "ast.hpp"

namespace ast {

node_ptr clone( const ast_node& node )
{
    return std::visit( 
            []( const auto& p ) { return std::make_shared< ast_node >( p ); }
            , node ); 
}

path_ptr clone( const ast::function_path& path )
{
    return std::make_shared< const ast::function_path >( path ); 
}

}
