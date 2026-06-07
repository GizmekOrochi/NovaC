#include "../../include/compiler/Compiler.hpp"

#include "../../include/lexer/Lexer.hpp"
#include "../../include/parser/Parser.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace novac::compiler {

Compiler::Compiler(const language::Language &language, std::string startDomain)
    : language_{language}, startDomain_{std::move(startDomain)} {}

ast::NodePtr Compiler::parse(const std::string &source) const {
    lexer::Lexer lexer{language_.lexer};
    std::vector<token::Token> tokens{lexer.tokenize(source)};

    parser::Parser parser{language_.parser, startDomain_};

    return parser.parse(std::move(tokens));
}

ir::HIRModule Compiler::lowerToHIR(const ast::Node &root) const {
    ir::ASTLoweringPass pass{language_.lowering};

    return pass.lower(root);
}

ir::HIRModule Compiler::lowerToHIR(const std::string &source) const {
    const ast::NodePtr root{parse(source)};

    if (!root) {
        throw std::runtime_error(
            "Compiler::lowerToHIR: parser returned null AST");
    }

    return lowerToHIR(*root);
}

ir::MIRModule Compiler::lowerToMIR(const ast::Node &root) const {
    const ir::HIRModule hir{lowerToHIR(root)};

    ir::HIRLoweringPass pass{language_.lowering};

    return pass.lower(hir);
}

ir::MIRModule Compiler::lowerToMIR(const std::string &source) const {
    const ast::NodePtr root{parse(source)};

    if (!root) {
        throw std::runtime_error(
            "Compiler::lowerToMIR: parser returned null AST");
    }

    return lowerToMIR(*root);
}

runtime::Value Compiler::run(const ast::Node &root) const {
    runtime::Runtime runtime{language_.runtime};

    return runtime.run(root);
}

runtime::Value Compiler::run(const std::string &source) const {
    const ast::NodePtr root{parse(source)};

    if (!root) {
        throw std::runtime_error(
            "Compiler::run: parser returned null AST");
    }

    return run(*root);
}

bool Compiler::emit(const ast::Node &root, const std::string &backendName) const {
    std::unique_ptr<backend::Backend> backend{language_.backends.create(backendName)};

    if (!backend) {
        return false;
    }

    const ir::MIRModule mir{lowerToMIR(root)};
    backend->emit(mir);

    return true;
}

bool Compiler::emit(const std::string &source, const std::string &backendName) const {
    const ast::NodePtr root{parse(source)};

    if (!root) {
        throw std::runtime_error(
            "Compiler::emit: parser returned null AST");
    }

    return emit(*root, backendName);
}

} // namespace novac::compiler