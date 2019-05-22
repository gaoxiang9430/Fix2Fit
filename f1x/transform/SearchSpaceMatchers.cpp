/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Shin Hwei Tan, Abhik Roychoudhury

  f1x is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SearchSpaceMatchers.h"

using namespace clang;
using namespace ast_matchers;


// FIXME: how about unary negation?
StatementMatcher RepairableOperator = 
  anyOf(binaryOperator(anyOf(hasOperatorName("=="),
                             hasOperatorName("!="),
                             hasOperatorName("<="),
                             hasOperatorName(">="),
                             hasOperatorName(">"),
                             hasOperatorName("<"),
                             hasOperatorName("||"),
                             hasOperatorName("&&"))).bind(BOUND),
        binaryOperator(anyOf(hasOperatorName("+"),
                             hasOperatorName("-"),
                             hasOperatorName("*"),
                             hasOperatorName("%"),
                             hasOperatorName("/"))).bind(BOUND),
        unaryOperator(hasOperatorName("!")).bind(BOUND),
        binaryOperator(anyOf(hasOperatorName("&"),
                             hasOperatorName("|"),
                             hasOperatorName("^"),
                             hasOperatorName("<<"),
                             hasOperatorName(">>"))).bind(BOUND),
        unaryOperator(hasOperatorName("~")).bind(BOUND));
//        unaryOperator(hasOperatorName("*"), hasUnaryOperand(ignoringImpCasts(declRefExpr()))).bind(BOUND));

// FIXME: currently support only integer arrays:
StatementMatcher RepairableArraySubscript =
  arraySubscriptExpr(hasIndex(ignoringImpCasts(anyOf(integerLiteral(),
                                                     declRefExpr(),
                                                     memberExpr()))),
                     hasBase(implicitCastExpr(hasSourceExpression(declRefExpr(hasType(arrayType(hasElementType(isInteger())))))))).bind(BOUND);

StatementMatcher RepairableAtom =
  anyOf(declRefExpr(to(varDecl(anyOf(hasType(isInteger()),
                                     hasType(pointerType()))))).bind(BOUND),
        declRefExpr(to(enumConstantDecl())).bind(BOUND),
        declRefExpr(to(namedDecl())), // NOTE: no binding because it is only for member expression
        integerLiteral().bind(BOUND),
        characterLiteral().bind(BOUND),
        memberExpr(anyOf(hasType(isInteger()),
                         hasType(pointerType()))).bind(BOUND), // TODO: I need to make sure that base is a variable here?
        cStyleCastExpr(hasType(isInteger()), // TODO: for now only integer
                       hasSourceExpression(implicitCastExpr(hasSourceExpression(anyOf(integerLiteral(),
                                                                                      declRefExpr(),
                                                                                      memberExpr()))))).bind(BOUND),
        castExpr(hasType(asString("void *")), hasDescendant(integerLiteral(equals(0)))).bind(BOUND) // NULL, redundant?
        );
               
StatementMatcher RepairableNode =
  anyOf(RepairableOperator,
        RepairableArraySubscript,
        RepairableAtom);

StatementMatcher NonRepairableNode =
  unless(RepairableNode);

/*
  Matches 
  - integer and pointer variables
  - literals
  - arrays subscripts
  - memeber expressions
  - supported binary and pointer operators
 */
StatementMatcher BaseRepairableExpression =
  allOf(RepairableNode,
        unless(hasDescendant(expr(ignoringParenImpCasts(NonRepairableNode)))));

StatementMatcher NonBaseRepairableExpression =
  anyOf(unless(RepairableNode),
        hasDescendant(expr(ignoringParenImpCasts(NonRepairableNode))));

StatementMatcher Splittable =
  anyOf(binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(BaseRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(NonBaseRepairableExpression))),
        binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(NonBaseRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(BaseRepairableExpression))));

auto hasSplittableCondition =
  anyOf(hasCondition(ignoringParenImpCasts(BaseRepairableExpression)),
        eachOf(hasCondition(Splittable), hasCondition(forEachDescendant(Splittable))));

/*
  Matches condition (if, while, for) a part of which is BaseRepairableExpression
 */
StatementMatcher RepairableCondition = 
  anyOf(ifStmt(hasSplittableCondition),
        whileStmt(hasSplittableCondition),
        forStmt(hasSplittableCondition));

/*
  Matches RHS of assignments and compound assignments if it is BaseRepairableExpression
  FIXME: why to restrict to variables, members and arrays?
 */
StatementMatcher RepairableAssignment =
  binaryOperator(anyOf(hasOperatorName("="),
                       hasOperatorName("+="),
                       hasOperatorName("-="),
                       hasOperatorName("*="),
                       hasOperatorName("/="),
                       hasOperatorName("%="),
                       hasOperatorName("&="),
                       hasOperatorName("|="),
                       hasOperatorName("^="),
                       hasOperatorName("<<="),
                       hasOperatorName(">>=")),
                 anyOf(hasLHS(ignoringParenImpCasts(declRefExpr())),
                       hasLHS(ignoringParenImpCasts(memberExpr())),
                       hasLHS(ignoringParenImpCasts(arraySubscriptExpr()))),
                 hasRHS(ignoringParenImpCasts(BaseRepairableExpression)));

StatementMatcher RepairableReturn =
  returnStmt(has(expr(ignoringParenImpCasts(BaseRepairableExpression))));

StatementMatcher ExpressionSchemaMatcher =
  anyOf(RepairableCondition,
        RepairableAssignment,
        RepairableReturn);

StatementMatcher IfGuardSchemaMatcher =
  anyOf(allOf(unless(hasDescendant(expr(ignoringParenImpCasts(ExpressionSchemaMatcher)))),
              unless(hasDescendant(stmt(compoundStmt()))), //NOTE: should be stmtExpr, but it is unavailable in Clang 3.8.1
              callExpr().bind(BOUND)),
        breakStmt().bind(BOUND),
        continueStmt().bind(BOUND));

//TODO: support other kinds of conditions
StatementMatcher RefinementSchemaMatcher =
  anyOf(ifStmt(allOf(unless(hasSplittableCondition),
                     hasCondition(expr().bind(BOUND)))),
        whileStmt(allOf(unless(hasSplittableCondition),
                        hasCondition(expr().bind(BOUND)))),
        forStmt(allOf(unless(hasSplittableCondition),
                      hasCondition(expr().bind(BOUND)))));
  
