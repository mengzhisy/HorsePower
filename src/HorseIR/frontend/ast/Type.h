#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#include "ASTNode.h"
#include "antlr4-runtime.h"
#include "HorseIRParser.h"

namespace horseIR {
    namespace ast {
        class Type : public ASTNode {
        public:
            enum class TypeClass {
                Wildcard, Scalar, List, Dictionary, Enumeration, Function
            } ;
            Type() = delete ;
            Type(ASTNode* parent, const antlr4::tree::ParseTree* cst, ASTNode::MemManagerType& mem, Type::TypeClass p_typeClass, ASTNode::ASTNodeClass type) ;
            Type(ASTNode::MemManagerType& mem, Type::TypeClass p_typeClass, ASTNode::ASTNodeClass type) ;
            virtual ~Type() override = default ;

            Type::TypeClass getTypeClass() const ;
            virtual bool isGeneralizationOf(const Type*) const = 0 ;
            
            static Type* makeTypeASTNode(ASTNode* parent, HorseIRParser::TypeContext* cst, ASTNode::MemManagerType& mem) ;
            static Type* makeTypeASTNode(HorseIRParser::TypeContext* cst, ASTNode::MemManagerType& mem) ;
            static std::vector<Type*> makeTypeSignatureASTNodes(ASTNode* parent, HorseIRParser::TypeSignatureListContext* cst, ASTNode::MemManagerType& mem) ;
            static std::vector<Type*> makeTypeSignatureASTNodes(HorseIRParser::TypeSignatureListContext* cst, ASTNode::MemManagerType& mem) ;
        protected:
            const Type::TypeClass typeClass ;
        } ;
        
        class ScalarType : public Type {
        public:
            enum class ScalarClass {
                Bool, Char, Integer8, Integer16, Integer32, Integer64, FP32, FP64,
                Complex, Symbol, Month, Date, DateTime, Minute, Second, Time, String,
                Table, KeyTable
            } ;
            
            ScalarType() = delete ;
            ScalarType(ASTNode* parent, HorseIRParser::TypeCaseScalarContext* cst, ASTNode::MemManagerType& mem) ;
            ScalarType(HorseIRParser::TypeCaseScalarContext* cst, ASTNode::MemManagerType& mem) ;
            ScalarType(ScalarType::ScalarClass type, ASTNode::MemManagerType& mem) ;
            ScalarType(ASTNode::MemManagerType& mem) ;
            ~ScalarType() override = default ;
            
            virtual bool isGeneralizationOf(const Type* type) const override ;

            virtual std::size_t getNumNodesRecursively() const override ;
            virtual std::vector<ASTNode*> getChildren() const override ;
            virtual std::string toString() const override ;
            virtual std::string toTreeString() const override ;
            
            ScalarType::ScalarClass getScalarClass() const ;
            ScalarType& setScalarClass(const ScalarType::ScalarClass type) ;
        protected:
            ScalarType::ScalarClass scalarClass ; 
        } ;

        class WildcardType : public Type {
        public:
            WildcardType() = delete ;
            WildcardType(ASTNode* parent, HorseIRParser::TypeCaseWildcardContext* cst, ASTNode::MemManagerType& mem) ;
            WildcardType(HorseIRParser::TypeCaseWildcardContext* cst, ASTNode::MemManagerType& mem) ;
            WildcardType(ASTNode::MemManagerType& mem) ;
            ~WildcardType() override = default ;
            
            virtual bool isGeneralizationOf(const Type* type) const override ;

            virtual std::size_t getNumNodesRecursively() const override ;
            virtual std::vector<ASTNode*> getChildren() const override ;
            virtual std::string toString() const override ;
            virtual std::string toTreeString() const override ;
        } ;

        class ListType : public Type {
        public:
            ListType() = delete ;
            ListType(ASTNode* parent, HorseIRParser::TypeCaseListContext* cst, ASTNode::MemManagerType& mem) ;
            ListType(HorseIRParser::TypeCaseListContext* cst, ASTNode::MemManagerType& mem) ;
            ListType(ASTNode::MemManagerType& mem) ;
            ~ListType() override = default ;

            virtual bool isGeneralizationOf(const Type* type) const override ;

            virtual std::size_t getNumNodesRecursively() const override ;
            virtual std::vector<ASTNode*> getChildren() const override ;
            virtual std::string toString() const override ;
            virtual std::string toTreeString() const override ;
            
            Type* getElementType() const ;
            ListType& setElementType(const Type* type) ;
        protected:
            Type* elementType ;
        } ;

        class DictionaryType : public Type {
        public:
            DictionaryType() = delete ;
            DictionaryType(ASTNode* parent, HorseIRParser::TypeCaseDictContext* cst, ASTNode::MemManagerType& mem) ;
            DictionaryType(HorseIRParser::TypeCaseDictContext* cst, ASTNode::MemManagerType& mem) ;
            DictionaryType(ASTNode::MemManagerType& mem) ;
            ~DictionaryType() override = default ;
            
            virtual bool isGeneralizationOf(const Type* type) const override ;

            virtual std::size_t getNumNodesRecursively() const override ;
            virtual std::vector<ASTNode*> getChildren() const override ;
            virtual std::string toString() const override ;
            virtual std::string toTreeString() const override ;
            
            Type* getKeyType() const ;
            Type* getValueType() const ;
            DictionaryType& setKeyType(const Type* type) ;
            DictionaryType& setValueType(const Type* type) ;
        protected:
            Type* keyType ;
            Type* valueType ;
        } ;
        
        class EnumerationType : public Type {
        public:
            EnumerationType() = delete ;
            EnumerationType(ASTNode* parent, HorseIRParser::TypeCaseEnumContext* cst, ASTNode::MemManagerType& mem) ;
            EnumerationType(HorseIRParser::TypeCaseEnumContext* cst, ASTNode::MemManagerType& mem) ;
            EnumerationType(ASTNode::MemManagerType& mem) ;
            ~EnumerationType() override = default ;
            
            virtual bool isGeneralizationOf(const Type* type) const override ;

            virtual std::size_t getNumNodesRecursively() const override ;
            virtual std::vector<ASTNode*> getChildren() const override ;
            virtual std::string toString() const override ;
            virtual std::string toTreeString() const override ;
            
            Type* getElementType() const ;
            EnumerationType& setElementType(const Type* type) ;
        protected:
            Type* elementType ;
        } ;

        class FunctionType : public Type {
        public:
            FunctionType() = delete ;
            FunctionType(ASTNode* parent, HorseIRParser::TypeCaseFuncContext* cst, ASTNode::MemManagerType& mem) ;
            FunctionType(HorseIRParser::TypeCaseFuncContext* cst, ASTNode::MemManagerType& mem) ;
            FunctionType(ASTNode::MemManagerType& mem) ;
            ~FunctionType() override = default ;
            
            virtual bool isGeneralizationOf(const Type* type) const override ;

            virtual std::size_t getNumNodesRecursively() const override ;
            virtual std::vector<ASTNode*> getChildren() const override ;
            virtual std::string toString() const override ;
            virtual std::string toTreeString() const override ;

            std::size_t getMinNumParameters() const ;
            bool isFlexible() const ;
            Type* getReturnType() const ;
            std::vector<Type*> getParameterTypes() const ;
            Type* getParameterTypeAt(std::size_t pos) const ;

            FunctionType& addParameterType(Type* type) ;
            FunctionType& setParameterTypeAt(std::size_t pos, Type* type) ;
            FunctionType& setFlexible(bool f) ;
            FunctionType& setReturnType(Type* type) ;
            FunctionType& truncateNumParameter(std::size_t num) ;
        protected:
            std::vector<Type*> parameterTypes ;
            bool flexible ;
            Type* returnType ;
        } ;
    }
}