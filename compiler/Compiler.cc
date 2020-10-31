#include <Compiler.hh>

#include <CharStream.hh>
#include <Config.hh>
#include <IrGen.hh>
#include <Lexer.hh>
#include <Parser.hh>
#include <support/Error.hh>

#include <filesystem>
#include <fstream>

void Compiler::add_code(const std::string &path) {
    if (!m_visited.insert(path).second) {
        return;
    }

    bool std = path.starts_with("std");
    std::ifstream ifstream(std ? ROOT_PATH + path : path);
    if (!ifstream) {
        print_error_and_abort("Could not open file {}", path);
    }
    CharStream stream(&ifstream);
    Lexer lexer(&stream);
    Parser parser(&lexer);
    auto root = parser.parse();
    for (const auto *decl : root->decls()) {
        const auto *import_stmt = decl->as_or_null<ast::ImportStmt>();
        if (import_stmt == nullptr) {
            continue;
        }
        add_code(import_stmt->path());
    }
    m_roots.push_back(std::move(root));
}

std::unique_ptr<ir::Program> Compiler::compile(const std::string &main_path) {
    add_code("std/start.kd");
    add_code(main_path);
    return gen_ir(std::move(m_roots));
}
