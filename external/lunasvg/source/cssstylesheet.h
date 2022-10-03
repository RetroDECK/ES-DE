#ifndef CSSSTYLESHEET_H
#define CSSSTYLESHEET_H

#include "element.h"
#include "pointer.h"

#include <map>
#include <set>

namespace lunasvg {

class CSSValue : public RefCounted<CSSValue> {
public:
    virtual ~CSSValue() = default;
    virtual bool isInitialValue() const { return false; }
    virtual bool isInheritValue() const { return false; }
    virtual bool isIdentValue() const { return false; }
    virtual bool isIntegerValue() const { return false; }
    virtual bool isNumberValue() const { return false; }
    virtual bool isPercentValue() const { return false; }
    virtual bool isLengthValue() const { return false; }
    virtual bool isStringValue() const { return false; }
    virtual bool isUrlValue() const { return false; }
    virtual bool isColorValue() const { return false; }
    virtual bool isPairValue() const { return false; }
    virtual bool isListValue() const { return false; }

protected:
    CSSValue() = default;
};

using CSSValueList = std::vector<RefPtr<CSSValue>>;

class CSSInitialValue final : public CSSValue {
public:
    static RefPtr<CSSInitialValue> create();

    bool isInitialValue() const final { return true; }

private:
    CSSInitialValue() = default;
};

template<>
struct is<CSSInitialValue> {
    static bool check(const CSSValue& value) { return value.isInitialValue(); }
};

class CSSInheritValue final : public CSSValue {
public:
    static RefPtr<CSSInheritValue> create();

    bool isInheritValue() const final { return true; }

private:
    CSSInheritValue() = default;
};

template<>
struct is<CSSInheritValue> {
    static bool check(const CSSValue& value) { return value.isInheritValue(); }
};

enum class CSSValueID {
    Unknown,
    Auto,
    Bevel,
    Bold,
    Bolder,
    Butt,
    Clip,
    Collapse,
    Color,
    CurrentColor,
    End,
    Evenodd,
    Hidden,
    Inherit,
    Initial,
    Inline,
    Italic,
    Large,
    Larger,
    Lighter,
    Medium,
    Middle,
    Miter,
    None,
    Nonzero,
    Normal,
    Oblique,
    Round,
    Small,
    SmallCaps,
    Smaller,
    Square,
    Start,
    Stroke,
    Visible,
    XLarge,
    XSmall,
    XxLarge,
    XxSmall,
    XxxLarge
};

class CSSIdentValue final : public CSSValue {
public:
    static RefPtr<CSSIdentValue> create(CSSValueID value);

    CSSValueID value() const { return m_value; }
    bool isIdentValue() const final { return true; }

private:
    CSSIdentValue(CSSValueID value) : m_value(value) {}
    CSSValueID m_value;
};

template<>
struct is<CSSIdentValue> {
    static bool check(const CSSValue& value) { return value.isIdentValue(); }
};

class CSSIntegerValue final : public CSSValue {
public:
    static RefPtr<CSSIntegerValue> create(int value);

    int value() const { return m_value; }
    bool isIntegerValue() const final { return true; }

private:
    CSSIntegerValue(int value) : m_value(value) {}
    int m_value;
};

template<>
struct is<CSSIntegerValue> {
    static bool check(const CSSValue& value) { return value.isIntegerValue(); }
};

class CSSNumberValue final : public CSSValue {
public:
    static RefPtr<CSSNumberValue> create(double value);

    double value() const { return m_value; }
    bool isNumberValue() const final { return true; }

private:
    CSSNumberValue(double value) : m_value(value) {}
    double m_value;
};

template<>
struct is<CSSNumberValue> {
    static bool check(const CSSValue& value) { return value.isNumberValue(); }
};

class CSSPercentValue final : public CSSValue {
public:
    static RefPtr<CSSPercentValue> create(double value);

    double value() const { return m_value; }
    bool isPercentValue() const final { return true; }

private:
    CSSPercentValue(double value) : m_value(value) {}
    double m_value;
};

template<>
struct is<CSSPercentValue> {
    static bool check(const CSSValue& value) { return value.isPercentValue(); }
};

class CSSLengthValue final : public CSSValue {
public:
    enum class Unit {
        None,
        Ems,
        Exs,
        Pixels,
        Centimeters,
        Millimeters,
        Inches,
        Points,
        Picas,
        ViewportWidth,
        ViewportHeight,
        ViewportMin,
        ViewportMax,
        Rems,
        Chs
    };

    static RefPtr<CSSLengthValue> create(double value, Unit unit);

    double value() const { return m_value; }
    Unit unit() const { return m_unit; }
    bool isLengthValue() const final { return true; }

private:
    CSSLengthValue(double value, Unit unit)
        : m_value(value), m_unit(unit)
    {}

    double m_value;
    Unit m_unit;
};

template<>
struct is<CSSLengthValue> {
    static bool check(const CSSValue& value) { return value.isLengthValue(); }
};

class CSSStringValue final : public CSSValue {
public:
    static RefPtr<CSSStringValue> create(std::string value);

    const std::string& value() const { return m_value; }
    bool isStringValue() const final { return true; }

private:
    CSSStringValue(std::string value) : m_value(std::move(value)) {}
    std::string m_value;
};

template<>
struct is<CSSStringValue> {
    static bool check(const CSSValue& value) { return value.isStringValue(); }
};

class CSSUrlValue final : public CSSValue {
public:
    static RefPtr<CSSUrlValue> create(std::string value);

    const std::string& value() const { return m_value; }
    bool isUrlValue() const final { return true; }

private:
    CSSUrlValue(std::string value) : m_value(std::move(value)) {}
    std::string m_value;
};

template<>
struct is<CSSUrlValue> {
    static bool check(const CSSValue& value) { return value.isUrlValue(); }
};

class CSSColorValue final : public CSSValue {
public:
    static RefPtr<CSSColorValue> create(uint32_t value);
    static RefPtr<CSSColorValue> create(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    uint32_t value() const { return m_value; }
    bool isColorValue() const final { return true; }

private:
    CSSColorValue(uint32_t value) : m_value(value) {}
    uint32_t m_value;
};

template<>
struct is<CSSColorValue> {
    static bool check(const CSSValue& value) { return value.isColorValue(); }
};

class CSSPairValue final : public CSSValue {
public:
    static RefPtr<CSSPairValue> create(RefPtr<CSSValue> first, RefPtr<CSSValue> second);

    const RefPtr<CSSValue>& first() const { return m_first; }
    const RefPtr<CSSValue>& second() const { return m_second; }
    bool isPairValue() const final { return true; }

private:
    CSSPairValue(RefPtr<CSSValue> first, RefPtr<CSSValue> second)
        : m_first(first), m_second(second)
    {}

    RefPtr<CSSValue> m_first;
    RefPtr<CSSValue> m_second;
};

template<>
struct is<CSSPairValue> {
    static bool check(const CSSValue& value) { return value.isPairValue(); }
};

class CSSListValue : public CSSValue {
public:
    static RefPtr<CSSListValue> create(CSSValueList values);

    size_t length() const { return m_values.size(); }
    const RefPtr<CSSValue>& front() const { return m_values.front(); }
    const RefPtr<CSSValue>& back() const { return m_values.back(); }
    const RefPtr<CSSValue>& at(size_t index) const { return m_values.at(index); }
    const CSSValueList& values() const { return m_values; }
    bool isListValue() const final { return true; }

protected:
    CSSListValue(CSSValueList values) : m_values(std::move(values)) {}
    CSSValueList m_values;
};

enum class CSSPropertyID {
    Unknown = 0,
    Clip_Path,
    Clip_Rule,
    Color,
    Display,
    Fill,
    Fill_Opacity,
    Fill_Rule,
    Font_Family,
    Font_Size,
    Font_Style,
    Font_Variant,
    Font_Weight,
    Letter_Spacing,
    Marker_End,
    Marker_Mid,
    Marker_Start,
    Mask,
    Opacity,
    Overflow,
    Solid_Color,
    Solid_Opacity,
    Stop_Color,
    Stop_Opacity,
    Stroke,
    Stroke_Dasharray,
    Stroke_Dashoffset,
    Stroke_Linecap,
    Stroke_Linejoin,
    Stroke_Miterlimit,
    Stroke_Opacity,
    Stroke_Width,
    Text_Anchor,
    Text_Decoration,
    Visibility,
    Word_Spacing
};

CSSPropertyID csspropertyid(const std::string_view& name);

class CSSProperty {
public:
    CSSProperty(CSSPropertyID id, bool important, RefPtr<CSSValue> value)
        : m_id(id), m_important(important), m_value(value)
    {}

    CSSPropertyID id() const { return m_id; }
    bool important() const { return m_important; }
    const RefPtr<CSSValue>& value() const { return m_value; }

private:
    CSSPropertyID m_id;
    bool m_important;
    RefPtr<CSSValue> m_value;
};

using CSSPropertyList = std::vector<CSSProperty>;
using CSSPropertyMap = std::map<CSSPropertyID, CSSProperty>;

struct CSSSimpleSelector;

using CSSSelector = std::vector<CSSSimpleSelector>;
using CSSSelectorList = std::vector<CSSSelector>;

struct CSSSimpleSelector {
    struct Attribute {
        enum class Type {
            None,
            Equals,
            Contains,
            Includes,
            StartsWith,
            EndsWith,
            DashEquals
        };

        Type type{Type::None};
        PropertyID id{PropertyID::Unknown};
        std::string value;
    };

    struct Pseudo {
        enum class Type {
            Unknown,
            Empty,
            Root,
            Is,
            Not,
            FirstChild,
            LastChild,
            OnlyChild,
            FirstOfType,
            LastOfType,
            OnlyOfType,
            NthChild,
            NthLastChild,
            NthOfType,
            NthLastOfType
        };

        using MatchPattern = std::pair<int16_t, int16_t>;

        bool matchnth(size_t count) const;

        Type type{Type::Unknown};
        MatchPattern pattern;
        CSSSelectorList selectors;
    };

    enum class Combinator {
        None,
        Descendant,
        Child,
        DirectAdjacent,
        InDirectAdjacent
    };

    Combinator combinator{Combinator::Descendant};
    ElementID id{ElementID::Star};
    std::vector<Attribute> attributes;
    std::vector<Pseudo> pseudos;
};

class CSSRule {
public:
    static std::unique_ptr<CSSRule> create(CSSSelectorList selectors, CSSPropertyList properties);

    const CSSSelectorList& selectors() const { return m_selectors; }
    const CSSPropertyList& properties() const { return m_properties; }

private:
    CSSRule(CSSSelectorList selectors, CSSPropertyList properties)
        : m_selectors(std::move(selectors)), m_properties(std::move(properties))
    {}

    CSSSelectorList m_selectors;
    CSSPropertyList m_properties;
};

using CSSRuleList = std::vector<std::unique_ptr<CSSRule>>;

class CSSRuleData {
public:
    CSSRuleData(const CSSSelector& selector, const CSSPropertyList& properties, uint32_t specificity, uint32_t position)
        : m_selector(selector), m_properties(properties), m_specificity(specificity), m_position(position)
    {}

    const CSSSelector& selector() const { return m_selector; }
    const CSSPropertyList& properties() const { return m_properties; }
    const uint32_t& specificity() const { return m_specificity; }
    const uint32_t& position() const { return m_position; }

    bool match(const Element* element) const;

private:
    static bool matchSimpleSelector(const CSSSimpleSelector& selector, const Element* element);
    static bool matchAttributeSelector(const CSSSimpleSelector::Attribute& attribute, const Element* element);
    static bool matchPseudoClassSelector(const CSSSimpleSelector::Pseudo& pseudo, const Element* element);

private:
    const CSSSelector& m_selector;
    const CSSPropertyList& m_properties;
    uint32_t m_specificity;
    uint32_t m_position;
};

inline bool operator<(const CSSRuleData& a, const CSSRuleData& b) { return std::tie(a.specificity(), a.position()) < std::tie(b.specificity(), b.position()); }
inline bool operator>(const CSSRuleData& a, const CSSRuleData& b) { return std::tie(a.specificity(), a.position()) > std::tie(b.specificity(), b.position()); }

using CSSRuleSet = std::multiset<CSSRuleData>;

class ComputedStyle;

class CSSStyleSheet {
public:
    CSSStyleSheet() = default;

    void parse(const std::string_view& content);
    bool empty() const { return m_ruleList.empty(); }

    RefPtr<ComputedStyle> styleForElement(const Element* element, const RefPtr<ComputedStyle>& parentStyle) const;

private:
    CSSRuleList m_ruleList;
    CSSRuleSet m_ruleSet;
};

} // namespace lunasvg

#endif // CSSSTYLESHEET_H
