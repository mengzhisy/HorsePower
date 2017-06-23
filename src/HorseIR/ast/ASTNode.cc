#include "ASTNode.h"

using ASTNode = horseIR::ast::ASTNode ;

ASTNode::ASTNode(ASTNode::MemManagerType& mem)
    : ASTNode(nullptr, mem)
{}

ASTNode::ASTNode(const antlr4::tree::ParseTree* pTree, ASTNode::MemManagerType& mem)
    : cst{pTree}
{
    mem.manage(this) ;
}

const antlr4::tree::ParseTree* ASTNode::getCST()
{
    return cst ;
}

ASTNode::MemManagerType& ASTNode::MemManagerType::manage(ASTNode* ptr)
{
    auto p = std::find_if(pool.begin(), pool.end(),
                          [&] (std::unique_ptr<ASTNode>& p_search) -> bool {
                              return p_search.get() == ptr ;
                          }) ;
    if (p != pool.cend()) {
        /* duplicate ignore */
    } else {
        pool.emplace_back(ptr) ;
    }
    return *this ;
}

ASTNode::MemManagerType& ASTNode::MemManagerType::release(ASTNode* ptr)
{
    pool.erase(std::remove_if(pool.begin(), pool.end(),
                              [&] (std::unique_ptr<ASTNode>& p_search) -> bool {
                                  return p_search.get() == ptr ;
                              }));
    return *this ;
}