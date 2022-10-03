#include "element.h"
#include "parser.h"
#include "svgelement.h"

#include <algorithm>

namespace lunasvg {

ElementID elementid(const std::string_view& name)
{
    static const struct {
        std::string_view name;
        ElementID value;
    } table[] = {
        {"a", ElementID::A},
        {"circle", ElementID::Circle},
        {"clipPath", ElementID::ClipPath},
        {"defs", ElementID::Defs},
        {"ellipse", ElementID::Ellipse},
        {"g", ElementID::G},
        {"image", ElementID::Image},
        {"line", ElementID::Line},
        {"linearGradient", ElementID::LinearGradient},
        {"marker", ElementID::Marker},
        {"mask", ElementID::Mask},
        {"path", ElementID::Path},
        {"pattern", ElementID::Pattern},
        {"polygon", ElementID::Polygon},
        {"polyline", ElementID::Polyline},
        {"radialGradient", ElementID::RadialGradient},
        {"rect", ElementID::Rect},
        {"solidColor", ElementID::SolidColor},
        {"stop", ElementID::Stop},
        {"style", ElementID::Style},
        {"svg", ElementID::Svg},
        {"switch", ElementID::Switch},
        {"symbol", ElementID::Symbol},
        {"text", ElementID::Text},
        {"textPath", ElementID::TextPath},
        {"tref", ElementID::Tref},
        {"tspan", ElementID::Tspan},
        {"use", ElementID::Use}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](auto& item, auto& name) { return item.name < name; });
    if(it != std::end(table) && it->name == name)
        return it->value;
    return ElementID::Unknown;
}

PropertyID propertyid(const std::string_view& name)
{
    static const struct {
        std::string_view name;
        PropertyID value;
    } table[] = {
        {"class", PropertyID::Class},
        {"clip-path", PropertyID::Clip_Path},
        {"clip-rule", PropertyID::Clip_Rule},
        {"clipPathUnits", PropertyID::ClipPathUnits},
        {"color", PropertyID::Color},
        {"cx", PropertyID::Cx},
        {"cy", PropertyID::Cy},
        {"d", PropertyID::D},
        {"dx", PropertyID::Dx},
        {"dy", PropertyID::Dy},
        {"display", PropertyID::Display},
        {"fill", PropertyID::Fill},
        {"fill-opacity", PropertyID::Fill_Opacity},
        {"fill-rule", PropertyID::Fill_Rule},
        {"font-family", PropertyID::Font_Family},
        {"font-size", PropertyID::Font_Size},
        {"font-style", PropertyID::Font_Style},
        {"font-variant", PropertyID::Font_Variant},
        {"font-weight", PropertyID::Font_Weight},
        {"fx", PropertyID::Fx},
        {"fy", PropertyID::Fy},
        {"gradientTransform", PropertyID::GradientTransform},
        {"gradientUnits", PropertyID::GradientUnits},
        {"height", PropertyID::Height},
        {"href", PropertyID::Href},
        {"id", PropertyID::Id},
        {"letter-spacing", PropertyID::Letter_Spacing},
        {"marker-end", PropertyID::Marker_End},
        {"marker-mid", PropertyID::Marker_Mid},
        {"marker-start", PropertyID::Marker_Start},
        {"markerHeight", PropertyID::MarkerHeight},
        {"markerUnits", PropertyID::MarkerUnits},
        {"markerWidth", PropertyID::MarkerWidth},
        {"mask", PropertyID::Mask},
        {"maskContentUnits", PropertyID::MaskContentUnits},
        {"maskUnits", PropertyID::MaskUnits},
        {"offset", PropertyID::Offset},
        {"opacity", PropertyID::Opacity},
        {"orient", PropertyID::Orient},
        {"overflow", PropertyID::Overflow},
        {"patternContentUnits", PropertyID::PatternContentUnits},
        {"patternTransform", PropertyID::PatternTransform},
        {"patternUnits", PropertyID::PatternUnits},
        {"points", PropertyID::Points},
        {"preserveAspectRatio", PropertyID::PreserveAspectRatio},
        {"r", PropertyID::R},
        {"refX", PropertyID::RefX},
        {"refY", PropertyID::RefY},
        {"rotate", PropertyID::Rotate},
        {"rx", PropertyID::Rx},
        {"ry", PropertyID::Ry},
        {"solid-color", PropertyID::Solid_Color},
        {"solid-opacity", PropertyID::Solid_Opacity},
        {"spreadMethod", PropertyID::SpreadMethod},
        {"startOffset", PropertyID::StartOffset},
        {"stop-color", PropertyID::Stop_Color},
        {"stop-opacity", PropertyID::Stop_Opacity},
        {"stroke", PropertyID::Stroke},
        {"stroke-dasharray", PropertyID::Stroke_Dasharray},
        {"stroke-dashoffset", PropertyID::Stroke_Dashoffset},
        {"stroke-linecap", PropertyID::Stroke_Linecap},
        {"stroke-linejoin", PropertyID::Stroke_Linejoin},
        {"stroke-miterlimit", PropertyID::Stroke_Miterlimit},
        {"stroke-opacity", PropertyID::Stroke_Opacity},
        {"stroke-width", PropertyID::Stroke_Width},
        {"style", PropertyID::Style},
        {"text-anchor", PropertyID::Text_Anchor},
        {"text-decoration", PropertyID::Text_Decoration},
        {"transform", PropertyID::Transform},
        {"viewBox", PropertyID::ViewBox},
        {"visibility", PropertyID::Visibility},
        {"width", PropertyID::Width},
        {"word-spacing", PropertyID::Word_Spacing},
        {"x", PropertyID::X},
        {"x1", PropertyID::X1},
        {"x2", PropertyID::X2},
        {"y", PropertyID::Y},
        {"y1", PropertyID::Y1},
        {"y2", PropertyID::Y2}
    };

    auto it = std::lower_bound(table, std::end(table), name, [](auto& item, auto& name) { return item.name < name; });
    if(it != std::end(table) && it->name == name)
        return it->value;
    return PropertyID::Unknown;
}

void PropertyList::set(PropertyID id, const std::string& value, int specificity)
{
    auto property = get(id);
    if(property == nullptr)
    {
        Property property{id, value, specificity};
        m_properties.push_back(std::move(property));
        return;
    }

    if(property->specificity > specificity)
        return;

    property->specificity = specificity;
    property->value = value;
}

Property* PropertyList::get(PropertyID id) const
{
    auto data = m_properties.data();
    auto end = data + m_properties.size();
    while(data < end)
    {
        if(data->id == id)
            return const_cast<Property*>(data);
        ++data;
    }

    return nullptr;
}

void PropertyList::add(const Property& property)
{
    set(property.id, property.value, property.specificity);
}

void PropertyList::add(const PropertyList& properties)
{
    auto it = properties.m_properties.begin();
    auto end = properties.m_properties.end();
    for(;it != end;++it)
        add(*it);
}

void Node::layout(LayoutContext*, LayoutContainer*) const
{
}

std::unique_ptr<Node> TextNode::clone() const
{
    auto node = std::make_unique<TextNode>();
    node->text = text;
    return std::move(node);
}

Element::Element(ElementID id)
    : id(id)
{
}

void Element::set(PropertyID id, const std::string& value, int specificity)
{
    properties.set(id, value, specificity);
}

static const std::string EmptyString;

const std::string& Element::get(PropertyID id) const
{
    auto property = properties.get(id);
    if(property == nullptr)
        return EmptyString;

    return property->value;
}

static const std::string InheritString{"inherit"};

const std::string& Element::find(PropertyID id) const
{
    auto element = this;
    do {
        auto& value = element->get(id);
        if(!value.empty() && value != InheritString)
            return value;
        element = element->parent;
    } while(element);

    return EmptyString;
}

bool Element::has(PropertyID id) const
{
    return properties.get(id);
}

Element* Element::previousElement() const
{
    if(parent == nullptr)
        return nullptr;

    Element* element = nullptr;
    auto it = parent->children.begin();
    auto end = parent->children.end();
    for(;it != end;++it)
    {
        auto node = it->get();
        if(node->isText())
            continue;

        if(node == this)
            return element;
        element = static_cast<Element*>(node);
    }

    return nullptr;
}

Element* Element::nextElement() const
{
    if(parent == nullptr)
        return nullptr;

    Element* element = nullptr;
    auto it = parent->children.rbegin();
    auto end = parent->children.rend();
    for(;it != end;++it)
    {
        auto node = it->get();
        if(node->isText())
            continue;

        if(node == this)
            return element;
        element = static_cast<Element*>(node);
    }

    return nullptr;
}

Node* Element::addChild(std::unique_ptr<Node> child)
{
    child->parent = this;
    children.push_back(std::move(child));
    return &*children.back();
}

void Element::layoutChildren(LayoutContext* context, LayoutContainer* current) const
{
    for(auto& child : children)
        child->layout(context, current);
}

Rect Element::currentViewport() const
{
    if(parent == nullptr)
    {
        auto element = static_cast<const SVGElement*>(this);
        if(element->has(PropertyID::ViewBox))
            return element->viewBox(); 
        return Rect{0, 0, 300, 150};
    }

    if(parent->id == ElementID::Svg)
    {
        auto element = static_cast<SVGElement*>(parent);
        if(element->has(PropertyID::ViewBox))
            return element->viewBox();

        LengthContext lengthContext(element);
        auto _x = lengthContext.valueForLength(element->x(), LengthMode::Width);
        auto _y = lengthContext.valueForLength(element->y(), LengthMode::Height);
        auto _w = lengthContext.valueForLength(element->width(), LengthMode::Width);
        auto _h = lengthContext.valueForLength(element->height(), LengthMode::Height);
        return Rect{_x, _y, _w, _h};
    }

    return parent->currentViewport();
}

} // namespace lunasvg
