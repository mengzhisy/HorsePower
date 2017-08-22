#include <iostream>

#include "./ast/AST.h"
#include "./ast/CSTConverter.h"

using namespace horseIR;

const char *rawProgram = R"PROGRAM(
/*
 * varchar(99) -> sym
 * int         -> i64
 */
module default {
    import Builtin.*;
    def loop() :i32 {
        [ entry ]
        i :? = (1, 2, 3):i32;
        goto [entry] i;
    }

    def find_valid_index(colVal:i64, indexBool:i64) : i64 {
        colSize   :? = @len(colVal);
        validBool :? = @lt(indexBool,colSize);
        indexSize :? = @len(indexBool);
        indexRange:? = @range(indexSize);
        validIndex:? = @compress(validBool, indexRange);
        return validIndex;
    }
    def find_valid_item(colVal:i64, indexBool:i64)  : i64 {
        colSize   :? = @len(colVal);
        validBool :? = @lt(indexBool,colSize);
        validItem :? = @compress(validBool, indexBool);
        return validItem;
    }
    def main() : table {
        a0:table = @load_table(`Employee);
        a1:table = @load_table(`Department);

        s0:sym = check_cast(@column_value(a0, `LastName)      , sym);
        s1:i64 = check_cast(@column_value(a0, `DepartmentID)  , i64);
        s2:i64 = check_cast(@column_value(a1, `DepartmentID)  , i64);
        s3:sym = check_cast(@column_value(a1, `DepartmentName), sym);

        t0:i64 = @index_of       (s2,s1);
        t1:i64 = @find_valid_index(s2,t0);
        t2:i64 = @find_valid_item (s2,t0);

        r0:sym = @index(s0,t1);
        r1:i64 = @index(s1,t1);
        r2:sym = @index(s3,t2);

        k0:sym       = (`LastName,`DepartmentID,`DepartmentName);
        k1:list<sym> = @tolist(k0);
        k2:list<?>   = @list(r0,r1,r2);
        z:table      = @table(k1,k2);

        return z;
    }
}
)PROGRAM";

#include <chrono>
#include <thread>
#include "ast/ASTPrinter.h"
#include "interpreter/StatementFlow.h"

int main (int argc, const char *argv[])
{
  auto start = std::chrono::steady_clock::now ();
  antlr4::ANTLRInputStream stream (rawProgram);
  HorseIRLexer lexer (&stream);
  antlr4::CommonTokenStream tokenStream (&lexer);
  HorseIRParser parser (&tokenStream);
  auto context = parser.program ();

  ast::ASTNode::ASTNodeMemory mem;

  auto astNode = ast::CSTConverter::convert (mem, context);
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>
      (std::chrono::steady_clock::now () - start);
  std::cout << "Time :" << duration.count () / 1000000.0 << " ms" << std::endl;

  astNode->setEnclosingFilename ("stdin");
  std::cout << astNode->getASTNodeClass () << std::endl;
  std::cout << astNode->getNumNodesRecursively () << std::endl;

  ast::ASTPrinter printer (std::cout);
  printer.print (astNode);
  std::cout << std::endl;

  auto flow = interpreter::StatementFlow::construct (astNode);
  ast::Module *module = *(astNode->modulesConstBegin ());
  ast::Method *method = *(module->methodsConstBegin ());
  const ast::Statement *statement = *(method->statementsConstBegin ());
  while (statement != nullptr)
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for (1s);
      printer.print (statement);
      std::cout << std::endl;
      statement = flow.getOutwardFlowOnTrue (statement);
    }

  return 0;
}