//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  InputConfig.cpp
//
//  Input device configuration functions.
//

#include "InputConfig.h"

#include "Log.h"

#include <pugixml.hpp>

InputConfig::InputConfig(int deviceId, const std::string& deviceName, const std::string& deviceGUID)
    : mDeviceId {deviceId}
    , mDeviceName {deviceName}
    , mDeviceGUID {deviceGUID}
{
}

std::string InputConfig::inputTypeToString(InputType type)
{
    switch (type) {
        case TYPE_AXIS:
            return "axis";
        case TYPE_BUTTON:
            return "button";
        case TYPE_KEY:
            return "key";
        case TYPE_CEC_BUTTON:
            return "cec-button";
        default:
            return "error";
    }
}

InputType InputConfig::stringToInputType(const std::string& type)
{
    if (type == "axis")
        return TYPE_AXIS;
    if (type == "button")
        return TYPE_BUTTON;
    if (type == "key")
        return TYPE_KEY;
    if (type == "cec-button")
        return TYPE_CEC_BUTTON;
    return TYPE_COUNT;
}

std::string InputConfig::toLower(std::string str)
{
    for (unsigned int i = 0; i < str.length(); ++i)
        str[i] = static_cast<char>(tolower(str[i]));

    return str;
}

void InputConfig::mapInput(const std::string& name, Input input)
{
    mNameMap[toLower(name)] = input;
}

void InputConfig::unmapInput(const std::string& name)
{
    auto it = mNameMap.find(toLower(name));
    if (it != mNameMap.cend())
        mNameMap.erase(it);
}

bool InputConfig::isMappedTo(const std::string& name, Input input)
{
    Input comp;
    if (!getInputByName(name, &comp))
        return false;

    if (comp.configured && comp.type == input.type && comp.id == input.id) {
        if (comp.type == TYPE_AXIS)
            return input.value == 0 || comp.value == input.value;
        else
            return true;
    }
    return false;
}

bool InputConfig::isMappedLike(const std::string& name, Input input)
{
    if (name == "left") {
        return isMappedTo("left", input) || isMappedTo("leftthumbstickleft", input) ||
               isMappedTo("rightthumbstickleft", input);
    }
    else if (name == "right") {
        return isMappedTo("right", input) || isMappedTo("leftthumbstickright", input) ||
               isMappedTo("rightthumbstickright", input);
    }
    else if (name == "up") {
        return isMappedTo("up", input) || isMappedTo("leftthumbstickup", input) ||
               isMappedTo("rightthumbstickup", input);
    }
    else if (name == "down") {
        return isMappedTo("down", input) || isMappedTo("leftthumbstickdown", input) ||
               isMappedTo("rightthumbstickdown", input);
    }
    else if (name == "leftshoulder") {
        return isMappedTo("leftshoulder", input) || isMappedTo("pageup", input);
    }
    else if (name == "rightshoulder") {
        return isMappedTo("rightshoulder", input) || isMappedTo("pagedown", input);
    }
    else if (name == "lefttrigger") {
        return isMappedTo("lefttrigger", input) || isMappedTo("home", input);
    }
    else if (name == "righttrigger") {
        return isMappedTo("righttrigger", input) || isMappedTo("end", input);
    }
    return isMappedTo(name, input);
}

std::vector<std::string> InputConfig::getMappedTo(Input input)
{
    std::vector<std::string> maps;

    for (auto it = mNameMap.cbegin(); it != mNameMap.cend(); ++it) {
        Input chk = it->second;

        if (!chk.configured)
            continue;

        if (chk.device == input.device && chk.type == input.type && chk.id == input.id) {
            if (input.type == TYPE_AXIS) {
                if (input.value == 0 || chk.value == input.value)
                    maps.push_back(it->first);
            }
            else {
                maps.push_back(it->first);
            }
        }
    }
    return maps;
}

bool InputConfig::getInputByName(const std::string& name, Input* result)
{
    auto it = mNameMap.find(toLower(name));
    if (it != mNameMap.cend()) {
        *result = it->second;
        return true;
    }
    return false;
}

int InputConfig::getInputIDByName(const std::string& name)
{
    auto it = mNameMap.find(toLower(name));
    if (it != mNameMap.cend()) {
        return it->second.id;
    }
    return -1;
}

void InputConfig::loadFromXML(pugi::xml_node& node)
{
    clear();

    for (pugi::xml_node input = node.child("input"); input; input = input.next_sibling("input")) {
        std::string name = input.attribute("name").as_string();
        std::string type = input.attribute("type").as_string();
        InputType typeEnum = stringToInputType(type);

        if (typeEnum == TYPE_COUNT) {
            LOG(LogError) << "InputConfig load error - input of type \"" << type
                          << "\" is invalid! Skipping input \"" << name << "\".\n";
            continue;
        }

        int id = input.attribute("id").as_int();
        int value = input.attribute("value").as_int();

        if (value == 0) {
            LOG(LogWarning) << "InputConfig value is 0 for " << type << " " << id << "!\n";
        }

        mNameMap[toLower(name)] = Input(mDeviceId, typeEnum, id, value, true);
    }
}

void InputConfig::writeToXML(pugi::xml_node& parent)
{
    pugi::xml_node cfg = parent.append_child("inputConfig");

    if (mDeviceId == DEVICE_KEYBOARD) {
        cfg.append_attribute("type") = "keyboard";
        cfg.append_attribute("deviceName") = "Keyboard";
    }
    else if (mDeviceId == DEVICE_CEC) {
        cfg.append_attribute("type") = "cec";
        cfg.append_attribute("deviceName") = "CEC";
    }
    else {
        cfg.append_attribute("type") = "controller";
        cfg.append_attribute("deviceName") = mDeviceName.c_str();
    }

    cfg.append_attribute("deviceGUID") = mDeviceGUID.c_str();

    for (auto it = mNameMap.cbegin(); it != mNameMap.cend(); ++it) {
        if (!it->second.configured)
            continue;

        pugi::xml_node input = cfg.append_child("input");
        input.append_attribute("name") = it->first.c_str();
        input.append_attribute("type") = inputTypeToString(it->second.type).c_str();
        input.append_attribute("id").set_value(it->second.id);
        input.append_attribute("value").set_value(it->second.value);
    }
}
