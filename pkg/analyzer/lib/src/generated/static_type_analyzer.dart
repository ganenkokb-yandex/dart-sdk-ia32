// Copyright (c) 2014, the Dart project authors. Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

import 'package:analyzer/dart/element/type.dart';
import 'package:analyzer/src/dart/ast/ast.dart';
import 'package:analyzer/src/dart/ast/extensions.dart';
import 'package:analyzer/src/dart/element/type.dart';
import 'package:analyzer/src/dart/element/type_provider.dart';
import 'package:analyzer/src/dart/element/type_system.dart';
import 'package:analyzer/src/dart/resolver/invocation_inference_helper.dart';
import 'package:analyzer/src/generated/resolver.dart';

/// Instances of the class `StaticTypeAnalyzer` perform two type-related tasks. First, they
/// compute the static type of every expression. Second, they look for any static type errors or
/// warnings that might need to be generated. The requirements for the type analyzer are:
/// <ol>
/// * Every element that refers to types should be fully populated.
/// * Every node representing an expression should be resolved to the Type of the expression.
/// </ol>
class StaticTypeAnalyzer {
  /// The resolver driving the resolution and type analysis.
  final ResolverVisitor _resolver;

  final InvocationInferenceHelper _inferenceHelper;

  /// The object providing access to the types defined by the language.
  late TypeProviderImpl _typeProvider;

  /// The type system in use for static type analysis.
  late TypeSystemImpl _typeSystem;

  /// The type representing the type 'dynamic'.
  late DartType _dynamicType;

  /// Initialize a newly created static type analyzer to analyze types for the
  /// [_resolver] based on the
  ///
  /// @param resolver the resolver driving this participant
  StaticTypeAnalyzer(this._resolver)
      : _inferenceHelper = _resolver.inferenceHelper {
    _typeProvider = _resolver.typeProvider;
    _typeSystem = _resolver.typeSystem;
    _dynamicType = _typeProvider.dynamicType;
  }

  /// The Dart Language Specification, 12.5: <blockquote>The static type of a string literal is
  /// `String`.</blockquote>
  void visitAdjacentStrings(covariant AdjacentStringsImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.stringType);
  }

  /// The Dart Language Specification, 12.32: <blockquote>... the cast expression <i>e as T</i> ...
  ///
  /// It is a static warning if <i>T</i> does not denote a type available in the current lexical
  /// scope.
  ///
  /// The static type of a cast expression <i>e as T</i> is <i>T</i>.</blockquote>
  void visitAsExpression(covariant AsExpressionImpl node) {
    _inferenceHelper.recordStaticType(node, _getType(node.type));
  }

  /// The Dart Language Specification, 16.29 (Await Expressions):
  ///
  ///   The static type of [the expression "await e"] is flatten(T) where T is
  ///   the static type of e.
  void visitAwaitExpression(covariant AwaitExpressionImpl node) {
    var resultType = node.expression.typeOrThrow;
    resultType = _typeSystem.flatten(resultType);
    _inferenceHelper.recordStaticType(node, resultType);
  }

  /// The Dart Language Specification, 12.4: <blockquote>The static type of a boolean literal is
  /// bool.</blockquote>
  void visitBooleanLiteral(covariant BooleanLiteralImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.boolType);
  }

  /// The Dart Language Specification, 12.15.2: <blockquote>A cascaded method invocation expression
  /// of the form <i>e..suffix</i> is equivalent to the expression <i>(t) {t.suffix; return
  /// t;}(e)</i>.</blockquote>
  void visitCascadeExpression(covariant CascadeExpressionImpl node) {
    _inferenceHelper.recordStaticType(node, node.target.typeOrThrow);
  }

  /// The Dart Language Specification, 12.19: <blockquote> ... a conditional expression <i>c</i> of
  /// the form <i>e<sub>1</sub> ? e<sub>2</sub> : e<sub>3</sub></i> ...
  ///
  /// It is a static type warning if the type of e<sub>1</sub> may not be assigned to `bool`.
  ///
  /// The static type of <i>c</i> is the least upper bound of the static type of <i>e<sub>2</sub></i>
  /// and the static type of <i>e<sub>3</sub></i>.</blockquote>
  void visitConditionalExpression(covariant ConditionalExpressionImpl node) {
    DartType staticType = _typeSystem.leastUpperBound(
        node.thenExpression.typeOrThrow, node.elseExpression.typeOrThrow);

    _inferenceHelper.recordStaticType(node, staticType);
  }

  /// The Dart Language Specification, 12.3: <blockquote>The static type of a literal double is
  /// double.</blockquote>
  void visitDoubleLiteral(covariant DoubleLiteralImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.doubleType);
  }

  void visitExtensionOverride(ExtensionOverride node) {
    assert(false,
        'Resolver should call extensionResolver.resolveOverride directly');
  }

  /// The Dart Language Specification, 12.9: <blockquote>The static type of a function literal of the
  /// form <i>(T<sub>1</sub> a<sub>1</sub>, &hellip;, T<sub>n</sub> a<sub>n</sub>, [T<sub>n+1</sub>
  /// x<sub>n+1</sub> = d1, &hellip;, T<sub>n+k</sub> x<sub>n+k</sub> = dk]) => e</i> is
  /// <i>(T<sub>1</sub>, &hellip;, Tn, [T<sub>n+1</sub> x<sub>n+1</sub>, &hellip;, T<sub>n+k</sub>
  /// x<sub>n+k</sub>]) &rarr; T<sub>0</sub></i>, where <i>T<sub>0</sub></i> is the static type of
  /// <i>e</i>. In any case where <i>T<sub>i</sub>, 1 &lt;= i &lt;= n</i>, is not specified, it is
  /// considered to have been specified as dynamic.
  ///
  /// The static type of a function literal of the form <i>(T<sub>1</sub> a<sub>1</sub>, &hellip;,
  /// T<sub>n</sub> a<sub>n</sub>, {T<sub>n+1</sub> x<sub>n+1</sub> : d1, &hellip;, T<sub>n+k</sub>
  /// x<sub>n+k</sub> : dk}) => e</i> is <i>(T<sub>1</sub>, &hellip;, T<sub>n</sub>, {T<sub>n+1</sub>
  /// x<sub>n+1</sub>, &hellip;, T<sub>n+k</sub> x<sub>n+k</sub>}) &rarr; T<sub>0</sub></i>, where
  /// <i>T<sub>0</sub></i> is the static type of <i>e</i>. In any case where <i>T<sub>i</sub>, 1
  /// &lt;= i &lt;= n</i>, is not specified, it is considered to have been specified as dynamic.
  ///
  /// The static type of a function literal of the form <i>(T<sub>1</sub> a<sub>1</sub>, &hellip;,
  /// T<sub>n</sub> a<sub>n</sub>, [T<sub>n+1</sub> x<sub>n+1</sub> = d1, &hellip;, T<sub>n+k</sub>
  /// x<sub>n+k</sub> = dk]) {s}</i> is <i>(T<sub>1</sub>, &hellip;, T<sub>n</sub>, [T<sub>n+1</sub>
  /// x<sub>n+1</sub>, &hellip;, T<sub>n+k</sub> x<sub>n+k</sub>]) &rarr; dynamic</i>. In any case
  /// where <i>T<sub>i</sub>, 1 &lt;= i &lt;= n</i>, is not specified, it is considered to have been
  /// specified as dynamic.
  ///
  /// The static type of a function literal of the form <i>(T<sub>1</sub> a<sub>1</sub>, &hellip;,
  /// T<sub>n</sub> a<sub>n</sub>, {T<sub>n+1</sub> x<sub>n+1</sub> : d1, &hellip;, T<sub>n+k</sub>
  /// x<sub>n+k</sub> : dk}) {s}</i> is <i>(T<sub>1</sub>, &hellip;, T<sub>n</sub>, {T<sub>n+1</sub>
  /// x<sub>n+1</sub>, &hellip;, T<sub>n+k</sub> x<sub>n+k</sub>}) &rarr; dynamic</i>. In any case
  /// where <i>T<sub>i</sub>, 1 &lt;= i &lt;= n</i>, is not specified, it is considered to have been
  /// specified as dynamic.</blockquote>
  void visitFunctionExpression(FunctionExpression node) {}

  void visitFunctionReference(covariant FunctionReferenceImpl node) {
    // TODO(paulberry): implement
    node.staticType = _dynamicType;
  }

  /// <blockquote>
  /// An integer literal has static type \code{int}, unless the surrounding
  /// static context type is a type which \code{int} is not assignable to, and
  /// \code{double} is. In that case the static type of the integer literal is
  /// \code{double}.
  /// <blockquote>
  ///
  /// and
  ///
  /// <blockquote>
  /// If $e$ is an expression of the form \code{-$l$} where $l$ is an integer
  /// literal (\ref{numbers}) with numeric integer value $i$, then the static
  /// type of $e$ is the same as the static type of an integer literal with the
  /// same contexttype
  /// </blockquote>
  void visitIntegerLiteral(IntegerLiteralImpl node,
      {required DartType? contextType}) {
    var strictCasts = _resolver.analysisOptions.strictCasts;
    if (contextType == null ||
        _typeSystem.isAssignableTo(_typeProvider.intType, contextType,
            strictCasts: strictCasts) ||
        !_typeSystem.isAssignableTo(_typeProvider.doubleType, contextType,
            strictCasts: strictCasts)) {
      _inferenceHelper.recordStaticType(node, _typeProvider.intType);
    } else {
      _inferenceHelper.recordStaticType(node, _typeProvider.doubleType);
    }
  }

  /// The Dart Language Specification, 12.31: <blockquote>It is a static warning if <i>T</i> does not
  /// denote a type available in the current lexical scope.
  ///
  /// The static type of an is-expression is `bool`.</blockquote>
  void visitIsExpression(covariant IsExpressionImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.boolType);
  }

  void visitMethodInvocation(MethodInvocation node) {
    throw StateError('Should not be invoked');
  }

  void visitNamedExpression(covariant NamedExpressionImpl node) {
    Expression expression = node.expression;
    _inferenceHelper.recordStaticType(node, expression.typeOrThrow);
  }

  /// The Dart Language Specification, 12.2: <blockquote>The static type of `null` is bottom.
  /// </blockquote>
  void visitNullLiteral(covariant NullLiteralImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.nullType);
  }

  void visitParenthesizedExpression(
      covariant ParenthesizedExpressionImpl node) {
    Expression expression = node.expression;
    _inferenceHelper.recordStaticType(node, expression.typeOrThrow);
  }

  /// The Dart Language Specification, 12.9: <blockquote>The static type of a rethrow expression is
  /// bottom.</blockquote>
  void visitRethrowExpression(covariant RethrowExpressionImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.bottomType);
  }

  /// The Dart Language Specification, 12.5: <blockquote>The static type of a string literal is
  /// `String`.</blockquote>
  void visitSimpleStringLiteral(covariant SimpleStringLiteralImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.stringType);
  }

  /// The Dart Language Specification, 12.5: <blockquote>The static type of a string literal is
  /// `String`.</blockquote>
  void visitStringInterpolation(covariant StringInterpolationImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.stringType);
  }

  void visitSuperExpression(covariant SuperExpressionImpl node) {
    var thisType = _resolver.thisType;
    _resolver.flowAnalysis.flow
        ?.thisOrSuper(node, thisType ?? _dynamicType, isSuper: true);
    if (thisType == null ||
        node.thisOrAncestorOfType<ExtensionDeclaration>() != null) {
      // TODO(brianwilkerson): Report this error if it hasn't already been
      // reported.
      _inferenceHelper.recordStaticType(node, InvalidTypeImpl.instance);
    } else {
      _inferenceHelper.recordStaticType(node, thisType);
    }
  }

  void visitSymbolLiteral(covariant SymbolLiteralImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.symbolType);
  }

  /// The Dart Language Specification, 12.10: <blockquote>The static type of `this` is the
  /// interface of the immediately enclosing class.</blockquote>
  void visitThisExpression(covariant ThisExpressionImpl node) {
    var thisType = _resolver.thisType;
    _resolver.flowAnalysis.flow
        ?.thisOrSuper(node, thisType ?? _dynamicType, isSuper: false);
    if (thisType == null) {
      // TODO(brianwilkerson): Report this error if it hasn't already been
      // reported.
      _inferenceHelper.recordStaticType(node, _dynamicType);
    } else {
      _inferenceHelper.recordStaticType(node, thisType);
    }
  }

  /// The Dart Language Specification, 12.8: <blockquote>The static type of a throw expression is
  /// bottom.</blockquote>
  void visitThrowExpression(covariant ThrowExpressionImpl node) {
    _inferenceHelper.recordStaticType(node, _typeProvider.bottomType);
  }

  /// Return the type represented by the given type [annotation].
  DartType _getType(TypeAnnotation annotation) {
    var type = annotation.type;
    if (type == null) {
      // TODO(brianwilkerson): Determine the conditions for which the type is
      // null.
      return _dynamicType;
    }
    return type;
  }
}
