#include "cssparser.h"

#include <sstream>

namespace lunasvg {

constexpr bool isNameStart(char cc) { return isalpha(cc) || cc == '_'; }
constexpr bool isNameChar(char cc) { return isNameStart(cc) || isdigit(cc) || cc == '-'; }
constexpr bool isNewLine(char cc) { return (cc == '\n' || cc == '\r' || cc == '\f'); }
constexpr bool isNonPrintable(char cc) { return (cc >= 0 && cc <= 0x8) || cc == 0xb || (cc >= 0xf && cc <= 0x1f) || cc == 0x7f; }

CSSTokenStream CSSTokenizer::tokenize()
{
    while(true) {
        auto token = nextToken();
        if(token.type() == CSSToken::Type::Comment)
            continue;
        if(token.type() == CSSToken::Type::EndOfFile)
            break;
        m_tokenList.push_back(token);
    }

    auto begin = m_tokenList.data();
    auto end = begin + m_tokenList.size();
    return CSSTokenStream(begin, end);
}

bool CSSTokenizer::isEscapeSequence(char first, char second)
{
    return first == '\\' && !isNewLine(second);
}

bool CSSTokenizer::isIdentSequence(char first, char second, char third)
{
    if(isNameStart(first) || isEscapeSequence(first, second))
        return true;
    if(first == '-')
        return isNameStart(second) || second == '-' || isEscapeSequence(second, third);
    return false;
}

bool CSSTokenizer::isNumberSequence(char first, char second, char third)
{
    if(isdigit(first))
        return true;
    if(first == '-' || first == '+')
        return isdigit(second) || (second == '.' && isdigit(third));
    if(first == '.')
        return isdigit(second);
    return false;
}

bool CSSTokenizer::isEscapeSequence() const
{
    if(m_input.empty())
        return false;
    return isEscapeSequence(*m_input, m_input.peek(1));
}

bool CSSTokenizer::isIdentSequence() const
{
    if(m_input.empty())
        return false;
    auto second = m_input.peek(1);
    if(second == 0)
        return isIdentSequence(*m_input, 0, 0);
    return isIdentSequence(*m_input, second, m_input.peek(2));
}

bool CSSTokenizer::isNumberSequence() const
{
    if(m_input.empty())
        return false;
    auto second = m_input.peek(1);
    if(second == 0)
        return isNumberSequence(*m_input, 0, 0);
    return isNumberSequence(*m_input, second, m_input.peek(2));
}

bool CSSTokenizer::isExponentSequence() const
{
    if(m_input.peek() != 'E' && m_input.peek() != 'e')
        return false;
    if(m_input.peek(1) == '+' || m_input.peek(1) == '-')
        return isdigit(m_input.peek(2));
    return isdigit(m_input.peek(1));
}

std::string_view CSSTokenizer::substring(size_t offset, size_t count)
{
    return m_input.string(offset, count);
}

std::string_view CSSTokenizer::addstring(std::string&& value)
{
    m_stringList.push_back(std::move(value));
    return m_stringList.back();
}

std::string_view CSSTokenizer::consumeName()
{
    size_t count = 0;
    while(true) {
        auto cc = m_input.peek(count);
        if(cc == 0 || cc == '\\')
            break;
        if(!isNameChar(cc)) {
            auto offset = m_input.offset();
            m_input.advance(count);
            return substring(offset, count);
        }

        count += 1;
    }

    std::string output;
    while(true) {
        auto cc = m_input.peek();
        if(isNameChar(cc)) {
            output += cc;
            m_input.advance();
        } else if(isEscapeSequence()) {
            appendCodepoint(output, consumeEscape());
        } else {
            break;
        }
    }

    return addstring(std::move(output));
}

uint32_t CSSTokenizer::consumeEscape()
{
    assert(isEscapeSequence());
    auto cc = m_input.advance();
    if(isxdigit(cc)) {
        int count = 0;
        uint32_t cp = 0;
        do {
            cp = cp * 16 + xdigit(cc);
            cc = m_input.advance();
            count += 1;
        } while(count < 6 && isxdigit(cc));

        if(isspace(cc)) {
            if(cc == '\r' && m_input.peek(1) == '\n')
                m_input.advance();
            m_input.advance();
        }

        if(cp == 0 || cp >= 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return 0xFFFD;
        return cp;
    }

    if(cc == 0)
        return 0xFFFD;
    m_input.advance();
    return cc;
}

CSSToken CSSTokenizer::consumeStringToken()
{
    auto endingCodePoint = m_input.peek();
    assert(endingCodePoint == '\"' || endingCodePoint == '\'');
    m_input.advance();
    size_t count = 0;
    while(true) {
        auto cc = m_input.peek(count);
        if(cc == 0 || cc == '\\')
            break;
        if(cc == endingCodePoint) {
            auto offset = m_input.offset();
            m_input.advance(count);
            m_input.advance();
            return CSSToken(CSSToken::Type::String, substring(offset, count));
        }

        if(isNewLine(cc)) {
            m_input.advance(count);
            return CSSToken(CSSToken::Type::BadString);
        }

        count += 1;
    }

    std::string output;
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == endingCodePoint) {
            m_input.advance();
            break;
        }

        if(isNewLine(cc))
            return CSSToken(CSSToken::Type::BadString);

        if(cc == '\\') {
            auto next = m_input.peek(1);
            if(next == 0) {
                m_input.advance();
            } else if(isNewLine(next)) {
                if(next == '\r' && m_input.peek(2) == '\n')
                    m_input.advance();
                m_input.advance(2);
            } else {
                appendCodepoint(output, consumeEscape());
            }
        } else {
            output += cc;
            m_input.advance();
        }
    }

    if(output.empty())
        return CSSToken(CSSToken::Type::String);
    return CSSToken(CSSToken::Type::String, addstring(std::move(output)));
}

CSSToken CSSTokenizer::consumeNumericToken()
{
    assert(isNumberSequence());
    auto numberType = CSSToken::NumberType::Integer;
    auto numberSign = CSSToken::NumberSign::None;
    double fraction = 0;
    double integer = 0;
    int exponent = 0;
    int expsign = 1;

    if(m_input.peek() == '-') {
        numberSign = CSSToken::NumberSign::Minus;
        m_input.advance();
    } else if(m_input.peek() == '+') {
        numberSign = CSSToken::NumberSign::Plus;
        m_input.advance();
    }

    if(isdigit(m_input.peek())) {
        auto cc = m_input.peek();
        do {
            integer = 10.0 * integer + (cc - '0');
            cc = m_input.advance();
        } while(isdigit(cc));
    }

    if(m_input.peek() == '.' && isdigit(m_input.peek(1))) {
        numberType = CSSToken::NumberType::Number;
        auto cc = m_input.advance();
        int count = 0;
        do {
            fraction = 10.0 * fraction + (cc - '0');
            count += 1;
            cc = m_input.advance();
        } while(isdigit(cc));
        fraction *= std::pow(10.0, -count);
    }

    if(isExponentSequence()) {
        numberType = CSSToken::NumberType::Number;
        m_input.advance();
        if(m_input.peek() == '-') {
            expsign = -1;
            m_input.advance();
        } else if(m_input.peek() == '+') {
            m_input.advance();
        }

        auto cc = m_input.peek();
        do {
            exponent = 10 * exponent + (cc - '0');
            cc = m_input.advance();
        } while(isdigit(cc));
    }

    double number = (integer + fraction) * std::pow(10.0, exponent * expsign);
    if(numberSign == CSSToken::NumberSign::Minus)
        number = -number;

    if(m_input.peek() == '%') {
        m_input.advance();
        return CSSToken(CSSToken::Type::Percentage, numberType, numberSign, number);
    }

    if(isIdentSequence())
        return CSSToken(CSSToken::Type::Dimension, numberType, numberSign, number, consumeName());
    return CSSToken(CSSToken::Type::Number, numberType, numberSign, number);
}

CSSToken CSSTokenizer::consumeIdentLikeToken()
{
    auto name = consumeName();
    if(equals(name, "url", false) && m_input.peek() == '(') {
        auto cc = m_input.advance();
        while(isspace(cc) && isspace(m_input.peek(1))) {
            cc = m_input.advance();
        }

        if(isspace(cc))
            cc = m_input.peek(1);

        if(cc == '"' || cc == '\'')
            return CSSToken(CSSToken::Type::Function, name);
        return consumeUrlToken();
    }

    if(m_input.peek() == '(') {
        m_input.advance();
        return CSSToken(CSSToken::Type::Function, name);
    }

    return CSSToken(CSSToken::Type::Ident, name);
}

CSSToken CSSTokenizer::consumeUrlToken()
{
    auto cc = m_input.peek();
    while(isspace(cc)) {
        cc = m_input.advance();
    }

    size_t count = 0;
    while(true) {
        auto cc = m_input.peek(count);
        if(cc == 0 || cc == '\\' || isspace(cc))
            break;
        if(cc == ')') {
            auto offset = m_input.offset();
            m_input.advance(count);
            m_input.advance();
            return CSSToken(CSSToken::Type::Url, substring(offset, count));
        }

        if(cc == '"' || cc == '\'' || cc == '(' || isNonPrintable(cc)) {
            m_input.advance(count);
            return consumeBadUrlRemnants();
        }

        count += 1;
    }

    std::string output;
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == ')') {
            m_input.advance();
            break;
        }

        if(cc == '\\') {
            if(isEscapeSequence()) {
                appendCodepoint(output, consumeEscape());
                continue;
            }

            return consumeBadUrlRemnants();
        }

        if(isspace(cc)) {
            do {
                cc = m_input.advance();
            } while(isspace(cc));

            if(cc == 0)
                break;
            if(cc == ')') {
                m_input.advance();
                break;
            }

            return consumeBadUrlRemnants();
        }

        if(cc == '"' || cc == '\'' || cc == '(' || isNonPrintable(cc))
            return consumeBadUrlRemnants();

        output += cc;
        m_input.advance();
    }

    return CSSToken(CSSToken::Type::Url, addstring(std::move(output)));
}

CSSToken CSSTokenizer::consumeBadUrlRemnants()
{
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == ')') {
            m_input.advance();
            break;
        }

        if(isEscapeSequence()) {
            consumeEscape();
        } else {
            m_input.advance();
        }
    }

    return CSSToken(CSSToken::Type::BadUrl);
}

CSSToken CSSTokenizer::consumeWhitespaceToken()
{
    auto cc = m_input.peek();
    assert(isspace(cc));
    do {
        cc = m_input.advance();
    } while(isspace(cc));

    return CSSToken(CSSToken::Type::Whitespace);
}

CSSToken CSSTokenizer::consumeCommentToken()
{
    while(true) {
        auto cc = m_input.peek();
        if(cc == 0)
            break;
        if(cc == '*' && m_input.peek(1) == '/') {
            m_input.advance(2);
            break;
        }

        m_input.advance();
    }

    return CSSToken(CSSToken::Type::Comment);
}

CSSToken CSSTokenizer::consumeSolidusToken()
{
    auto cc = m_input.advance();
    if(cc == '*') {
        m_input.advance();
        return consumeCommentToken();
    }

    return CSSToken(CSSToken::Type::Delim, '/');
}

CSSToken CSSTokenizer::consumeHashToken()
{
    auto cc = m_input.advance();
    if(isNameChar(cc) || isEscapeSequence()) {
        if(isIdentSequence())
            return CSSToken(CSSToken::Type::Hash, CSSToken::HashType::Identifier, consumeName());
        return CSSToken(CSSToken::Type::Hash, CSSToken::HashType::Unrestricted, consumeName());
    }

    return CSSToken(CSSToken::Type::Delim, '#');
}

CSSToken CSSTokenizer::consumePlusSignToken()
{
    if(isNumberSequence())
        return consumeNumericToken();

    m_input.advance();
    return CSSToken(CSSToken::Type::Delim, '+');
}

CSSToken CSSTokenizer::consumeHyphenMinusToken()
{
    if(isNumberSequence())
        return consumeNumericToken();

    if(m_input.peek(1) == '-' && m_input.peek(2) == '>') {
        m_input.advance(3);
        return CSSToken(CSSToken::Type::CDC);
    }

    if(isIdentSequence())
        return consumeIdentLikeToken();

    m_input.advance();
    return CSSToken(CSSToken::Type::Delim, '-');
}

CSSToken CSSTokenizer::consumeFullStopToken()
{
    if(isNumberSequence())
        return consumeNumericToken();

    m_input.advance();
    return CSSToken(CSSToken::Type::Delim, '.');
}

CSSToken CSSTokenizer::consumeLessThanSignToken()
{
    auto cc = m_input.advance();
    if(cc == '!' && m_input.peek(1) == '-' && m_input.peek(2) == '-') {
        m_input.advance(3);
        return CSSToken(CSSToken::Type::CDO);
    }

    return CSSToken(CSSToken::Type::Delim, '<');
}

CSSToken CSSTokenizer::consumeCommercialAtToken()
{
    m_input.advance();
    if(isIdentSequence())
        return CSSToken(CSSToken::Type::AtKeyword, consumeName());

    return CSSToken(CSSToken::Type::Delim, '@');
}

CSSToken CSSTokenizer::consumeReverseSolidusToken()
{
    if(isEscapeSequence())
        return consumeIdentLikeToken();

    m_input.advance();
    return CSSToken(CSSToken::Type::Delim, '\\');
}

CSSToken CSSTokenizer::nextToken()
{
    auto cc = m_input.peek();
    if(cc == 0)
        return CSSToken(CSSToken::Type::EndOfFile);

    if(isspace(cc))
        return consumeWhitespaceToken();

    if(isdigit(cc))
        return consumeNumericToken();

    if(isNameStart(cc))
        return consumeIdentLikeToken();

    switch(cc) {
    case '/':
        return consumeSolidusToken();
    case '#':
        return consumeHashToken();
    case '+':
        return consumePlusSignToken();
    case '-':
        return consumeHyphenMinusToken();
    case '.':
        return consumeFullStopToken();
    case '<':
        return consumeLessThanSignToken();
    case '@':
        return consumeCommercialAtToken();
    case '\\':
        return consumeReverseSolidusToken();
    case '\"':
        return consumeStringToken();
    case '\'':
        return consumeStringToken();
    }

    m_input.advance();
    switch(cc) {
    case '(':
        return CSSToken(CSSToken::Type::LeftParenthesis);
    case ')':
        return CSSToken(CSSToken::Type::RightParenthesis);
    case '[':
        return CSSToken(CSSToken::Type::LeftSquareBracket);
    case ']':
        return CSSToken(CSSToken::Type::RightSquareBracket);
    case '{':
        return CSSToken(CSSToken::Type::LeftCurlyBracket);
    case '}':
        return CSSToken(CSSToken::Type::RightCurlyBracket);
    case ',':
        return CSSToken(CSSToken::Type::Comma);
    case ':':
        return CSSToken(CSSToken::Type::Colon);
    case ';':
        return CSSToken(CSSToken::Type::Semicolon);
    }

    return CSSToken(CSSToken::Type::Delim, cc);
}

void CSSParser::parseSheet(CSSRuleList& rules, const std::string_view& content)
{
    CSSTokenizer tokenizer(content);
    auto input = tokenizer.tokenize();
    while(!input.empty()) {
        input.consumeWhitespace();
        if(input->type() == CSSToken::Type::CDC
            || input->type() == CSSToken::Type::CDO) {
            input.consume();
            continue;
        }

        consumeRule(input, rules);
    }
}

void CSSParser::parseStyle(CSSPropertyList& properties, const std::string_view& content)
{
    CSSTokenizer tokenizer(content);
    auto input = tokenizer.tokenize();
    consumeDeclaractionList(input, properties);
}

bool CSSParser::consumeRule(CSSTokenStream& input, CSSRuleList& rules)
{
    if(input->type() == CSSToken::Type::AtKeyword)
        return consumeAtRule(input, rules);
    return consumeStyleRule(input, rules);
}

bool CSSParser::consumeStyleRule(CSSTokenStream& input, CSSRuleList& rules)
{
    auto preludeBegin = input.begin();
    while(!input.empty() && input->type() != CSSToken::Type::LeftCurlyBracket) {
        input.consumeComponent();
    }

    CSSTokenStream prelude(preludeBegin, input.begin());
    if(input.empty())
        return false;

    auto block = input.consumeBlock();
    CSSSelectorList selectors;
    if(!consumeSelectorList(prelude, selectors))
        return false;

    CSSPropertyList properties;
    consumeDeclaractionList(block, properties);
    rules.push_back(CSSRule::create(std::move(selectors), std::move(properties)));
    return true;
}

bool CSSParser::consumeAtRule(CSSTokenStream& input, CSSRuleList& rules)
{
    auto name = input->data();
    input.consume();
    auto preludeBegin = input.begin();
    while(input->type() != CSSToken::Type::EndOfFile
        && input->type() != CSSToken::Type::LeftCurlyBracket
        && input->type() != CSSToken::Type::Semicolon) {
        input.consumeComponent();
    }

    CSSTokenStream prelude(preludeBegin, input.begin());
    if(input->type() == CSSToken::Type::EndOfFile
        || input->type() == CSSToken::Type::Semicolon) {
        if(input->type() == CSSToken::Type::Semicolon)
            input.consume();
        if(equals(name, "import", false))
            return consumeImportRule(prelude, rules);
        return false;
    }

    return false;
}

bool CSSParser::consumeImportRule(CSSTokenStream& input, CSSRuleList& rules)
{
    return false;
}

bool CSSParser::consumeSelectorList(CSSTokenStream& input, CSSSelectorList& selectors)
{
    CSSSelector selector;
    input.consumeWhitespace();
    if(!consumeSelector(input, selector))
        return false;

    selectors.push_back(std::move(selector));
    while(input->type() == CSSToken::Type::Comma) {
        input.consumeIncludingWhitespace();
        if(!consumeSelector(input, selector))
            return false;
        selectors.push_back(std::move(selector));
    }

    return input.empty();
}

bool CSSParser::consumeSelector(CSSTokenStream& input, CSSSelector& selector)
{
    auto combinator = CSSSimpleSelector::Combinator::None;
    do {
        CSSSimpleSelector sel;
        sel.combinator = combinator;
        if(!consumeSimpleSelector(input, sel))
            return combinator == CSSSimpleSelector::Combinator::Descendant;
        selector.push_back(std::move(sel));
    } while(consumeCombinator(input, combinator));
    return true;
}

bool CSSParser::consumeSimpleSelector(CSSTokenStream& input, CSSSimpleSelector& selector)
{
    auto consume = [](auto& input, auto& selector) {
        if(input->type() == CSSToken::Type::Hash)
            return consumeIdSelector(input, selector);
        if(input->type() == CSSToken::Type::Delim && input->delim() == '.')
            return consumeClassSelector(input, selector);
        if(input->type() == CSSToken::Type::LeftSquareBracket)
            return consumeAttributeSelector(input, selector);
        if(input->type() == CSSToken::Type::Colon)
            return consumePseudoSelector(input, selector);
        return false;
    };

    if(!consumeTagSelector(input, selector)
        && !consume(input, selector)) {
        return false;
    }

    while(consume(input, selector));
    return true;
}

bool CSSParser::consumeTagSelector(CSSTokenStream& input, CSSSimpleSelector& selector)
{
    if(input->type() == CSSToken::Type::Ident) {
        selector.id = elementid(input->data());
        input.consume();
        return true;
    }

    if(input->type() == CSSToken::Type::Delim && input->delim() == '*') {
        selector.id = ElementID::Star;
        input.consume();
        return true;
    }

    return false;
}

bool CSSParser::consumeIdSelector(CSSTokenStream& input, CSSSimpleSelector& selector)
{
    if(input->hashType() == CSSToken::HashType::Identifier) {
        CSSSimpleSelector::Attribute attribute;
        attribute.type = CSSSimpleSelector::Attribute::Type::Equals;
        attribute.id = PropertyID::Id;
        attribute.value = input->data();
        selector.attributes.push_back(attribute);
        input.consume();
        return true;
    }

    return false;
}

bool CSSParser::consumeClassSelector(CSSTokenStream& input, CSSSimpleSelector& selector)
{
    input.consume();
    if(input->type() == CSSToken::Type::Ident) {
        CSSSimpleSelector::Attribute attribute;
        attribute.type = CSSSimpleSelector::Attribute::Type::Includes;
        attribute.id = PropertyID::Class;
        attribute.value = input->data();
        selector.attributes.push_back(attribute);
        input.consume();
        return true;
    }

    return false;
}

bool CSSParser::consumeAttributeSelector(CSSTokenStream& input, CSSSimpleSelector& selector)
{
    auto block = input.consumeBlock();
    if(block->type() != CSSToken::Type::Ident)
        return false;

    CSSSimpleSelector::Attribute attribute;
    attribute.id = propertyid(block->data());
    block.consumeIncludingWhitespace();
    if(block.empty()) {
        attribute.type = CSSSimpleSelector::Attribute::Type::None;
        selector.attributes.push_back(attribute);
        return true;
    }

    if(block->type() != CSSToken::Type::Delim)
        return false;

    switch(block->delim()) {
    case '=':
        attribute.type = CSSSimpleSelector::Attribute::Type::Equals;
        break;
    case '*':
        attribute.type = CSSSimpleSelector::Attribute::Type::Contains;
        break;
    case '~':
        attribute.type = CSSSimpleSelector::Attribute::Type::Includes;
        break;
    case '^':
        attribute.type = CSSSimpleSelector::Attribute::Type::StartsWith;
        break;
    case '$':
        attribute.type = CSSSimpleSelector::Attribute::Type::EndsWith;
        break;
    case '|':
        attribute.type = CSSSimpleSelector::Attribute::Type::DashEquals;
        break;
    default:
        return false;
    }

    if(attribute.type != CSSSimpleSelector::Attribute::Type::Equals) {
        block.consume();
        if(block->type() != CSSToken::Type::Delim && block->delim() != '=')
            return false;
    }

    block.consumeIncludingWhitespace();
    if(block->type() != CSSToken::Type::Ident && block->type() != CSSToken::Type::String)
        return false;

    attribute.value = block->data();
    block.consumeIncludingWhitespace();
    if(block.empty()) {
        selector.attributes.push_back(attribute);
        return true;
    }

    return false;
}

bool CSSParser::consumePseudoSelector(CSSTokenStream& input, CSSSimpleSelector& selector)
{
    input.consume();
    if(input->type() == CSSToken::Type::Ident) {
        auto name = input->data();
        input.consume();
        static const struct {
            std::string_view name;
            CSSSimpleSelector::Pseudo::Type value;
        } table[] = {
            {"root", CSSSimpleSelector::Pseudo::Type::Root},
            {"empty", CSSSimpleSelector::Pseudo::Type::Empty},
            {"first-child", CSSSimpleSelector::Pseudo::Type::FirstChild},
            {"last-child", CSSSimpleSelector::Pseudo::Type::LastChild},
            {"only-child", CSSSimpleSelector::Pseudo::Type::OnlyChild},
            {"first-of-type", CSSSimpleSelector::Pseudo::Type::FirstOfType},
            {"last-of-type", CSSSimpleSelector::Pseudo::Type::LastOfType},
            {"only-of-type", CSSSimpleSelector::Pseudo::Type::OnlyOfType}
        };

        auto it = std::find_if(table, std::end(table), [name](auto& item) { return equals(name, item.name, false); });
        if(it == std::end(table))
            return false;
        CSSSimpleSelector::Pseudo pseudo;
        pseudo.type = it->value;
        selector.pseudos.push_back(pseudo);
        return true;
    }

    if(input->type() == CSSToken::Type::Function) {
        auto name = input->data();
        auto block = input.consumeBlock();
        block.consumeIncludingWhitespace();
        static const struct {
            std::string_view name;
            CSSSimpleSelector::Pseudo::Type value;
        } table[] = {
            {"is", CSSSimpleSelector::Pseudo::Type::Is},
            {"not", CSSSimpleSelector::Pseudo::Type::Not},
            {"nth-child", CSSSimpleSelector::Pseudo::Type::NthChild},
            {"nth-last-child", CSSSimpleSelector::Pseudo::Type::NthLastChild},
            {"nth-of-type", CSSSimpleSelector::Pseudo::Type::NthOfType},
            {"nth-last-of-type", CSSSimpleSelector::Pseudo::Type::NthLastOfType}
        };

        auto it = std::find_if(table, std::end(table), [name](auto& item) { return equals(name, item.name, false); });
        if(it == std::end(table))
            return false;
        CSSSimpleSelector::Pseudo pseudo;
        pseudo.type = it->value;
        switch(pseudo.type) {
        case CSSSimpleSelector::Pseudo::Type::Is:
        case CSSSimpleSelector::Pseudo::Type::Not: {
            if(!consumeSelectorList(block, pseudo.selectors))
                return false;
            break;
        }

        case CSSSimpleSelector::Pseudo::Type::NthChild:
        case CSSSimpleSelector::Pseudo::Type::NthLastChild:
        case CSSSimpleSelector::Pseudo::Type::NthOfType:
        case CSSSimpleSelector::Pseudo::Type::NthLastOfType: {
            if(!consumeMatchPattern(block, pseudo.pattern))
                return false;
            break;
        }

        default:
            assert(false);
        }

        selector.pseudos.push_back(pseudo);
        block.consumeWhitespace();
        return block.empty();
    }

    return false;
}

bool CSSParser::consumeCombinator(CSSTokenStream& input, CSSSimpleSelector::Combinator& combinator)
{
    combinator = CSSSimpleSelector::Combinator::None;
    while(input->type() == CSSToken::Type::Whitespace) {
        combinator = CSSSimpleSelector::Combinator::Descendant;
        input.consume();
    }

    if(input->type() == CSSToken::Type::Delim) {
        if(input->delim() == '+') {
            combinator = CSSSimpleSelector::Combinator::DirectAdjacent;
            input.consumeIncludingWhitespace();
            return true;
        }

        if(input->delim() == '~') {
            combinator = CSSSimpleSelector::Combinator::InDirectAdjacent;
            input.consumeIncludingWhitespace();
            return true;
        }

        if(input->delim() == '>') {
            combinator = CSSSimpleSelector::Combinator::Child;
            input.consumeIncludingWhitespace();
            return true;
        }
    }

    return combinator == CSSSimpleSelector::Combinator::Descendant;
}

bool CSSParser::consumeMatchPattern(CSSTokenStream& input, CSSSimpleSelector::Pseudo::MatchPattern& pattern)
{
    if(input->type() == CSSToken::Type::Number) {
        if(input->numberType() != CSSToken::NumberType::Integer)
            return false;
        pattern = std::make_pair(0, input->integer());
        input.consume();
        return true;
    }

    if(input->type() == CSSToken::Type::Ident) {
        if(equals(input->data(), "odd", false)) {
            pattern = std::make_pair(2, 1);
            input.consume();
            return true;
        }

        if(equals(input->data(), "even", false)) {
            pattern = std::make_pair(2, 0);
            input.consume();
            return true;
        }
    }

    std::stringstream ss;
    if(input->type() == CSSToken::Type::Delim) {
        if(input->delim() != '+')
            return false;
        input.consume();
        if(input->type() != CSSToken::Type::Ident)
            return false;
        pattern.first = 1;
        ss << input->data();
        input.consume();
    } else if(input->type() == CSSToken::Type::Ident) {
        auto ident = input->data();
        input.consume();
        if(ident.front() == '-') {
            pattern.first = -1;
            ss << ident.substr(1);
        } else {
            pattern.first = 1;
            ss << ident;
        }
    } else if(input->type() == CSSToken::Type::Dimension) {
        if(input->numberType() != CSSToken::NumberType::Integer)
            return false;
        pattern.first = input->integer();
        ss << input->data();
        input.consume();
    }

    constexpr auto eof = std::stringstream::traits_type::eof();
    if(ss.peek() == eof || !equals(ss.get(), 'n', false))
        return false;

    auto sign = CSSToken::NumberSign::None;
    if(ss.peek() != eof) {
        if(ss.get() != '-')
            return false;
        sign = CSSToken::NumberSign::Minus;
        if(ss.peek() != eof) {
            ss >> pattern.second;
            if(ss.fail())
                return false;
            pattern.second = -pattern.second;
            return true;
        }
    }

    input.consumeWhitespace();
    if(sign == CSSToken::NumberSign::None && input->type() == CSSToken::Type::Delim) {
        auto delim = input->delim();
        if(delim == '+')
            sign = CSSToken::NumberSign::Plus;
        else if(delim == '-')
            sign = CSSToken::NumberSign::Minus;
        else
            return false;
        input.consumeIncludingWhitespace();
    }

    if(sign == CSSToken::NumberSign::None && input->type() != CSSToken::Type::Number) {
        pattern.second = 0;
        return true;
    }

    if(input->type() != CSSToken::Type::Number || input->numberType() != CSSToken::NumberType::Integer)
        return false;

    if(sign == CSSToken::NumberSign::None && input->numberSign() == CSSToken::NumberSign::None)
        return false;

    if(sign != CSSToken::NumberSign::None && input->numberSign() != CSSToken::NumberSign::None)
        return false;

    pattern.second = input->integer();
    if(sign == CSSToken::NumberSign::Minus)
        pattern.second = -pattern.second;
    input.consume();
    return true;
}

void CSSParser::consumeDeclaractionList(CSSTokenStream& input, CSSPropertyList& properties)
{
    input.consumeWhitespace();
    consumeDeclaraction(input, properties);
    while(input->type() == CSSToken::Type::Semicolon) {
        input.consumeIncludingWhitespace();
        consumeDeclaraction(input, properties);
    }
}

bool CSSParser::consumeDeclaraction(CSSTokenStream& input, CSSPropertyList& properties)
{
    auto begin = input.begin();
    while(!input.empty() && input->type() != CSSToken::Type::Semicolon) {
        input.consumeComponent();
    }

    CSSTokenStream newInput(begin, input.begin());
    if(newInput->type() != CSSToken::Type::Ident)
        return false;

    auto id = csspropertyid(newInput->data());
    if(id == CSSPropertyID::Unknown)
        return false;

    newInput.consumeIncludingWhitespace();
    if(newInput->type() != CSSToken::Type::Colon)
        return false;

    newInput.consumeIncludingWhitespace();
    auto valueBegin = newInput.begin();
    auto valueEnd = newInput.end();

    auto it = valueEnd - 1;
    while(it->type() == CSSToken::Type::Whitespace) {
        it -= 1;
    }

    bool important = false;
    if(it->type() == CSSToken::Type::Ident && equals(it->data(), "important", false)) {
        do {
            it -= 1;
        } while(it->type() == CSSToken::Type::Whitespace);
        if(it->type() == CSSToken::Type::Delim && it->delim() == '!') {
            important = true;
            valueEnd = it;
        }
    }

    CSSTokenStream value(valueBegin, valueEnd);
    return consumeDeclaractionValue(value, properties, id, important);
}

bool CSSParser::consumeDeclaractionValue(CSSTokenStream& input, CSSPropertyList& properties, CSSPropertyID id, bool important)
{
    if(input->type() == CSSToken::Type::Ident) {
        if(equals(input->data(), "inherit", false)) {
            input.consumeIncludingWhitespace();
            if(!input.empty())
                return false;
            properties.emplace_back(id, important, CSSInheritValue::create());
            return true;
        }

        if(equals(input->data(), "initial", false)) {
            input.consumeIncludingWhitespace();
            if(!input.empty())
                return false;
            properties.emplace_back(id, important, CSSInitialValue::create());
            return true;
        }
    }

    auto value = consumeValue(input, id);
    input.consumeWhitespace();
    if(value == nullptr || !input.empty())
        return false;
    properties.emplace_back(id, important, value);
    return true;
}

struct idententry_t {
    std::string_view name;
    CSSValueID value;
};

template<unsigned int N>
inline CSSValueID matchIdent(const CSSTokenStream& input, const idententry_t(&table)[N])
{
    if(input->type() != CSSToken::Type::Ident)
        return CSSValueID::Unknown;

    auto name = input->data();
    for(auto& entry : table) {
        if(equals(name, entry.name, false))
            return entry.value;
    }

    return CSSValueID::Unknown;
}

template<unsigned int N>
inline RefPtr<CSSValue> consumeIdent(CSSTokenStream& input, const idententry_t(&table)[N])
{
    auto id = matchIdent(input, table);
    if(id == CSSValueID::Unknown)
        return nullptr;
    input.consumeIncludingWhitespace();
    return CSSIdentValue::create(id);
}

RefPtr<CSSValue> CSSParser::consumeNone(CSSTokenStream& input)
{
    if(input->type() == CSSToken::Type::Ident && equals(input->data(), "none", false)) {
        input.consumeIncludingWhitespace();
        return CSSIdentValue::create(CSSValueID::None);
    }

    return nullptr;
}

RefPtr<CSSValue> CSSParser::consumeNormal(CSSTokenStream& input)
{
    if(input->type() == CSSToken::Type::Ident && equals(input->data(), "normal", false)) {
        input.consumeIncludingWhitespace();
        return CSSIdentValue::create(CSSValueID::Normal);
    }

    return nullptr;
}

RefPtr<CSSValue> CSSParser::consumePercent(CSSTokenStream& input, bool negative)
{
    if(input->type() != CSSToken::Type::Percentage || (input->number() < 0 && !negative))
        return nullptr;

    auto value = input->number();
    input.consumeIncludingWhitespace();
    return CSSPercentValue::create(value);
}

RefPtr<CSSValue> CSSParser::consumeNumber(CSSTokenStream& input, bool negative)
{
    if(input->type() != CSSToken::Type::Number || (input->number() < 0 && !negative))
        return nullptr;

    auto value = input->number();
    input.consumeIncludingWhitespace();
    return CSSNumberValue::create(value);
}

RefPtr<CSSValue> CSSParser::consumeLength(CSSTokenStream& input, bool negative, bool unitless)
{
    if(input->type() != CSSToken::Type::Dimension && input->type() != CSSToken::Type::Number)
        return nullptr;

    auto value = input->number();
    if(value < 0.0 && !negative || (input->type() == CSSToken::Type::Number && !unitless))
        return nullptr;
    if(input->type() == CSSToken::Type::Number) {
        input.consumeIncludingWhitespace();
        return CSSLengthValue::create(value, CSSLengthValue::Unit::None);
    }

    static const struct {
        std::string_view name;
        CSSLengthValue::Unit value;
    } table[] = {
        {"em", CSSLengthValue::Unit::Ems},
        {"ex", CSSLengthValue::Unit::Exs},
        {"px", CSSLengthValue::Unit::Pixels},
        {"cm", CSSLengthValue::Unit::Centimeters},
        {"mm", CSSLengthValue::Unit::Millimeters},
        {"in", CSSLengthValue::Unit::Inches},
        {"pt", CSSLengthValue::Unit::Points},
        {"pc", CSSLengthValue::Unit::Picas},
        {"vw", CSSLengthValue::Unit::ViewportWidth},
        {"vh", CSSLengthValue::Unit::ViewportHeight},
        {"vmin", CSSLengthValue::Unit::ViewportMin},
        {"vmax", CSSLengthValue::Unit::ViewportMax},
        {"rem", CSSLengthValue::Unit::Rems},
        {"ch", CSSLengthValue::Unit::Chs}
    };

    auto name = input->data();
    auto it = std::find_if(table, std::end(table), [name](auto& item) { return equals(name, item.name, false); });
    if(it == std::end(table))
        return nullptr;
    input.consumeIncludingWhitespace();
    return CSSLengthValue::create(value, it->value);
}

RefPtr<CSSValue> CSSParser::consumeLengthOrNormal(CSSTokenStream& input, bool negative, bool unitless)
{
    if(auto value = consumeNormal(input))
        return value;
    return consumeLength(input, negative, unitless);
}

RefPtr<CSSValue> CSSParser::consumeLengthOrPercent(CSSTokenStream& input, bool negative, bool unitless)
{
    auto value = consumeLength(input, negative, unitless);
    if(value == nullptr)
        return consumePercent(input, negative);
    return value;
}

RefPtr<CSSValue> CSSParser::consumeNumberOrPercent(CSSTokenStream& input, bool negative)
{
    auto value = consumeNumber(input, negative);
    if(value == nullptr)
        return consumePercent(input, negative);
    return value;
}

RefPtr<CSSValue> CSSParser::consumeUrl(CSSTokenStream& input)
{
    std::string value;
    switch(input->type()) {
    case CSSToken::Type::Url:
    case CSSToken::Type::String:
        value = input->data();
        input.consumeIncludingWhitespace();
        break;
    case CSSToken::Type::Function: {
        if(!equals(input->data(), "url", false))
            return nullptr;
        CSSTokenStreamGuard guard(input);
        auto block = input.consumeBlock();
        block.consumeWhitespace();
        value = block->data();
        block.consumeIncludingWhitespace();
        if(!block.empty())
            return nullptr;
        input.consumeWhitespace();
        guard.release();
        break;
    }

    default:
        return nullptr;
    }

    return CSSUrlValue::create(std::move(value));
}

RefPtr<CSSValue> CSSParser::consumeUrlOrNone(CSSTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;
    return consumeUrl(input);
}

RefPtr<CSSValue> CSSParser::consumeColor(CSSTokenStream& input)
{
    if(input->type() == CSSToken::Type::Hash) {
        int count = 0;
        uint32_t value = 0;
        for(auto cc : input->data()) {
            if(count >= 6 || !isxdigit(cc))
                return nullptr;
            value = value * 16 + xdigit(cc);
            count += 1;
        }

        if(count != 6 && count != 3)
            return nullptr;
        if(count == 3) {
            value = ((value&0xf00) << 8) | ((value&0x0f0) << 4) | (value&0x00f);
            value |= value << 4;
        }

        input.consumeIncludingWhitespace();
        return CSSColorValue::create(value | 0xFF000000);
    }

    if(input->type() == CSSToken::Type::Function) {
        auto name = input->data();
        if(equals(name, "rgb", false) || equals(name, "rgba", false))
            return consumeRgb(input);
        return nullptr;
    }

    if(input->type() == CSSToken::Type::Ident) {
        auto name = input->data();
        if(equals(name, "currentcolor", false)) {
            input.consumeIncludingWhitespace();
            return CSSIdentValue::create(CSSValueID::CurrentColor);
        }

        if(equals(name, "transparent", false)) {
            input.consumeIncludingWhitespace();
            return CSSColorValue::create(0x00000000);
        }

        static const struct {
            std::string_view name;
            uint32_t value;
        } table[] = {
            {"aliceblue", 0xF0F8FF},
            {"antiquewhite", 0xFAEBD7},
            {"aqua", 0x00FFFF},
            {"aquamarine", 0x7FFFD4},
            {"azure", 0xF0FFFF},
            {"beige", 0xF5F5DC},
            {"bisque", 0xFFE4C4},
            {"black", 0x000000},
            {"blanchedalmond", 0xFFEBCD},
            {"blue", 0x0000FF},
            {"blueviolet", 0x8A2BE2},
            {"brown", 0xA52A2A},
            {"burlywood", 0xDEB887},
            {"cadetblue", 0x5F9EA0},
            {"chartreuse", 0x7FFF00},
            {"chocolate", 0xD2691E},
            {"coral", 0xFF7F50},
            {"cornflowerblue", 0x6495ED},
            {"cornsilk", 0xFFF8DC},
            {"crimson", 0xDC143C},
            {"cyan", 0x00FFFF},
            {"darkblue", 0x00008B},
            {"darkcyan", 0x008B8B},
            {"darkgoldenrod", 0xB8860B},
            {"darkgray", 0xA9A9A9},
            {"darkgreen", 0x006400},
            {"darkgrey", 0xA9A9A9},
            {"darkkhaki", 0xBDB76B},
            {"darkmagenta", 0x8B008B},
            {"darkolivegreen", 0x556B2F},
            {"darkorange", 0xFF8C00},
            {"darkorchid", 0x9932CC},
            {"darkred", 0x8B0000},
            {"darksalmon", 0xE9967A},
            {"darkseagreen", 0x8FBC8F},
            {"darkslateblue", 0x483D8B},
            {"darkslategray", 0x2F4F4F},
            {"darkslategrey", 0x2F4F4F},
            {"darkturquoise", 0x00CED1},
            {"darkviolet", 0x9400D3},
            {"deeppink", 0xFF1493},
            {"deepskyblue", 0x00BFFF},
            {"dimgray", 0x696969},
            {"dimgrey", 0x696969},
            {"dodgerblue", 0x1E90FF},
            {"firebrick", 0xB22222},
            {"floralwhite", 0xFFFAF0},
            {"forestgreen", 0x228B22},
            {"fuchsia", 0xFF00FF},
            {"gainsboro", 0xDCDCDC},
            {"ghostwhite", 0xF8F8FF},
            {"gold", 0xFFD700},
            {"goldenrod", 0xDAA520},
            {"gray", 0x808080},
            {"green", 0x008000},
            {"greenyellow", 0xADFF2F},
            {"grey", 0x808080},
            {"honeydew", 0xF0FFF0},
            {"hotpink", 0xFF69B4},
            {"indianred", 0xCD5C5C},
            {"indigo", 0x4B0082},
            {"ivory", 0xFFFFF0},
            {"khaki", 0xF0E68C},
            {"lavender", 0xE6E6FA},
            {"lavenderblush", 0xFFF0F5},
            {"lawngreen", 0x7CFC00},
            {"lemonchiffon", 0xFFFACD},
            {"lightblue", 0xADD8E6},
            {"lightcoral", 0xF08080},
            {"lightcyan", 0xE0FFFF},
            {"lightgoldenrodyellow", 0xFAFAD2},
            {"lightgray", 0xD3D3D3},
            {"lightgreen", 0x90EE90},
            {"lightgrey", 0xD3D3D3},
            {"lightpink", 0xFFB6C1},
            {"lightsalmon", 0xFFA07A},
            {"lightseagreen", 0x20B2AA},
            {"lightskyblue", 0x87CEFA},
            {"lightslategray", 0x778899},
            {"lightslategrey", 0x778899},
            {"lightsteelblue", 0xB0C4DE},
            {"lightyellow", 0xFFFFE0},
            {"lime", 0x00FF00},
            {"limegreen", 0x32CD32},
            {"linen", 0xFAF0E6},
            {"magenta", 0xFF00FF},
            {"maroon", 0x800000},
            {"mediumaquamarine", 0x66CDAA},
            {"mediumblue", 0x0000CD},
            {"mediumorchid", 0xBA55D3},
            {"mediumpurple", 0x9370DB},
            {"mediumseagreen", 0x3CB371},
            {"mediumslateblue", 0x7B68EE},
            {"mediumspringgreen", 0x00FA9A},
            {"mediumturquoise", 0x48D1CC},
            {"mediumvioletred", 0xC71585},
            {"midnightblue", 0x191970},
            {"mintcream", 0xF5FFFA},
            {"mistyrose", 0xFFE4E1},
            {"moccasin", 0xFFE4B5},
            {"navajowhite", 0xFFDEAD},
            {"navy", 0x000080},
            {"oldlace", 0xFDF5E6},
            {"olive", 0x808000},
            {"olivedrab", 0x6B8E23},
            {"orange", 0xFFA500},
            {"orangered", 0xFF4500},
            {"orchid", 0xDA70D6},
            {"palegoldenrod", 0xEEE8AA},
            {"palegreen", 0x98FB98},
            {"paleturquoise", 0xAFEEEE},
            {"palevioletred", 0xDB7093},
            {"papayawhip", 0xFFEFD5},
            {"peachpuff", 0xFFDAB9},
            {"peru", 0xCD853F},
            {"pink", 0xFFC0CB},
            {"plum", 0xDDA0DD},
            {"powderblue", 0xB0E0E6},
            {"purple", 0x800080},
            {"rebeccapurple", 0x663399},
            {"red", 0xFF0000},
            {"rosybrown", 0xBC8F8F},
            {"royalblue", 0x4169E1},
            {"saddlebrown", 0x8B4513},
            {"salmon", 0xFA8072},
            {"sandybrown", 0xF4A460},
            {"seagreen", 0x2E8B57},
            {"seashell", 0xFFF5EE},
            {"sienna", 0xA0522D},
            {"silver", 0xC0C0C0},
            {"skyblue", 0x87CEEB},
            {"slateblue", 0x6A5ACD},
            {"slategray", 0x708090},
            {"slategrey", 0x708090},
            {"snow", 0xFFFAFA},
            {"springgreen", 0x00FF7F},
            {"steelblue", 0x4682B4},
            {"tan", 0xD2B48C},
            {"teal", 0x008080},
            {"thistle", 0xD8BFD8},
            {"tomato", 0xFF6347},
            {"turquoise", 0x40E0D0},
            {"violet", 0xEE82EE},
            {"wheat", 0xF5DEB3},
            {"white", 0xFFFFFF},
            {"whitesmoke", 0xF5F5F5},
            {"yellow", 0xFFFF00},
            {"yellowgreen", 0x9ACD32}
        };

        auto it = std::lower_bound(table, std::end(table), name, [](auto& item, auto& name) { return item.name < name; });
        if(it == std::end(table) || it->name != name)
            return nullptr;
        input.consumeIncludingWhitespace();
        return CSSColorValue::create(it->value | 0xFF000000);
    }

    return nullptr;
}

inline bool consumeRgbComponent(CSSTokenStream& input, int& component)
{
    if(input->type() != CSSToken::Type::Number
        && input->type() != CSSToken::Type::Percentage) {
        return false;
    }

    auto value = input->number();
    if(input->type() == CSSToken::Type::Percentage)
        value *= 2.55;
    value = std::clamp(value, 0.0, 255.0);
    component = static_cast<int>(std::round(value));
    input.consumeIncludingWhitespace();
    return true;
}

RefPtr<CSSValue> CSSParser::consumeRgb(CSSTokenStream& input)
{
    assert(input->type() == CSSToken::Type::Function);
    CSSTokenStreamGuard guard(input);
    auto block = input.consumeBlock();
    block.consumeWhitespace();

    int red = 0;
    if(!consumeRgbComponent(block, red))
        return nullptr;

    if(block->type() != CSSToken::Type::Comma)
        return nullptr;

    int blue = 0;
    block.consumeIncludingWhitespace();
    if(!consumeRgbComponent(block, blue))
        return nullptr;

    if(block->type() != CSSToken::Type::Comma)
        return nullptr;

    int green = 0;
    block.consumeIncludingWhitespace();
    if(!consumeRgbComponent(block, green))
        return nullptr;

    int alpha = 255;
    if(block->type() == CSSToken::Type::Comma) {
        block.consumeIncludingWhitespace();
        if(block->type() != CSSToken::Type::Number
            && block->type() != CSSToken::Type::Percentage) {
            return nullptr;
        }

        auto value = block->number();
        if(block->type() == CSSToken::Type::Percentage)
            value /= 100.0;
        value = std::clamp(value, 0.0, 1.0);
        alpha = static_cast<int>(std::round(value * 255.0));
        block.consumeIncludingWhitespace();
    }

    if(!block.empty())
        return nullptr;
    input.consumeWhitespace();
    guard.release();
    return CSSColorValue::create(red, green, blue, alpha);
}

RefPtr<CSSValue> CSSParser::consumeFillOrStroke(CSSTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;

    auto first = consumeUrl(input);
    if(first == nullptr)
        return consumeColor(input);

    auto second = consumeNone(input);
    if(second == nullptr)
        second = consumeColor(input);
    if(second == nullptr)
        return first;
    return CSSPairValue::create(first, second);
}

RefPtr<CSSValue> CSSParser::consumeDashList(CSSTokenStream& input)
{
    if(auto value = consumeNone(input))
        return value;

    CSSValueList values;
    auto value = consumeLengthOrPercent(input, false, true);
    if(value == nullptr)
        return nullptr;

    values.push_back(value);
    while(input->type() == CSSToken::Type::Comma) {
        input.consumeIncludingWhitespace();
        auto value = consumeLengthOrPercent(input, false, true);
        if(value == nullptr)
            return nullptr;
        values.push_back(value);
    }

    if(!input.empty())
        return nullptr;
    return CSSListValue::create(std::move(values));
}

RefPtr<CSSValue> CSSParser::consumeFontWeight(CSSTokenStream& input)
{
    static const idententry_t table[] = {
        {"normal", CSSValueID::Normal},
        {"bold", CSSValueID::Bold},
        {"bolder", CSSValueID::Bolder},
        {"lighter", CSSValueID::Lighter}
    };

    if(auto value = consumeIdent(input, table))
        return value;

    if(input->type() != CSSToken::Type::Number || input->numberType() != CSSToken::NumberType::Integer)
        return nullptr;
    auto value = input->integer();
    if(value < 1 || value > 1000)
        return nullptr;
    input.consumeIncludingWhitespace();
    return CSSIntegerValue::create(value);
}

RefPtr<CSSValue> CSSParser::consumeFontSize(CSSTokenStream& input, bool unitless)
{
    static const idententry_t table[] = {
        {"xx-small", CSSValueID::XxSmall},
        {"x-small", CSSValueID::XSmall},
        {"small", CSSValueID::Small},
        {"medium", CSSValueID::Medium},
        {"large", CSSValueID::Large},
        {"x-large", CSSValueID::XLarge},
        {"xx-large", CSSValueID::XxLarge},
        {"xxx-large", CSSValueID::XxxLarge},
        {"smaller", CSSValueID::Smaller},
        {"larger", CSSValueID::Larger}
    };

    if(auto value = consumeIdent(input, table))
        return value;
    return consumeLengthOrPercent(input, false, unitless);
}

RefPtr<CSSValue> CSSParser::consumeFontFamilyValue(CSSTokenStream& input)
{
    if(input->type() == CSSToken::Type::String) {
        std::string value(input->data());
        input.consumeIncludingWhitespace();
        return CSSStringValue::create(std::move(value));
    }

    std::string value;
    while(input->type() == CSSToken::Type::Ident) {
        if(!value.empty())
            value += ' ';
        value += input->data();
        input.consumeIncludingWhitespace();
    }

    if(value.empty())
        return nullptr;
    return CSSStringValue::create(std::move(value));
}

RefPtr<CSSValue> CSSParser::consumeFontFamily(CSSTokenStream& input)
{
    CSSValueList values;
    while(!input.empty()) {
        auto value = consumeFontFamilyValue(input);
        if(value == nullptr)
            return nullptr;
        values.push_back(value);
    }

    return CSSListValue::create(std::move(values));
}

RefPtr<CSSValue> CSSParser::consumeValue(CSSTokenStream& input, CSSPropertyID id)
{
    switch(id) {
    case CSSPropertyID::Stroke_Miterlimit:
        return consumeNumber(input, false);
    case CSSPropertyID::Stroke_Dashoffset:
        return consumeLengthOrPercent(input, true, true);
    case CSSPropertyID::Stroke_Width:
        return consumeLengthOrPercent(input, false, true);
    case CSSPropertyID::Letter_Spacing:
    case CSSPropertyID::Word_Spacing:
        return consumeLengthOrNormal(input, true, true);
    case CSSPropertyID::Opacity:
    case CSSPropertyID::Fill_Opacity:
    case CSSPropertyID::Stroke_Opacity:
    case CSSPropertyID::Stop_Opacity:
    case CSSPropertyID::Solid_Opacity:
        return consumeNumberOrPercent(input, false);
    case CSSPropertyID::Stroke_Dasharray:
        return consumeDashList(input);
    case CSSPropertyID::Clip_Path:
    case CSSPropertyID::Marker_End:
    case CSSPropertyID::Marker_Mid:
    case CSSPropertyID::Marker_Start:
    case CSSPropertyID::Mask:
        return consumeUrlOrNone(input);
    case CSSPropertyID::Color:
    case CSSPropertyID::Stop_Color:
    case CSSPropertyID::Solid_Color:
        return consumeColor(input);
    case CSSPropertyID::Fill:
    case CSSPropertyID::Stroke:
        return consumeFillOrStroke(input);
    case CSSPropertyID::Font_Weight:
        return consumeFontWeight(input);
    case CSSPropertyID::Font_Size:
        return consumeFontSize(input, true);
    case CSSPropertyID::Font_Family:
        return consumeFontFamily(input);
    case CSSPropertyID::Font_Style: {
        static const idententry_t table[] = {
            {"normal", CSSValueID::Normal},
            {"italic", CSSValueID::Italic},
            {"oblique", CSSValueID::Oblique}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Font_Variant: {
        static const idententry_t table[] = {
            {"normal", CSSValueID::Normal},
            {"small-caps", CSSValueID::SmallCaps}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Fill_Rule:
    case CSSPropertyID::Clip_Rule: {
        static const idententry_t table[] = {
            {"nonzero", CSSValueID::Nonzero},
            {"evenodd", CSSValueID::Evenodd}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Overflow: {
        static const idententry_t table[] = {
            {"auto", CSSValueID::Auto},
            {"visible", CSSValueID::Visible},
            {"hidden", CSSValueID::Hidden}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Stroke_Linecap: {
        static const idententry_t table[] = {
            {"butt", CSSValueID::Butt},
            {"round", CSSValueID::Round},
            {"square", CSSValueID::Square}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Stroke_Linejoin: {
        static const idententry_t table[] = {
            {"miter", CSSValueID::Miter},
            {"round", CSSValueID::Round},
            {"bevel", CSSValueID::Bevel}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Text_Anchor: {
        static const idententry_t table[] = {
            {"start", CSSValueID::Start},
            {"middle", CSSValueID::Middle},
            {"end", CSSValueID::End}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Visibility: {
        static const idententry_t table[] = {
            {"visible", CSSValueID::Visible},
            {"hidden", CSSValueID::Hidden},
            {"collapse", CSSValueID::Collapse}
        };

        return consumeIdent(input, table);
    }

    case CSSPropertyID::Display: {
        static const idententry_t table[] = {
            {"none", CSSValueID::None},
            {"inline", CSSValueID::Inline}
        };

        return consumeIdent(input, table);
    }

    default:
        return nullptr;
    }
}

} // namespace lunasvg
