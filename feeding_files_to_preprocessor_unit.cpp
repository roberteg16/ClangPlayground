#include <iostream>
#include <memory>

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/LLVM.h" // llvm::outs, llvm::Expected
#include "llvm/Support/raw_ostream.h"

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

#include "clang/CodeGen/ObjectFilePCHContainerOperations.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Serialization/PCHContainerOperations.h"

int main() {
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> pDiagOpts;
    clang::TextDiagnosticPrinter *pTextDiagnosticPrinter =
        new clang::TextDiagnosticPrinter(llvm::outs(), pDiagOpts.get(), false);

    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> pDiagIDs;

    std::shared_ptr<clang::HeaderSearchOptions> hsOpts =
        std::make_shared<clang::HeaderSearchOptions>();

    clang::FileSystemOptions fileSystemOptions;
    clang::FileManager fileManager(fileSystemOptions);

    std::unique_ptr<clang::DiagnosticsEngine> diagnosticsEngine =
        std::make_unique<clang::DiagnosticsEngine>(pDiagIDs, pDiagOpts,
                                                   pTextDiagnosticPrinter);

    clang::SourceManager sourceManager(*diagnosticsEngine, fileManager);

    clang::LangOptions languageOptions;

    std::shared_ptr<clang::TargetOptions> targetOptions =
        std::make_shared<clang::TargetOptions>();
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

    preprocessor.Initialize(*targetInfo);

    clang::FrontendOptions frontendOptions;

    clang::ObjectFilePCHContainerReader ofpcr;

    clang::InitializePreprocessor(preprocessor, *PPOpts, ofpcr,
                                  frontendOptions);

    clang::ApplyHeaderSearchOptions(
        preprocessor.getHeaderSearchInfo(), compInstance.getHeaderSearchOpts(),
        preprocessor.getLangOpts(), preprocessor.getTargetInfo().getTriple());

    llvm::Expected<clang::FileEntryRef> exp_fileEntryRef =
        fileManager.getFileRef("tests/test.cpp");

    if (!exp_fileEntryRef) {
        std::cerr << "Error" << std::endl;
        exit(-1);
    }

    const clang::FileEntryRef fileEntryRef = exp_fileEntryRef.get();

    clang::FileID fileID = sourceManager.createFileID(
        fileEntryRef, clang::SourceLocation(), clang::SrcMgr::C_User);
    sourceManager.setMainFileID(fileID);
    preprocessor.EnterMainSourceFile();

    pTextDiagnosticPrinter->BeginSourceFile(languageOptions, &preprocessor);

    clang::Token token;
    do {
        preprocessor.Lex(token);

        if (diagnosticsEngine->hasErrorOccurred() ||
            diagnosticsEngine->hasFatalErrorOccurred() ||
            diagnosticsEngine->hasUnrecoverableErrorOccurred())
            break;

        preprocessor.DumpToken(token);
        std::cerr << std::endl;
    } while (token.isNot(clang::tok::eof));

    pTextDiagnosticPrinter->EndSourceFile();
}
