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

#include "clang/Lex/Preprocessor.h"

int main() {
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> pDiagOpts;
    clang::TextDiagnosticPrinter *pTextDiagnosticPrinter =
        new clang::TextDiagnosticPrinter(llvm::outs(), pDiagOpts.get());

    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs;

    std::shared_ptr<clang::HeaderSearchOptions> hsOpts(
        new clang::HeaderSearchOptions());

    clang::FileSystemOptions fileSystemOptions;
    clang::FileManager fileManager(fileSystemOptions);

    std::unique_ptr<clang::DiagnosticsEngine> diagnosticsEngine(
        new clang::DiagnosticsEngine(pDiagIDs, pDiagOpts,
                                     pTextDiagnosticPrinter));

    clang::SourceManager sourceManager(*diagnosticsEngine, fileManager);

    clang::LangOptions languageOptions;

    std::shared_ptr<clang::TargetOptions> targetOptions(
        new clang::TargetOptions());
    targetOptions->Triple = llvm::sys::getDefaultTargetTriple();
    clang::TargetInfo *targetInfo =
        clang::TargetInfo::CreateTargetInfo(*diagnosticsEngine, targetOptions);

    clang::HeaderSearch headerSearch(hsOpts, //
                                     sourceManager, *diagnosticsEngine,
                                     languageOptions, targetInfo);

    clang::CompilerInstance compInstance;

    std::shared_ptr<clang::PreprocessorOptions> PPOpts =
        std::make_shared<clang::PreprocessorOptions>();

    clang::Preprocessor preprocessor(PPOpts, *diagnosticsEngine,
                                     languageOptions, sourceManager,
                                     headerSearch, compInstance);
}
