#include <iostream>
#include <memory>

#include "clang/Basic/LLVM.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "clang/Basic/FileSystemOptions.h"
#include "clang/Basic/LangOptions.h"

#include "clang/Basic/Diagnostic.h" //DiagnosticsEngine
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/HeaderSearchOptions.h"

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PreprocessorOptions.h"

#include "clang/CodeGen/ObjectFilePCHContainerOperations.h" // Implementaton of PCHContainerReader
#include "clang/Lex/Preprocessor.h"
#include "clang/Serialization/PCHContainerOperations.h" // PCHContainerReader

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Parse/ParseAST.h"

#include "clang/Rewrite/Core/Rewriter.h"

class MyASTVisitor : public clang::RecursiveASTVisitor<MyASTVisitor> {
  public:
    MyASTVisitor(clang::Rewriter &R) : R(R){};

    bool VisitStmt(clang::Stmt *s) {

        if (llvm::isa<clang::IfStmt>(s)) {
            clang::IfStmt *IfStatement = llvm::cast<clang::IfStmt>(s);
            add_braces(IfStatement->getThen());

            clang::Stmt *elseStmt = IfStatement->getElse();
            if (elseStmt) {
                add_braces(elseStmt);
            }
        }

        return true;
    }

  private:
    void add_braces(clang::Stmt *stmt) {
        if (!llvm::isa<clang::CompoundStmt>(stmt)) {

            // Add braces
            clang::SourceLocation loc = stmt->getBeginLoc();
            R.InsertText(loc, "{\n", true, true);
            loc = stmt->getEndLoc();

            // + 1 for ;
            auto offset = clang::Lexer::MeasureTokenLength(
                              loc, R.getSourceMgr(), R.getLangOpts()) +
                          1;
            loc = loc.getLocWithOffset(offset);
            R.InsertTextAfterToken(loc, "\n}");
        }
    }

    clang::Rewriter &R;
};

class MyASTConsumer : public clang::ASTConsumer {
  public:
    MyASTConsumer(clang::Rewriter &R) : visitor(R) {}

    // Maneja las declaraciones de alto nivel. Structs, variables, typedefs,
    // namespaces, funciones... pero únicamente de scope global.
    /*		virtual bool HandleTopLevelDecl(clang::DeclGroupRef d) override
       { for(auto it = std::begin(d); it != std::end(d); it++) {

                                    clang::VarDecl *vd =
       llvm::dyn_cast<clang::VarDecl>(*it); std::cout <<
       (*it)->getDeclKindName() << std::endl; if(!vd) continue; if(
       vd->isFileVarDecl() && !vd->hasExternalStorage() ) { std::cerr << "Read
       top-level variable decl: '"; std::cerr << vd->getDeclName().getAsString()
       ; std::cerr << std::endl;
                                    }
                            }
                            return true;
                    }
    */
    // Se llama a este callback cuando todo el AST de la unidad ha sido parseado
    // por ello se lo pasamos al visitador para hacer lo que queramos
    virtual void HandleTranslationUnit(clang::ASTContext &context) override {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

  private:
    MyASTVisitor visitor;
};

int main() {
    clang::CompilerInstance ci;

    clang::CompilerInvocation &invocation = ci.getInvocation();

    ci.createDiagnostics();
    std::shared_ptr<clang::TargetOptions> targetOptions =
        std::make_shared<clang::TargetOptions>();
    targetOptions->Triple = llvm::sys::getDefaultTargetTriple();

    clang::TargetInfo *targetInfo =
        clang::TargetInfo::CreateTargetInfo(ci.getDiagnostics(), targetOptions);
    ci.setTarget(targetInfo);
    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());

    clang::LangOptions lo;
    lo.CPlusPlus = true;
    lo.CPlusPlus11 = true;
    lo.GNUMode = true;
    lo.Bool = true;

    clang::PreprocessorOptions &PPOpts = ci.getPreprocessorOpts();

    clang::InputKind ik(clang::Language::CXX);
    llvm::Triple triple(targetOptions->Triple);
    invocation.setLangDefaults(lo, ik, triple, PPOpts,
                               clang::LangStandard::Kind::lang_c11);

    ci.createPreprocessor(clang::TranslationUnitKind::TU_Complete);

    auto &headerSearch = ci.getPreprocessor().getHeaderSearchInfo();
    auto &searchOpts = headerSearch.getHeaderSearchOpts();

    searchOpts.UseBuiltinIncludes = false;
    searchOpts.UseStandardSystemIncludes = false;
    searchOpts.UseStandardCXXIncludes = false;
    searchOpts.Verbose = false;

    searchOpts.AddPath("/usr/include/c++/9", clang::frontend::Angled, false,
                       false);
    searchOpts.AddPath("/usr/include/x86_64-linux-gnu/c++/9",
                       clang::frontend::Angled, false, false);
    searchOpts.AddPath("/usr/include/c++/9/backward", clang::frontend::Angled,
                       false, false);
    searchOpts.AddPath("/usr/local/include", clang::frontend::Angled, false,
                       false);
    searchOpts.AddPath("/usr/local/lib/clang/11.0.0/include",
                       clang::frontend::Angled, false, false);
    searchOpts.AddPath("/usr/include/x86_64-linux-gnu", clang::frontend::Angled,
                       false, false);
    searchOpts.AddPath("/usr/include", clang::frontend::Angled, false, false);

    std::vector<clang::DirectoryLookup> lookups;
    for (auto entry : searchOpts.UserEntries) {
        auto expected = ci.getFileManager().getDirectoryRef(entry.Path);
        if (expected) {
            auto lookup = clang::DirectoryLookup(
                *expected, clang::SrcMgr::CharacteristicKind::C_System, false);
            if (!lookup.getDir()) {
                std::cerr << "Path no encontrado" << std::endl;
                exit(-1);
            }
            lookups.push_back(lookup);
        }
    }
    headerSearch.SetSearchPaths(lookups, 0, 0, true);

    std::unique_ptr<clang::PCHContainerReader> pch =
        std::make_unique<clang::ObjectFilePCHContainerReader>();
    clang::InitializePreprocessor(ci.getPreprocessor(),
                                  ci.getPreprocessorOpts(), *pch,
                                  ci.getFrontendOpts());

    ci.createASTContext();

    clang::Rewriter rewriter(ci.getSourceManager(), ci.getLangOpts());

    ci.setASTConsumer(std::make_unique<MyASTConsumer>(rewriter));
    ci.createSema(clang::TranslationUnitKind::TU_Complete, nullptr);

    const char *file = "tests/test.cpp";
    const llvm::ErrorOr<const clang::FileEntry *> tryFile =
        ci.getFileManager().getFile(file);
    if (std::error_code error = tryFile.getError()) {
        std::cerr << error.message() << std::endl;
        std::cerr << "File \"" << file << "\" does not exists." << std::endl;
        return -1;
    }
    const clang::FileEntry *pFile = tryFile.get();
    ci.getSourceManager().setMainFileID(ci.getSourceManager().createFileID(
        pFile, clang::SourceLocation(), clang::SrcMgr::C_User));

    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
                                             &ci.getPreprocessor());
    // Parse
    clang::ParseAST(ci.getSema());
    ci.getDiagnosticClient().EndSourceFile();
    const clang::RewriteBuffer *RewriteBuf =
        rewriter.getRewriteBufferFor(ci.getSourceManager().getMainFileID());
    std::cout << std::string(RewriteBuf->begin(), RewriteBuf->end());
    return 0;
}
