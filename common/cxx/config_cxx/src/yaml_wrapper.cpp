#include <fmt/format.h>
#include <yaml.h>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include "adstutil_cxx/error_handler.hpp"
#include "config_cxx/yaml_wrapper.hpp"

namespace adst::common {

using common::Error;
using common::OnErrorCallBack;

YamlWrapper::YamlWrapper(const OnErrorCallBack& onErrorCallBack, const std::string& staticDoc,
                         const std::string& fileName)
    : onErrorCallBack_(onErrorCallBack)
{
    bool res = false;
    if (!fileName.empty()) {
        res = initFromFile(fileName);
        if (!res && staticDoc.empty()) {
            onErrorCallBack_({Error{{1, fmt::format("file read Failed:{}", fileName)}}});
        }
    }

    if (!res && !staticDoc.empty()) {
        res = initFromConstString(staticDoc);
    }

    if (!res) {
        onErrorCallBack_({Error{
            {1, fmt::format("yaml config init failed StaticDoc'{}' fileName'{}'", staticDoc, fileName).c_str()}}});
    }
}

bool YamlWrapper::initFromConstString(const std::string& builtInDoc)
{
    /* Initialize parser */
    yaml_parser_t parser;
    if (yaml_parser_initialize(&parser) != 1) {
        onErrorCallBack_(Error{{1, "parser init failed"}});
        return false;
    }

    std::cout << "YamlWrapper: reading from 'builtInDoc':" << std::endl << builtInDoc << std::endl;

    // why unsigned ?.
    yaml_parser_set_input_string(&parser, reinterpret_cast<const unsigned char*>(builtInDoc.c_str()), // NOLINT
                                 builtInDoc.size());
    auto res = parse(&parser);
    yaml_parser_delete(&parser);
    return res;
}

bool YamlWrapper::initFromFile(const std::string& fileName)
{
    FILE* file = fopen(fileName.c_str(), "r"); // NOLINT
    if (file == nullptr) {
        return false;
    }

    std::cout << "YamlWrapper: reading from file: '" << fileName << "'" << std::endl;

    /* Initialize parser */
    yaml_parser_t parser;
    if (yaml_parser_initialize(&parser) != 1) {
        onErrorCallBack_(Error{{1, "parser init failed"}});
        return false;
    }

    yaml_parser_set_input_file(&parser, file);
    auto res = parse(&parser);
    yaml_parser_delete(&parser);

    fclose(file); // NOLINT
    return res;
}

bool YamlWrapper::parse(yaml_parser_t* parser)
{
    bool         prevWasKey = false;
    std::string  keyName;
    YamlNode*    current(root_.get());
    yaml_event_t event;
    do {
        if (yaml_parser_parse(parser, &event) == 0) {
            onErrorCallBack_(Error{{1, "parser error"}});
            return false;
        }

        switch (event.type) {
            /* Block delimeters */
            case YAML_SEQUENCE_START_EVENT: {
                if (!prevWasKey && current->getType() == YamlNode::Type::SEQUENCE) {
                    current->children_.emplace_back(
                        std::make_unique<YamlSequence>(*current, std::to_string(current->children_.size())));
                } else if (!prevWasKey) {
                    // std::cout << "YAML_SEQUENCE_START_EVENT without key" << std::endl;
                    current->children_.emplace_back(std::make_unique<YamlSequence>(*current, ""));
                } else {
                    // std::cout << "YAML_SEQUENCE_START_EVENT with key: " << keyName << std::endl;
                    current->children_.emplace_back(std::make_unique<YamlSequence>(*current, keyName));
                    prevWasKey = false;
                }
                current = current->children_.back().get();
                break;
            }
            case YAML_SEQUENCE_END_EVENT: {
                current = &current->parent_;
                break;
            }
            case YAML_MAPPING_START_EVENT: {
                if (!prevWasKey && current->getType() == YamlNode::Type::SEQUENCE) {
                    current->children_.emplace_back(
                        std::make_unique<YamlMapping>(*current, std::to_string(current->children_.size())));
                } else if (!prevWasKey) {
                    // std::cout << "YAML_MAPPING_START_EVENT without key" << std::endl;
                    current->children_.emplace_back(std::make_unique<YamlMapping>(*current, ""));
                } else {
                    // std::cout << "YAML_MAPPING_START_EVENT with key: " << keyName << std::endl;
                    current->children_.emplace_back(std::make_unique<YamlMapping>(*current, keyName));
                    prevWasKey = false;
                }
                current = current->children_.back().get();
                break;
            }
            case YAML_MAPPING_END_EVENT: {
                current = &current->parent_;
                break;
            }
                /* Data */
            case YAML_SCALAR_EVENT: {
                // suppress warnings of Clang-Tidy for C interface
                std::string value(reinterpret_cast<const char*>(event.data.scalar.value)); // NOLINT
                if (!prevWasKey && current->getType() == YamlNode::Type::SEQUENCE) {
                    // std::cout << "YAML_SCALAR_EVENT under sequence with value: " << value << std::endl;
                    current->children_.emplace_back(
                        std::make_unique<YamlValue>(*current, std::to_string(current->children_.size()), value));
                } else if (!prevWasKey) {
                    prevWasKey = true;
                    keyName    = value;
                    // std::cout << "storing key: " << keyName << std::endl;
                } else {
                    // std::cout << "YAML_SCALAR_EVENT with key: " << keyName << " and value: " << value << std::endl;
                    current->children_.emplace_back(std::make_unique<YamlValue>(*current, keyName, value));
                    prevWasKey = false;
                }
                break;
            }
                // we dont do anything for YAML_ALIAS_EVENT, YAML_STREAM_START_EVENT,
                // YAML_STREAM_END_EVENT, YAML_DOCUMENT_START_EVENT amd YAML_DOCUMENT_END_EVENT
            default:
                break;
        }
        if (event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
    } while (event.type != YAML_STREAM_END_EVENT);

    yaml_event_delete(&event);
    return true;
}

const char* YamlWrapper::getValueImpl(const std::string& accessPath, char separator) const
{
    if (accessPath.empty()) {
        return YamlWrapper::NOT_FOUND;
    }

    std::vector<std::string> pathElements;
    {
        std::stringstream ss(accessPath);
        std::string       element;
        while (std::getline(ss, element, separator)) {
            pathElements.push_back(element);
        }
    }

    YamlNode* current(root_.get());
    bool      pathValid = true;
    while (pathValid) {
        pathValid = false;
        for (size_t i = 0; i < current->children_.size(); ++i) {
            if (pathElements[0] == current->children_[i]->getKey()) {
                if (pathElements.size() == 1) {
                    return current->children_[i]->getValue().c_str();
                } else {
                    pathValid = true;
                    current   = current->children_[i].get();
                    pathElements.erase(pathElements.begin());
                    break;
                }
            }
        }
    }
    return YamlWrapper::NOT_FOUND.c_str();
}

} // namespace adst::common
