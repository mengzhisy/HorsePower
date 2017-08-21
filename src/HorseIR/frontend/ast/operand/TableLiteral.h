#pragma once

#include "../../misc/InfixOStreamIterator.h"
#include "../AST.h"

namespace horseIR
{
namespace ast
{

namespace storage
{

struct TableColumn {
  std::string head;
  Literal *content = nullptr;
};

}

class TableLiteral : public Literal {
 public:
  using ElementType = storage::TableColumn;

  TableLiteral ();
  explicit TableLiteral (const CSTType *cst);
  TableLiteral (TableLiteral &&literal) = default;
  TableLiteral (const TableLiteral &literal);
  TableLiteral &operator= (TableLiteral &&literal) = delete;
  TableLiteral &operator= (const TableLiteral &literal) = delete;
  ~TableLiteral () override = default;

  bool isNull () const;
  std::vector<ElementType> getValue () const;
  template<class T>
  std::enable_if_t<std::is_constructible<std::vector<ElementType>, T>::value>
  setValue (T &&valueContainer);
  void setValue (std::nullptr_t);

  std::size_t getNumNodesRecursively () const override;
  std::vector<ASTNode *> getChildren () const override;
  TableLiteral *duplicateDeep (ASTNodeMemory &mem) const override;
  std::string toString () const override;

 protected:
  std::unique_ptr<std::vector<ElementType>> value = nullptr;
  void __duplicateDeep (ASTNodeMemory &mem, const TableLiteral *literal);
  std::string elementToString (const ElementType &elementType) const;
};

inline TableLiteral::TableLiteral ()
    : Literal (ASTNodeClass::TableLiteral, LiteralClass::Table)
{}

inline TableLiteral::TableLiteral (const CSTType *cst)
    : Literal (ASTNodeClass::TableLiteral, cst, LiteralClass::Table)
{}

inline TableLiteral::TableLiteral (const TableLiteral &literal)
    : Literal (literal)
{
  if (literal.value == nullptr)
    { value.reset (nullptr); }
  else
    { value.reset (new std::vector<ElementType> (*(literal.value))); }
}

inline bool TableLiteral::isNull () const
{ return value == nullptr; }

inline std::vector<TableLiteral::ElementType> TableLiteral::getValue () const
{ return *value; }

template<class T>
inline std::enable_if_t<
    std::is_constructible<std::vector<TableLiteral::ElementType>, T>::value
>
TableLiteral::setValue (T &&valueContainer)
{
  value.reset (new std::vector<ElementType> (std::forward<T> (valueContainer)));
  for (const ElementType &element : *value)
    if (element.content != nullptr)
      { element.content->setParentASTNode (this); }
}

inline void TableLiteral::setValue (std::nullptr_t)
{ value.reset (nullptr); }

inline std::size_t TableLiteral::getNumNodesRecursively () const
{
  std::size_t counter = 1;
  if (literalType != nullptr)
    { counter = counter + literalType->getNumNodesRecursively (); }
  if (value != nullptr)
    {
      for (const ElementType &element : *value)
        if (element.content != nullptr)
          { counter = counter + element.content->getNumNodesRecursively (); }
    }
  return counter;
}

inline std::vector<ASTNode *> TableLiteral::getChildren () const
{
  std::vector<ASTNode *> childrenNodes{};
  if (value != nullptr)
    {
      for (const ElementType &element: *value)
        if (element.content != nullptr)
          { childrenNodes.push_back (static_cast<ASTNode *>(element.content)); }
    }
  if (literalType != nullptr)
    { childrenNodes.push_back (static_cast<ASTNode *>(literalType)); }
  return childrenNodes;
}

inline TableLiteral *TableLiteral::duplicateDeep (ASTNodeMemory &mem) const
{
  auto tableLiteral = mem.alloc<TableLiteral> ();
  tableLiteral->__duplicateDeep (mem, this);
  return tableLiteral;
}

inline std::string TableLiteral::toString () const
{
  std::ostringstream s;
  if (value == nullptr)
    {
      s << "null :"
        << ((literalType == nullptr) ? "nullptr" : literalType->toString ());
      return s.str ();
    }
  s << '{';
  std::transform (
      value->cbegin (), value->cend (),
      misc::InfixOStreamIterator<std::string> (s, ", "),
      [&] (const ElementType &element) -> std::string
      { return elementToString (element); });
  s << "} :"
    << ((literalType == nullptr) ? "nullptr" : literalType->toString ());
  return s.str ();
}

inline void
TableLiteral::__duplicateDeep (ASTNodeMemory &mem, const TableLiteral *literal)
{
  assert (literal != nullptr);
  Literal::__duplicateDeep (mem, literal);
  if (literal->value == nullptr)
    { value.reset (nullptr); }
  else
    {
      value.reset (new std::vector<ElementType> ());
      value->reserve (literal->value->size ());
      std::transform (
          literal->value->cbegin (), literal->value->cend (),
          std::back_inserter (*value),
          [&] (const ElementType &element) -> ElementType
          {
            ElementType returnElement{};
            returnElement.head = element.head;
            if (element.content != nullptr)
              {
                returnElement.content = dynamic_cast<Literal *>(
                    element.content->duplicateDeep (mem)
                );
                returnElement.content->setParentASTNode (this);
              }
            else
              { returnElement.content = nullptr; }
            return returnElement;
          });
    }
}

inline std::string
TableLiteral::elementToString (const ElementType &elementType) const
{
  std::ostringstream stream;
  stream << elementType.head
         << " -> "
         << ((elementType.content == nullptr) ?
             "nullptr" : elementType.content->toString ());
  return stream.str ();
}

}
}