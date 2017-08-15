#pragma once

#include "../AST.h"

namespace horseIR
{
namespace ast
{

namespace storage
{

struct Date {
  std::uint16_t year;
  std::uint8_t month;
  std::uint8_t day;
};

}

class DateLiteral : public VectorLiteral<storage::Date> {
 public:
  explicit DateLiteral (ASTNodeMemory &mem)
      : VectorLiteral<storage::Date>
            (mem, ASTNodeClass::DateLiteral, LiteralClass::Date)
  {}

  DateLiteral (ASTNodeMemory &mem, const CSTType *cst)
      : VectorLiteral<storage::Date>
            (mem, ASTNodeClass::DateLiteral, cst, LiteralClass::Date)
  {}

  DateLiteral (DateLiteral &&literal) = default;
  DateLiteral (const DateLiteral &literal) = default;
  DateLiteral &operator= (DateLiteral &&literal) = delete;
  DateLiteral &operator= (const DateLiteral &literal) = delete;
  ~DateLiteral () override = default;

  DateLiteral *duplicateDeep (ASTNodeMemory &mem) const override
  {
    auto dateLiteral = new DateLiteral (mem);
    dateLiteral->VectorLiteral<storage::Date>::__duplicateDeep (mem, this);
    return dateLiteral;
  }

 protected:
  std::string elementToString (const storage::Date &elementType) const override
  {
    std::ostringstream stream;
    stream << elementType.year << '.'
           << unsigned(elementType.month) << '.'
           << unsigned(elementType.day);
    return stream.str ();
  }
};

}
}