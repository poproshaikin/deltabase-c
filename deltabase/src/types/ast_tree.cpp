//
// Created by poproshaikin on 02.12.25.
//

#include "include/ast_tree.hpp"
#include <sstream>

namespace types
{
    std::string
    BinaryExpr::to_string() const
    {
        std::stringstream ss;
        ss << std::endl;
        ss << " BinaryExpr op " << static_cast<int>(this->op) << std::endl;
        ss << " BinaryExpr left " << static_cast<int>(this->left->type) << std::endl;
        ss << " BinaryExpr right " << static_cast<int>(this->right->type) << std::endl;
        return ss.str();
    }

    AstNode::AstNode(AstNodeType type, AstNodeValue&& value)
        : type(type), value(std::move(value))
    {
    }
}
