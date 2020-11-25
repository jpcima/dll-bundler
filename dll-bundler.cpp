// SPDX-License-Identifier: BSL-1.0

#include <llvm/Object/COFF.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringSet.h>
#include <getopt.h>
#include <vector>
#include <queue>
#include <string>

static llvm::ErrorOr<std::vector<std::string>> getDllImports(llvm::StringRef filePath, llvm::Triple::ArchType *fileArch = nullptr);
static std::string findImport(llvm::StringRef dllImport, llvm::Triple::ArchType dllArch, const std::vector<std::string> &searchPaths);
static bool checkFileArchitecture(llvm::StringRef filePath, llvm::Triple::ArchType dllArch);

int main(int argc, char *argv[])
{
    std::vector<std::string> dllSearchPaths;
    bool wantHelp = false;

    if (argc < 2)
        wantHelp = true;
    else {
        for (int c; (c = getopt(argc, argv, "hL:")) != -1;) {
            switch (c) {
            case 'h':
                wantHelp = true;
                break;
            case 'L':
                dllSearchPaths.push_back(optarg);
                break;
            default:
                return -1;
            }
        }
    }

    if (wantHelp) {
        llvm::outs() << "Usage: dll-bundler [-L dll-search-path]... <exe-or-dll>\n";
        return 0;
    }

    if (argc - optind != 1) {
        llvm::errs() << "Please indicate the binary file.\n";
        return 1;
    }

    if (dllSearchPaths.empty()) {
        llvm::errs() << "Please indicate at least one DLL search path.\n";
        return 1;
    }

    std::string rootBinaryFile = argv[optind];
    llvm::StringRef rootBinaryDir = llvm::sys::path::parent_path(rootBinaryFile);
    llvm::Triple::ArchType dllArch = llvm::Triple::ArchType::UnknownArch;

    auto dllImportsOrError = getDllImports(rootBinaryFile, &dllArch);
    if (std::error_code ec = dllImportsOrError.getError()) {
        llvm::errs() << ec.message() << "\n";
        return 1;
    }

    llvm::StringSet<> processed;
    std::queue<std::string> toProcess;

    for (const std::string &import : dllImportsOrError.get())
        toProcess.push(import);

    while (!toProcess.empty()) {
        std::string import = llvm::StringRef(toProcess.front()).lower();
        toProcess.pop();

        if (!processed.insert(import).second)
            continue;

        std::string fullPath = findImport(import, dllArch, dllSearchPaths);
        if (fullPath.empty())
            continue;

        llvm::SmallVector<char, 256> destinationPath;
        llvm::sys::path::append(destinationPath, rootBinaryDir, llvm::sys::path::filename(fullPath));

        llvm::errs() << fullPath << " -> " << destinationPath << "\n";
        llvm::sys::fs::copy_file(fullPath, destinationPath);

        dllImportsOrError = getDllImports(fullPath);
        if (std::error_code ec = dllImportsOrError.getError())
            ; // ignore and go on
        else {
            for (const std::string &import : dllImportsOrError.get())
                toProcess.push(import);
        }
    }

    return 0;
}

static llvm::ErrorOr<std::vector<std::string>> getDllImports(llvm::StringRef filePath, llvm::Triple::ArchType *fileArch)
{
    std::error_code ec;
    std::vector<std::string> imports;

    auto sourceOrError = llvm::MemoryBuffer::getFile(filePath);
    if ((ec = sourceOrError.getError()))
        return ec;

    llvm::object::COFFObjectFile obj(*sourceOrError.get(), ec);
    if (ec)
        return ec;

    if (fileArch)
        *fileArch = obj.getArch();

    for (auto &dir : obj.import_directories()) {
        llvm::StringRef name;
        ec = dir.getName(name);
        if (ec)
            llvm::errs() << ec.message() << "\n";
        else
            imports.emplace_back(name);
    }

    for (auto &dir : obj.delay_import_directories()) {
        llvm::StringRef name;
        ec = dir.getName(name);
        if (ec)
            llvm::errs() << ec.message() << "\n";
        else
            imports.emplace_back(name);
    }

    return imports;
}

std::string findImport(llvm::StringRef dllImport, llvm::Triple::ArchType dllArch, const std::vector<std::string> &searchPaths)
{
    for (llvm::StringRef dir : searchPaths) {
        std::error_code ec;
        llvm::sys::fs::directory_iterator it(dir, ec);
        if (ec)
            continue;
        while (it != llvm::sys::fs::directory_iterator()) {
            const llvm::sys::fs::directory_entry &ent = *it;
            llvm::StringRef filePath = ent.path();
            llvm::StringRef fileName = llvm::sys::path::filename(filePath);
            if (fileName.equals_lower(dllImport)) {
                if (!checkFileArchitecture(filePath, dllArch))
                    llvm::errs() << "Skipped: " << filePath << "\n";
                else
                    return std::string(filePath);
            }
            it.increment(ec);
            if (ec)
                break;
        }
    }
    return std::string();
}

bool checkFileArchitecture(llvm::StringRef filePath, llvm::Triple::ArchType dllArch)
{
    std::error_code ec;

    auto sourceOrError = llvm::MemoryBuffer::getFile(filePath);
    if ((ec = sourceOrError.getError()))
        return false;

    llvm::object::COFFObjectFile obj(*sourceOrError.get(), ec);
    if (ec)
        return false;

    return obj.getArch() == dllArch;
}
