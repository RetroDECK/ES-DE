#include "cssstylesheet.h"
#include "cssparser.h"

#include <cassert>

namespace lunasvg {

RefPtr<CSSInitialValue> CSSInitialValue::create()
{
    static auto value = adoptPtr(new CSSInitialValue);
    return value;
}

RefPtr<CSSInheritValue> CSSInheritValue::create()
{
    static auto value = adoptPtr(new CSSInheritValue);
    return value;
}

RefPtr<CSSIdentValue> CSSIdentValue::create(CSSValueID value)
{
    static std::map<CSSValueID, RefPtr<CSSIdentValue>> table;
    auto it = table.find(value);
    if(it == table.end()) {
        auto item = adoptPtr(new CSSIdentValue(value));
        table.emplace(value, item);
        return item;
    }

    return it->second;
}

RefPtr<CSSIntegerValue> CSSIntegerValue::create(int value)
{
    return adoptPtr(new CSSIntegerValue(value));
}

RefPtr<CSSNumberValue> CSSNumberValue::create(double value)
{
    return adoptPtr(new CSSNumberValue(value));
}

RefPtr<CSSPercentValue> CSSPercentValue::create(double value)
{
    return adoptPtr(new CSSPercentValue(value));
}

RefPtr<CSSLengthValue> CSSLengthValue::create(double value, Unit unit)
{
    return adoptPtr(new CSSLengthValue(value, unit));
}

RefPtr<CSSStringValue> CSSStringValue::create(std::string value)
{
    return adoptPtr(new CSSStringValue(std::move(value)));
}

RefPtr<CSSUrlValue> CSSUrlValue::create(std::string value)
{
    return adoptPtr(new CSSUrlValue(std::move(value)));
}

RefPtr<CSSColorValue> CSSColorValue::create(uint32_t value)
{
    return adoptPtr(new CSSColorValue(value));
}

RefPtr<CSSColorValue> CSSColorValue::create(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return adoptPtr(new CSSColorValue(a << 24 | r << 16 | g << 8 | b));
}

RefPtr<CSSPairValue> CSSPairValue::create(RefPtr<CSSValue> first, RefPtr<CSSValue> second)
{
    return adoptPtr(new CSSPairValue(std::move(first), std::move(second)));
}

RefPtr<CSSListValue> CSSListValue::create(CSSValueList values)
{
    return adoptPtr(new CSSListValue(std::move(values)));
}

CSSPropertyID csspropertyid(const std::string_view& name)
{
    static const struct {
        std::string_view name;
        CSSPropertyID value;
    } table[] = {
        {"clip-path", CSSPropertyID::Clip_Path},
        {"clip-rule", CSSPropertyID::Clip_Rule},
        {"color", CSSPropertyID::Color},
        {"display", CSSPropertyID::Display},
        {"fill", CSSPropertyID::Fill},
        {"fill-opacity", CSSPropertyID::Fill_Opacity},
        {"fill-rule", CSSPropertyID::Fill_Rule},
        {"font-family", CSSPropertyID::Font_Family},
        {"font-size", CSSPropertyID::Font_Size},
        {"font-style", CSSPropertyID::Font_Style},
        {"font-variant", CSSPropertyID::Font_Variant},
        {"font-weight", CSSPropertyID::Font_Weight},
        {"letter-spacing", CSSPropertyID::Letter_Spacing},
        {"marker-end", CSSPropertyID::Marker_End},
        {"marker-mid", CSSPropertyID::Marker_Mid},
        {"marker-start", CSSPropertyID::Marker_Start},
        {"mask", CSSPropertyID::Mask},
        {"opacity", CSSPropertyID::Opacity},
        {"overflow", CSSPropertyID::Overflow},
        {"solid-color", CSSPropertyID::Solid_Color},
        {"solid-opacity", CSSPropertyID::Solid_Opacity},
        {"stop-color", CSSPropertyID::Stop_Color},
        {"stop-opacity", CSSPropertyID::Stop_Opacity},
        {"stroke", CSSPropertyID::Stroke},
        {"stroke-dasharray", CSSPropertyID::Stroke_Dasharray},
        {"stroke-dashoffset", CSSPropertyID::Stroke_Dashoffset},
        {"stroke-linecap", CSSPropertyID::Stroke_Linecap},
        {"stroke-linejoin", CSSPropertyID::Stroke_Linejoin},
        {"stroke-miterlimit", CSSPropertyID::Stroke_Miterlimit},
        {"stroke-opacity", CSSPropertyID::Stroke_Opacity},
        {"stroke-width", CSSPropertyID::Stroke_Width},
        {"text-anchor", CSSPropertyID::Text_Anchor},
        {"text-decoration", CSSPropertyID::Text_Decoration},
        {"visibility", CSSPropertyID::Visibility},
        {"word-spacing", CSSPropertyID::Word_Spacing},
    };

    auto it = std::lower_bound(table, std::end(table), name, [](auto& item, auto& name) { return item.name < name; });
    if(it != std::end(table) && it->name == name)
        return it->value;
    return CSSPropertyID::Unknown;
}

bool CSSSimpleSelector::Pseudo::matchnth(size_t count) const
{
    auto [a, b] = pattern;
    if(a == 0)
        return count == b;
    if(a > 0) {
        if(count < b)
            return false;
        return (count - b) % a == 0;
    }

    if(count > b)
        return false;
    return (b - count) % -a == 0;
}

std::unique_ptr<CSSRule> CSSRule::create(CSSSelectorList selectors, CSSPropertyList properties)
{
    return std::unique_ptr<CSSRule>(new CSSRule(std::move(selectors), std::move(properties)));
}

bool CSSRuleData::match(const Element* element) const
{
    if(m_selector.empty())
        return false;

    if(m_selector.size() == 1)
        return matchSimpleSelector(m_selector.front(), element);

    auto it = m_selector.rbegin();
    auto end = m_selector.rend();
    if(!matchSimpleSelector(*it, element))
        return false;
    ++it;

    while(it != end) {
        switch(it->combinator) {
        case CSSSimpleSelector::Combinator::Child:
        case CSSSimpleSelector::Combinator::Descendant:
            element = element->parent;
            break;
        case CSSSimpleSelector::Combinator::DirectAdjacent:
        case CSSSimpleSelector::Combinator::InDirectAdjacent:
            element = element->previousElement();
            break;
        default:
            assert(false);
        }

        if(element == nullptr)
            return false;

        auto match = matchSimpleSelector(*it, element);
        if(!match && (it->combinator != CSSSimpleSelector::Combinator::Descendant && it->combinator != CSSSimpleSelector::Combinator::InDirectAdjacent))
            return false;

        if(match || (it->combinator != CSSSimpleSelector::Combinator::Descendant && it->combinator != CSSSimpleSelector::Combinator::InDirectAdjacent))
            ++it;
    }

    return true;
}

bool CSSRuleData::matchSimpleSelector(const CSSSimpleSelector& selector, const Element* element)
{
    if(selector.id != ElementID::Star && selector.id != element->id)
        return false;

    for(auto& attribute : selector.attributes)
        if(!matchAttributeSelector(attribute, element))
            return false;

    for(auto& pseudo : selector.pseudos)
        if(!matchPseudoClassSelector(pseudo, element))
            return false;

    return true;
}

bool CSSRuleData::matchAttributeSelector(const CSSSimpleSelector::Attribute& attribute, const Element* element)
{
    auto& value = element->get(attribute.id);
    switch(attribute.type) {
    case CSSSimpleSelector::Attribute::Type::None:
        return !value.empty();
    case CSSSimpleSelector::Attribute::Type::Equals:
        return equals(value, attribute.value, false);
    case CSSSimpleSelector::Attribute::Type::Contains:
        return contains(value, attribute.value, false);
    case CSSSimpleSelector::Attribute::Type::Includes:
        return includes(value, attribute.value, false);
    case CSSSimpleSelector::Attribute::Type::StartsWith:
        return startswith(value, attribute.value, false);
    case CSSSimpleSelector::Attribute::Type::EndsWith:
        return endswith(value, attribute.value, false);
    case CSSSimpleSelector::Attribute::Type::DashEquals:
        return dashequals(value, attribute.value, false);
    default:
        return false;
    }
}

bool CSSRuleData::matchPseudoClassSelector(const CSSSimpleSelector::Pseudo& pseudo, const Element* element)
{
    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::Empty)
        return element->children.empty();
    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::Root)
        return element->parent == nullptr;
    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::Is) {
        for(auto& selector : pseudo.selectors) {
            for(auto& sel : selector) {
                if(!matchSimpleSelector(sel, element)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::Not) {
        for(auto& selector : pseudo.selectors) {
            for(auto& sel : selector) {
                if(matchSimpleSelector(sel, element)) {
                    return false;
                }
            }
        }

        return true;
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::FirstChild)
        return !element->previousElement();
    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::LastChild)
        return !element->nextElement();
    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::OnlyChild)
        return !(element->previousElement() || element->nextElement());
    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::FirstOfType) {
        auto child = element->nextElement();
        while(child) {
            if(child->id == element->id)
                return false;
            child = element->nextElement();
        }

        return true;
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::LastOfType) {
        auto child = element->nextElement();
        while(child) {
            if(child->id == element->id)
                return false;
            child = element->nextElement();
        }

        return true;
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::OnlyOfType) {
        auto child = element->nextElement();
        while(child) {
            if(child->id == element->id)
                return false;
            child = element->nextElement();
        }

        child = element->nextElement();
        while(child) {
            if(child->id == element->id)
                return false;
            child = element->nextElement();
        }

        return true;
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::NthChild) {
        int count = 1;
        auto child = element->previousElement();
        while(child) {
            count += 1;
            child = element->previousElement();
        }

        return pseudo.matchnth(count);
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::NthLastChild) {
        int count = 1;
        auto child = element->nextElement();
        while(child) {
            count += 1;
            child = element->nextElement();
        }

        return pseudo.matchnth(count);
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::NthOfType) {
        int count = 1;
        auto child = element->previousElement();
        while(child && child->id == element->id) {
            count += 1;
            child = element->previousElement();
        }

        return pseudo.matchnth(count);
    }

    if(pseudo.type == CSSSimpleSelector::Pseudo::Type::NthLastOfType) {
        int count = 1;
        auto child = element->nextElement();
        while(child && child->id == element->id) {
            count += 1;
            child = element->nextElement();
        }

        return pseudo.matchnth(count);
    }

    return false;
}

void CSSStyleSheet::parse(const std::string_view& content)
{
    auto position = m_ruleList.size();
    CSSParser::parseSheet(m_ruleList, content);
    for(; position < m_ruleList.size(); ++position) {
        auto& rule = m_ruleList[position];
        auto& properties = rule->properties();
        for(auto& selector : rule->selectors()) {
            uint32_t specificity = 0;
            for(auto& sel : selector) {
                specificity += (sel.id == ElementID::Star) ? 0x0 : 0x1;
                for(auto& attribute : sel.attributes) {
                    specificity += (attribute.id == PropertyID::Id) ? 0x10000 : 0x100;
                }
            }

            m_ruleSet.emplace(selector, properties, specificity, position);
        }
    }
}

} // namespace lunasvg
