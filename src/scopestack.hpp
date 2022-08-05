#include "k-either.hpp"
#include "pprint.hpp"
#include <map>
#include <ostream>
#include <vector>

template < typename identifier_t, typename store_id >
struct bindings_tree
{
    struct bindings_node;

    using bindings_t = std::map< identifier_t, store_id >;
    using bindings_node_ptr = std::unique_ptr< bindings_node >;

    struct bindings_node 
    {
        bindings_node* parent = nullptr;
        std::vector< bindings_node_ptr > children;
        std::vector< bindings_t > bindings;

        bindings_node( bindings_node* parent ) : parent( parent ) {};
        bindings_node() {};

        friend std::ostream& operator<<( std::ostream& os, struct bindings_node& node )
        {
            pprint::PrettyPrinter printer( os );
            printer.print( node.bindings );
            for ( const auto& child : node.children )
                os << *child;
            return os;
        }
    };

    bindings_node_ptr root;
    bindings_node* head = nullptr;

    bindings_tree() : root( std::move( std::make_unique< bindings_node >() ) )
                    , head( root.get() ) {}

    void add_scope()
    {
        assert( head != nullptr );
        head->bindings.push_back( {} );
    }

    void pop_scope()
    {
        assert( head != nullptr );
        assert( !head->bindings.empty() );
        head->bindings.pop_back();
    }

    void pop_head()
    {
        assert( head != nullptr );
        assert( head->parent != nullptr );
        auto parent = head->parent;
        parent->children.pop();
        if ( parent->children.empty() )
            head = parent;
        else 
            head = parent.children.back().get();
    }

    void add_sibling()
    {
        assert( head != nullptr );
        assert( head->parent != nullptr );
        head->parent->children.push( bindings_node( head->parent ) );
        head = &head->parent->children.back();
    }

    std::optional< store_id > lookup( const identifier_t& name )
    {
        auto current = head;

        while ( current != nullptr ) { 
            for ( int i = current->bindings.size() - 1; i >= 0; i-- ) {
                const auto &it = current->bindings[ i ].find( name );
                if ( it != current->bindings[ i ].end() )
                    return it->second;
            }
            current = current->parent;
        }
        return {};
    }

    either< identifier_t, std::map< identifier_t, store_id > > lookup( const std::set< identifier_t >& names )
    {
        std::map< identifier_t, store_id > bindings;

        for ( const auto& name : names )
        {
            const auto id = lookup( name );
            if ( ! id.has_value() )
                return { name };
            bindings.insert( { name, id.value() } );
        }
        return { bindings };
    }

    void bind( const identifier_t& name, store_id id )
    {
        assert( head != nullptr );
        assert( ! head->bindings.empty() );
        head->bindings.back().insert_or_assign( name, id );
    }

    friend std::ostream& operator<<( std::ostream& os, const bindings_tree& tree )
    {
        return os << *tree.root;
    }
};
