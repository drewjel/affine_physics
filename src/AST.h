#ifndef AST_H
#define AST_H

#include "clang/AST/AST.h"
//#include "clang/AST/ASTConsumer.h"
//#include "clang/AST/Expr.h"
//#include "clang/AST/Stmt.h"


namespace ast {
// Scalar
using ScalarValue = double;


// Ident
using VecIdent = const clang::VarDecl;
using ScalarIdent = const clang::VarDecl;

// Expr
using VecExpr = const clang::Expr;
using VecLitExpr = const clang::CXXConstructExpr;
using VecVarExpr = const clang::DeclRefExpr;
using VecVecAddExpr = const clang::CXXMemberCallExpr;


using ScalarExpr = const clang::Expr;
using ScalarLitExpr = const clang::CXXConstructExpr;
using ScalarVarExpr = const clang::DeclRefExpr;


using ScalarScalarAddExpr = const clang::BinaryOperator;
using ScalarScalarMulExpr = const clang::BinaryOperator;

using VecScalarMulExpr = const clang::CXXMemberCallExpr;

using ScalarAssignExpr = const clang::CXXConstructExpr;
using VectorAssignExpr = const clang::CXXConstructExpr;


// KEVIN: Add for VecParenExpr module
using VecParenExpr = const clang::ParenExpr;

using ScalarParenExpr = const clang::ParenExpr;

// Value
using Vector = const clang::CXXConstructExpr;
using Vector_Lit = const clang::CXXConstructExpr;
using Vector_Var = const clang::CXXConstructExpr;
using Vector_Expr = const clang::CXXConstructExpr; // A Clang Stmt!

using Scalar = const clang::Expr;
using Scalar_Lit = const clang::Expr;
using Scalar_Var = const clang::Expr;
using Scalar_Expr = const clang::Expr;

using ScalarScalarAddExpr = const clang::BinaryOperator;
using ScalarScalarMulExpr = const clang::BinaryOperator;


// Def
using Vector_Def = const clang::DeclStmt;

using Scalar_Def = const clang::DeclStmt;

using Vector_Assign = const clang::CXXOperatorCallExpr;

using Scalar_Assign = const clang::BinaryOperator;

} // namespace

#endif
