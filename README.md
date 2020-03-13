# ClangPlayground
Here lies different examples of clang code.

To build this proyect you will need to install [LLVM](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm) and [Clang](https://clang.llvm.org/get_started.html). 
This proyect makes use and it's adapted to the version 11 of LLVM and Clang.

## Structure
This proyect contains 6 files. The first two files builds all the components from scratch. The other 4 make use of the [Compiler Instance](https://clang.llvm.org/doxygen/classclang_1_1CompilerInstance.html) object that provides a awesome API for faster
developing.

* [minimal_preprocessor_unit.cpp](https://github.com/roberteg16/ClangPlayground/blob/master/minimal_preprocessor_unit.cpp)
  * This file builds a [Preprocessor](https://clang.llvm.org/doxygen/classclang_1_1Preprocessor.html) from scratch. That's all.
* [feeding_files_to_preprocessor_unit.cpp](https://github.com/roberteg16/ClangPlayground/blob/master/feeding_files_to_preprocessor_unit.cpp)
  * This file is the continuation of the first one. The addition is that we parse a file. It only works with C files, so the test provided will not work here. You will have to write your own file. Notice that
  this preprocessor do not include any path so it can not find headers and therefore it will not work if the test has them included.
* [CI_Lexer.cpp](https://github.com/roberteg16/ClangPlayground/blob/master/CI_Lexer.cpp)
  * This file do exactly the same as the previous one but it uses the CompilerInstance interface to ease the code and comprehension. In case you need to be in total control and you need your compiler to be more configurable, 
  you should program it separately. This version includes the path to the headers so it can parse files with headers like \<iostream\> and others. Notice that these paths are my paths, you should include your own.
* [CI_Parser.cpp](https://github.com/roberteg16/ClangPlayground/blob/master/CI_Parser.cpp)
  * This file builds a [Parser](https://clang.llvm.org/doxygen/classclang_1_1Parser.html). After that it prints some stats.
* [CI_ASTConsumer.cpp](https://github.com/roberteg16/ClangPlayground/blob/master/CI_ASTConsumer.cpp)
  * This file shows how to build and plug an [ASTConsumer](https://clang.llvm.org/doxygen/classclang_1_1ASTConsumer.html). The overloaded function HandleTopLevelDecl is called each time a struct, variable, typedef, namespace, functions, etc is found at global scope.
  More functions that can be overloaded can be found in the documentation of the ATSConsumer. Each one gets called depending of what the parser find.
* [CI_ASTVisitor.cpp](https://github.com/roberteg16/ClangPlayground/blob/master/CI_ASTVisitor.cpp)
  * This file shows how to build and plug a [RecursiveASTVisitor](https://clang.llvm.org/doxygen/classclang_1_1RecursiveASTVisitor.html) to the parser. The RecursiveASTVisitor gets called once the [Parser](https://en.wikipedia.org/wiki/Parsing#Parser) is finished building the [Abstract Syntax Tree](https://en.wikipedia.org/wiki/Abstract_syntax_tree). 
  My implementation of the RecursiveASTVisitor overloads the function VisitStmt, that means that once the Parser is finished parsing the AST, each time it finds a [Stmt](https://clang.llvm.org/doxygen/classclang_1_1Stmt.html) it gets called. 
  This function adds braces to the if-statements that lack of them, for this porpouse it uses the help class [Rewriter](https://clang.llvm.org/doxygen/classclang_1_1Rewriter.html). 
  Other functions that you can overload are [these](https://clang.llvm.org/doxygen/classclang_1_1ASTDeclReader.html).
  
## Compilation
The LLVM and therfore the Clang proyect was build with a library structure in mind so each "component" we want we have to link it.

The command I use is below:
```
clang++ `llvm-config --cxxflags --ldflags` <file-in> -lclangCodeGen -lclangFrontend -lclangTooling -lclangFrontendTool -lclangDriver -lclangSerialization -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewrite -lclangRewriteFrontend -lclangEdit -lclangAST -lclangLex -lclangBasic -lclang `llvm-config --libs --system-libs` -o <file-out>
```

There is a possibility to configure CMAKE to build the proyect also. Here is the [link](https://llvm.org/docs/CMake.html).
  
