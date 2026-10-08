#include "novac/engine/source/PreprocessorController.hpp"

#include "novac/engine/foundation/registry/RegistryHelpers.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

namespace novac::source {

namespace {

/**
 * @brief Implements the `trim` operation.
 *
 * @param value Value supplied for `value`.
 * @return Value produced by the operation.
 */
std::string trim(std::string value) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    auto begin = std::find_if_not(value.begin(), value.end(), [&](char ch) { return isSpace(static_cast<unsigned char>(ch)); });
    auto end = std::find_if_not(value.rbegin(), value.rend(), [&](char ch) { return isSpace(static_cast<unsigned char>(ch)); }).base();
    if (begin >= end) {
        return {};
    }
    return std::string{begin, end};
}

/**
 * @brief Starts the operation represented by `startsWithAt`.
 *
 * @param text Value supplied for `text`.
 * @param pos Value supplied for `pos`.
 * @param needle Value supplied for `needle`.
 * @return Value produced by the operation.
 */
bool startsWithAt(const std::string &text, std::size_t pos, const std::string &needle) {
    return !needle.empty() && text.compare(pos, needle.size(), needle) == 0;
}

/**
 * @brief Adds data through `appendFragment`.
 *
 * @param output Value supplied for `output`.
 * @param text Value supplied for `text`.
 * @param origin Value supplied for `origin`.
 */
void appendFragment(
    std::vector<SourceFragment> &output,
    std::string text,
    diagnostics::SourceLocation origin) {
    if (text.empty()) {
        return;
    }

    if (!output.empty()) {
        SourceFragment &last{output.back()};
        if (last.origin.file == origin.file
            && last.origin.offset + last.text.size() == origin.offset) {
            last.text += std::move(text);
            return;
        }
    }

    output.push_back({std::move(text), std::move(origin)});
}

enum class BlockCommentState {
    None,
    Source,
    SuppressedDirective
};

struct LineScanResult {
    std::optional<Directive> directive{};
    BlockCommentState blockComment{BlockCommentState::None};
    std::size_t visibleStart{};
};

/**
 * @brief Implements the `scanLine` operation.
 *
 * @param source Value supplied for `source`.
 * @param line Value supplied for `line`.
 * @param lineOffset Value supplied for `lineOffset`.
 * @param lineNumber Value supplied for `lineNumber`.
 * @param blockComment Value supplied for `blockComment`.
 * @param lexerRules Value supplied for `lexerRules`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
LineScanResult scanLine(
    const Source &source,
    std::string_view line,
    std::size_t lineOffset,
    int lineNumber,
    BlockCommentState blockComment,
    const lexer::LexerRegistry &lexerRules,
    const PreprocessOptions &options) {
    bool seenCode{};
    bool inString{};
    bool escaped{};
    std::size_t visibleStart{};

    const std::string lineText{line};
    const std::string &lineComment{lexerRules.lineCommentPrefix()};
    const std::string &blockBegin{lexerRules.blockCommentBegin()};
    const std::string &blockEnd{lexerRules.blockCommentEnd()};

    for (std::size_t index{}; index < lineText.size();) {
        if (blockComment != BlockCommentState::None) {
            if (startsWithAt(lineText, index, blockEnd)) {
                const bool wasSuppressed{blockComment == BlockCommentState::SuppressedDirective};
                blockComment = BlockCommentState::None;
                index += blockEnd.size();
                if (wasSuppressed) {
                    visibleStart = index;
                }
            } else {
                ++index;
            }
            continue;
        }

        if (inString) {
            if (escaped) {
                escaped = false;
                ++index;
                continue;
            }
            if (lineText[index] == '\\') {
                escaped = true;
                ++index;
                continue;
            }
            if (lineText[index] == '"') {
                inString = false;
            }
            ++index;
            continue;
        }

        if (startsWithAt(lineText, index, lineComment)) {
            break;
        }
        if (startsWithAt(lineText, index, blockBegin)) {
            blockComment = BlockCommentState::Source;
            index += blockBegin.size();
            continue;
        }

        const unsigned char current{static_cast<unsigned char>(lineText[index])};
        if (std::isspace(current) != 0) {
            ++index;
            continue;
        }

        if (!seenCode && startsWithAt(lineText, index, options.directivePrefix)) {
            const std::size_t directiveStart{index};
            index += options.directivePrefix.size();
            while (index < lineText.size()
                && std::isspace(static_cast<unsigned char>(lineText[index])) != 0) {
                ++index;
            }

            const std::size_t nameStart{index};
            while (index < lineText.size()) {
                const unsigned char ch{static_cast<unsigned char>(lineText[index])};
                if (std::isalnum(ch) == 0 && ch != '_') {
                    break;
                }
                ++index;
            }

            if (nameStart == index) {
                throw std::runtime_error(
                    "PreprocessorController::process: expected directive name at line "
                    + std::to_string(lineNumber));
            }

            const std::string name{lineText.substr(nameStart, index - nameStart)};
            while (index < lineText.size()
                && std::isspace(static_cast<unsigned char>(lineText[index])) != 0) {
                ++index;
            }

            const std::size_t argumentsStart{index};
            std::size_t physicalEnd{lineText.size()};
            while (physicalEnd > 0 && (lineText[physicalEnd - 1] == '\n' || lineText[physicalEnd - 1] == '\r')) {
                --physicalEnd;
            }

            // Directive comments follow lexical whitespace semantics. Keep
            // same-line block comments as spaces so text after a closed
            // comment remains part of the directive and source offsets remain
            // aligned with the original line. A line comment still terminates
            // the directive.
            std::string argumentsText{lineText.substr(argumentsStart, physicalEnd - argumentsStart)};
            bool quoted{};
            bool quoteEscape{};
            std::size_t cursor{argumentsStart};

            while (cursor < physicalEnd) {
                if (quoted) {
                    if (quoteEscape) {
                        quoteEscape = false;
                    } else if (lineText[cursor] == '\\') {
                        quoteEscape = true;
                    } else if (lineText[cursor] == '"') {
                        quoted = false;
                    }
                    ++cursor;
                    continue;
                }

                if (startsWithAt(lineText, cursor, lineComment)) {
                    argumentsText.resize(cursor - argumentsStart);
                    break;
                }

                if (startsWithAt(lineText, cursor, blockBegin)) {
                    const std::size_t commentBegin{cursor};
                    cursor += blockBegin.size();
                    bool closed{};
                    while (cursor < physicalEnd) {
                        if (startsWithAt(lineText, cursor, blockEnd)) {
                            cursor += blockEnd.size();
                            closed = true;
                            break;
                        }
                        ++cursor;
                    }

                    const std::size_t maskedEnd{closed ? cursor : physicalEnd};
                    std::fill(
                        argumentsText.begin() + static_cast<std::ptrdiff_t>(commentBegin - argumentsStart),
                        argumentsText.begin() + static_cast<std::ptrdiff_t>(maskedEnd - argumentsStart),
                        ' ');

                    if (!closed) {
                        blockComment = BlockCommentState::SuppressedDirective;
                        break;
                    }
                    continue;
                }

                if (lineText[cursor] == '"') {
                    quoted = true;
                }
                ++cursor;
            }

            const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
            std::size_t argumentBegin{};
            while (argumentBegin < argumentsText.size()
                && isSpace(static_cast<unsigned char>(argumentsText[argumentBegin]))) {
                ++argumentBegin;
            }
            std::size_t argumentEnd{argumentsText.size()};
            while (argumentEnd > argumentBegin
                && isSpace(static_cast<unsigned char>(argumentsText[argumentEnd - 1]))) {
                --argumentEnd;
            }

            const std::size_t sourceArgumentBegin{argumentsStart + argumentBegin};
            const std::size_t sourceArgumentEnd{argumentsStart + argumentEnd};
            const std::string fileName{source.name.empty() ? source.id : source.name};
            Directive directive{};
            directive.name = name;
            directive.arguments = argumentsText.substr(argumentBegin, argumentEnd - argumentBegin);
            directive.span = {
                {fileName, lineOffset + directiveStart, lineNumber, static_cast<int>(directiveStart + 1)},
                {fileName, lineOffset + (directive.arguments.empty() ? nameStart + name.size() : sourceArgumentEnd),
                    lineNumber,
                    static_cast<int>((directive.arguments.empty() ? nameStart + name.size() : sourceArgumentEnd) + 1)}};
            directive.argumentsSpan = {
                {fileName, lineOffset + sourceArgumentBegin, lineNumber, static_cast<int>(sourceArgumentBegin + 1)},
                {fileName, lineOffset + sourceArgumentEnd, lineNumber, static_cast<int>(sourceArgumentEnd + 1)}};
            return {std::move(directive), blockComment, visibleStart};
        }

        seenCode = true;
        if (lineText[index] == '"') {
            inString = true;
        }
        ++index;
    }

    if (blockComment == BlockCommentState::SuppressedDirective) {
        visibleStart = lineText.size();
    }

    return {{}, blockComment, visibleStart};
}

struct SplitWordResult {
    std::string first{};
    std::string rest{};
    std::size_t restOffset{};
};

/**
 * @brief Implements the `splitFirstWord` operation.
 *
 * @param text Value supplied for `text`.
 * @return Value produced by the operation.
 */
SplitWordResult splitFirstWord(const std::string &text) {
    std::size_t index{};
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) != 0) {
        ++index;
    }
    const std::size_t begin{index};
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) == 0) {
        ++index;
    }

    std::string first{text.substr(begin, index - begin)};
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) != 0) {
        ++index;
    }

    const std::size_t restOffset{index};
    std::string rest{index < text.size() ? trim(text.substr(index)) : std::string{}};
    return {std::move(first), std::move(rest), restOffset};
}

/**
 * @brief Implements the `sourceEndLocation` operation.
 *
 * @param source Value supplied for `source`.
 * @return Value produced by the operation.
 */
diagnostics::SourceLocation sourceEndLocation(const Source &source) {
    diagnostics::SourceLocation location{
        source.name.empty() ? source.id : source.name,
        0,
        1,
        1};

    for (char ch : source.text) {
        ++location.offset;
        if (ch == '\n') {
            ++location.line;
            location.column = 1;
        } else {
            ++location.column;
        }
    }

    return location;
}

} // namespace

/**
 * @brief Constructs a `PreprocessorContext` instance.
 *
 * @param source Value supplied for `source`.
 * @param diagnostics Value supplied for `diagnostics`.
 * @param defines Value supplied for `defines`.
 * @param conditionals Value supplied for `conditionals`.
 * @param conditionalFloor Value supplied for `conditionalFloor`.
 * @param output Value supplied for `output`.
 * @param includeCallback Value supplied for `includeCallback`.
 */
PreprocessorContext::PreprocessorContext(
    const Source &source,
    diagnostics::DiagnosticEngine &diagnostics,
    std::unordered_set<std::string> &defines,
    std::vector<ConditionalFrame> &conditionals,
    std::size_t conditionalFloor,
    std::vector<SourceFragment> &output,
    IncludeCallback includeCallback)
    : source_{&source}, diagnostics_{&diagnostics}, defines_{&defines}, conditionals_{&conditionals},
      conditionalFloor_{conditionalFloor}, output_{&output}, includeCallback_{std::move(includeCallback)} {}

/**
 * @brief Implements the `source` operation.
 *
 * @return Value produced by the operation.
 */
const Source &PreprocessorContext::source() const {
    return *source_;
}

/**
 * @brief Implements the `diagnostics` operation.
 *
 * @return Value produced by the operation.
 */
diagnostics::DiagnosticEngine &PreprocessorContext::diagnostics() {
    return *diagnostics_;
}

/**
 * @brief Creates a value through `define`.
 *
 * @param name Value supplied for `name`.
 */
void PreprocessorContext::define(std::string name) {
    if (name.empty()) {
        throw std::runtime_error("PreprocessorContext::define: symbol cannot be empty");
    }
    defines_->insert(std::move(name));
}

/**
 * @brief Implements the `undefine` operation.
 *
 * @param name Value supplied for `name`.
 */
void PreprocessorContext::undefine(const std::string &name) {
    if (name.empty()) {
        throw std::runtime_error("PreprocessorContext::undefine: symbol cannot be empty");
    }
    defines_->erase(name);
}

/**
 * @brief Creates a value through `defined`.
 *
 * @param name Value supplied for `name`.
 * @return Value produced by the operation.
 */
bool PreprocessorContext::defined(const std::string &name) const {
    return defines_->find(name) != defines_->end();
}

/**
 * @brief Checks the condition represented by `active`.
 *
 * @return Value produced by the operation.
 */
bool PreprocessorContext::active() const {
    if (conditionals_->empty()) {
        return true;
    }
    const ConditionalFrame &frame{conditionals_->back()};
    const bool branch{frame.alternateSeen ? !frame.condition : frame.condition};
    return frame.parentActive && branch;
}

/**
 * @brief Implements the `pushCondition` operation.
 *
 * @param value Value supplied for `value`.
 */
void PreprocessorContext::pushCondition(bool value) {
    conditionals_->push_back({active(), value, false});
}

/**
 * @brief Implements the `alternateCondition` operation.
 */
void PreprocessorContext::alternateCondition() {
    if (conditionals_->size() <= conditionalFloor_) {
        throw std::runtime_error("PreprocessorContext::alternateCondition: #else without matching conditional");
    }
    ConditionalFrame &frame{conditionals_->back()};
    if (frame.alternateSeen) {
        throw std::runtime_error("PreprocessorContext::alternateCondition: duplicate #else");
    }
    frame.alternateSeen = true;
}

/**
 * @brief Implements the `popCondition` operation.
 */
void PreprocessorContext::popCondition() {
    if (conditionals_->size() <= conditionalFloor_) {
        throw std::runtime_error("PreprocessorContext::popCondition: #endif without matching conditional");
    }
    conditionals_->pop_back();
}

/**
 * @brief Implements the `include` operation.
 *
 * @param specifier Value supplied for `specifier`.
 */
void PreprocessorContext::include(const std::string &specifier) {
    includeCallback_(specifier, false);
}

/**
 * @brief Implements the `import` operation.
 *
 * @param specifier Value supplied for `specifier`.
 */
void PreprocessorContext::import(const std::string &specifier) {
    includeCallback_(specifier, true);
}

/**
 * @brief Emits output through `emit`.
 *
 * @param text Value supplied for `text`.
 * @param origin Value supplied for `origin`.
 */
void PreprocessorContext::emit(std::string text, diagnostics::SourceLocation origin) {
    appendFragment(*output_, std::move(text), std::move(origin));
}

/**
 * @brief Constructs a `PreprocessorController` instance.
 *
 * @param duplicatePolicy Value supplied for `duplicatePolicy`.
 */
PreprocessorController::PreprocessorController(registry::DuplicatePolicy duplicatePolicy)
    : directives_{}, pragmas_{}, duplicatePolicy_{duplicatePolicy} {}

/**
 * @brief Implements the `directive` operation.
 *
 * @param name Value supplied for `name`.
 * @param handler Value supplied for `handler`.
 * @param mode Value supplied for `mode`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus PreprocessorController::directive(
    std::string name,
    DirectiveHandler handler,
    DirectiveMode mode) {
    if (name.empty()) {
        throw std::runtime_error("PreprocessorController::directive: name cannot be empty");
    }
    if (!handler) {
        throw std::runtime_error("PreprocessorController::directive: handler cannot be empty");
    }
    if (name == "pragma") {
        throw std::runtime_error("PreprocessorController::directive: 'pragma' is reserved for pragma dispatch");
    }

    return registry::registerEntry(
        directives_, std::move(name), DirectiveEntry{std::move(handler), mode},
        duplicatePolicy_, "PreprocessorController::directive");
}

/**
 * @brief Implements the `pragma` operation.
 *
 * @param name Value supplied for `name`.
 * @param handler Value supplied for `handler`.
 * @return Value produced by the operation.
 */
registry::RegisterStatus PreprocessorController::pragma(std::string name, PragmaHandler handler) {
    if (name.empty()) {
        throw std::runtime_error("PreprocessorController::pragma: name cannot be empty");
    }
    if (!handler) {
        throw std::runtime_error("PreprocessorController::pragma: handler cannot be empty");
    }
    return registry::registerEntry(
        pragmas_, std::move(name), std::move(handler), duplicatePolicy_, "PreprocessorController::pragma");
}

/**
 * @brief Implements the `process` operation.
 *
 * @param root Value supplied for `root`.
 * @param sources Value supplied for `sources`.
 * @param lexerRules Value supplied for `lexerRules`.
 * @param diagnostics Value supplied for `diagnostics`.
 * @param options Value supplied for `options`.
 * @return Value produced by the operation.
 */
PreprocessedSource PreprocessorController::process(
    const Source &root,
    const SourceController &sources,
    const lexer::LexerRegistry &lexerRules,
    diagnostics::DiagnosticEngine &diagnostics,
    PreprocessOptions options) const {
    if (root.id.empty()) {
        throw std::runtime_error("PreprocessorController::process: source id cannot be empty");
    }
    if (options.directivePrefix.empty()) {
        throw std::runtime_error("PreprocessorController::process: directive prefix cannot be empty");
    }

    std::unordered_set<std::string> defines{};
    for (const std::string &name : options.defines) {
        if (name.empty()) {
            throw std::runtime_error("PreprocessorController::process: predefined symbol cannot be empty");
        }
        defines.insert(name);
    }

    std::vector<PreprocessorContext::ConditionalFrame> conditionals{};
    std::unordered_set<std::string> imported{};
    std::vector<std::string> sourceStack{};
    std::vector<SourceFragment> output{};

    std::function<void(const Source &)> processSource;
    processSource = [&](const Source &current) {
        if (current.id.empty()) {
            throw std::runtime_error("PreprocessorController::process: included source id cannot be empty");
        }
        if (std::find(sourceStack.begin(), sourceStack.end(), current.id) != sourceStack.end()) {
            std::string chain{};
            for (const std::string &id : sourceStack) {
                if (!chain.empty()) chain += " -> ";
                chain += id;
            }
            if (!chain.empty()) chain += " -> ";
            chain += current.id;
            throw std::runtime_error("PreprocessorController::process: cyclic source inclusion detected: " + chain);
        }
        if (options.maxIncludeDepth != 0 && sourceStack.size() > options.maxIncludeDepth) {
            throw std::runtime_error("PreprocessorController::process: maximum include depth exceeded");
        }

        sourceStack.push_back(current.id);
        const std::size_t conditionalFloor{conditionals.size()};

        PreprocessorContext context{
            current,
            diagnostics,
            defines,
            conditionals,
            conditionalFloor,
            output,
            [&](const std::string &specifier, bool importOnce) {
                Source resolved{sources.resolve({specifier, current.id})};
                if (importOnce && imported.find(resolved.id) != imported.end()) {
                    return;
                }

                // Mark imports only after successful processing. Besides keeping
                // failed imports retryable, this lets processSource detect an
                // active import cycle instead of silently treating it as an
                // already-completed import.
                processSource(resolved);

                if (importOnce) {
                    imported.insert(resolved.id);
                }
            }};

        BlockCommentState blockComment{BlockCommentState::None};
        std::size_t offset{};
        int lineNumber{1};
        while (offset < current.text.size()) {
            const std::size_t newline{current.text.find('\n', offset)};
            const std::size_t end{newline == std::string::npos ? current.text.size() : newline + 1};
            const std::string_view line{current.text.data() + offset, end - offset};

            LineScanResult scanned{scanLine(
                current, line, offset, lineNumber, blockComment, lexerRules, options)};
            blockComment = scanned.blockComment;

            if (scanned.directive) {
                Directive directiveValue{std::move(*scanned.directive)};
                const bool activeBeforeDirective{context.active()};
                const std::size_t directiveOffset{directiveValue.span.begin.offset - offset};
                if (activeBeforeDirective && directiveOffset > scanned.visibleStart) {
                    const std::string fileName{current.name.empty() ? current.id : current.name};
                    appendFragment(
                        output,
                        std::string{line.substr(scanned.visibleStart, directiveOffset - scanned.visibleStart)},
                        diagnostics::SourceLocation{
                            fileName,
                            offset + scanned.visibleStart,
                            lineNumber,
                            static_cast<int>(scanned.visibleStart + 1)});
                }

                if (directiveValue.name == "pragma") {
                    if (context.active()) {
                        SplitWordResult pragmaParts{splitFirstWord(directiveValue.arguments)};
                        if (pragmaParts.first.empty()) {
                            throw std::runtime_error("PreprocessorController::process: pragma name cannot be empty");
                        }
                        const auto pragmaIter{pragmas_.find(pragmaParts.first)};
                        if (pragmaIter == pragmas_.end()) {
                            throw std::runtime_error(
                                "PreprocessorController::process: unknown pragma '" + pragmaParts.first + "'");
                        }

                        directiveValue.arguments = std::move(pragmaParts.rest);
                        directiveValue.argumentsSpan.begin.offset += pragmaParts.restOffset;
                        directiveValue.argumentsSpan.begin.column += static_cast<int>(pragmaParts.restOffset);
                        directiveValue.argumentsSpan.end = directiveValue.argumentsSpan.begin;
                        directiveValue.argumentsSpan.end.offset += directiveValue.arguments.size();
                        directiveValue.argumentsSpan.end.column += static_cast<int>(directiveValue.arguments.size());
                        pragmaIter->second(directiveValue, context);
                    }
                } else {
                    const auto iter{directives_.find(directiveValue.name)};
                    if (iter == directives_.end()) {
                        if (context.active()) {
                            throw std::runtime_error(
                                "PreprocessorController::process: unknown directive '" + directiveValue.name + "'");
                        }
                    } else if (iter->second.mode == DirectiveMode::Always || context.active()) {
                        iter->second.handler(directiveValue, context);
                    }
                }
            } else if (context.active() && scanned.visibleStart < line.size()) {
                const std::string fileName{current.name.empty() ? current.id : current.name};
                appendFragment(
                    output,
                    std::string{line.substr(scanned.visibleStart)},
                    diagnostics::SourceLocation{
                        fileName,
                        offset + scanned.visibleStart,
                        lineNumber,
                        static_cast<int>(scanned.visibleStart + 1)});
            }

            offset = end;
            ++lineNumber;
        }

        if (blockComment == BlockCommentState::SuppressedDirective) {
            throw std::runtime_error(
                "PreprocessorController::process: unterminated block comment in directive from source '"
                + current.id + "'");
        }

        if (conditionals.size() != conditionalFloor) {
            throw std::runtime_error(
                "PreprocessorController::process: unterminated conditional in source '" + current.id + "'");
        }

        sourceStack.pop_back();
    };

    processSource(root);
    return {std::move(output), sourceEndLocation(root)};
}

} // namespace novac::source
