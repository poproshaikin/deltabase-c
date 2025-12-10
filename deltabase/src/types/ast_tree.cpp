//
// Created by poproshaikin on 02.12.25.
//

#include "include/ast_tree.hpp"

namespace types
{
    AstNode::AstNode(AstNodeType type, AstNodeValue&& value)
        : type(type), value(std::move(value))
    {
    }
}
