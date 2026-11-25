#include "save_xml_node.h"
#include "save_xml_layout.h"
#include "save_xml.h"

#include <cstring>
#include <string>

namespace x4n {

namespace {

template <typename T>
T& at(void* node, size_t offset) {
    return *reinterpret_cast<T*>(static_cast<uint8_t*>(node) + offset);
}

template <typename T>
const T& at(const void* node, size_t offset) {
    return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(node) + offset);
}

// A small vector inside a node: its entries inline at its start while its
// capacity is the inline capacity, otherwise a pointer to them there.
const uint8_t* entries(const void* node, size_t data, size_t capacity) {
    return at<size_t>(node, capacity) == X4_XML_NODE_INLINE_CAPACITY
        ? static_cast<const uint8_t*>(node) + data
        : at<const uint8_t*>(node, data);
}

} // namespace

const X4GameFunctions* SaveXmlNode::s_game = nullptr;
SaveXmlNode::LogFn     SaveXmlNode::s_log  = nullptr;

void SaveXmlNode::attach(const X4GameFunctions* game, LogFn log) {
    s_game = game;
    s_log = log;
}

void* SaveXmlNode::create(int32_t element) {
    if (!s_game || !s_game->GameAlloc) return nullptr;
    void* node = s_game->GameAlloc(X4_XML_NODE_SIZE, 0, 0, 0, X4_XML_NODE_ALIGNMENT);
    if (!node) return nullptr;

    // As the game's writers construct a node; the 8-byte store at the prefix
    // also clears the element, so the element is written after it.
    at<uint16_t>(node, X4_XML_NODE_OFFSET_SOURCE_LINE) = X4_XML_NODE_NO_SOURCE_LINE;
    at<int32_t>(node, X4_XML_NODE_OFFSET_SOURCE_COLUMN) = X4_XML_NODE_NO_SOURCE_COLUMN;
    at<uint64_t>(node, X4_XML_NODE_OFFSET_PREFIX) = 0;
    at<uint64_t>(node, X4_XML_NODE_OFFSET_TEXT) = 0;
    at<size_t>(node, X4_XML_NODE_OFFSET_ATTRIBUTE_CAPACITY) = X4_XML_NODE_INLINE_CAPACITY;
    at<size_t>(node, X4_XML_NODE_OFFSET_ATTRIBUTE_COUNT) = 0;
    at<size_t>(node, X4_XML_NODE_OFFSET_CHILD_CAPACITY) = X4_XML_NODE_INLINE_CAPACITY;
    at<size_t>(node, X4_XML_NODE_OFFSET_CHILD_COUNT) = 0;
    at<uint8_t>(node, X4_XML_NODE_OFFSET_FLAGS) = X4_XML_NODE_FLAG_ELEMENT;
    at<int32_t>(node, X4_XML_NODE_OFFSET_ELEMENT) = element;
    return node;
}

bool SaveXmlNode::set_attribute(void* node, int32_t attribute, const char* value, const char* ext_id) {
    if (!node || !value || !s_game || !s_game->XmlNode_SetAttribute) return false;
    if (const size_t size = std::strlen(value); size > kMaxValueBytes) {
        if (s_log) {
            const char* name = SaveXml::format_xml_name(&attribute);
            const std::string line = std::string("SaveXML: ") + (ext_id ? ext_id : "an extension") + ": '" +
                                     (name ? name : "") + "' refused a value of " + std::to_string(size) +
                                     " bytes, over the save's limit of " + std::to_string(kMaxValueBytes);
            s_log(line.c_str());
        }
        return false;
    }
    s_game->XmlNode_SetAttribute(node, attribute, value);
    return true;
}

bool SaveXmlNode::append_child(void* parent, void* child) {
    if (!parent || !child || !s_game || !s_game->XmlNodeVector_PushBack) return false;
    void* const item = child;
    s_game->XmlNodeVector_PushBack(static_cast<uint8_t*>(parent) + X4_XML_NODE_OFFSET_CHILDREN, &item);
    return true;
}

const char* SaveXmlNode::get_attribute(const void* node, int32_t attribute) {
    if (!node) return nullptr;
    const size_t count = at<size_t>(node, X4_XML_NODE_OFFSET_ATTRIBUTE_COUNT);
    const uint8_t* data =
        entries(node, X4_XML_NODE_OFFSET_ATTRIBUTES, X4_XML_NODE_OFFSET_ATTRIBUTE_CAPACITY);
    for (size_t i = 0; i < count; ++i) {
        const uint8_t* entry = data + i * X4_XML_ATTRIBUTE_SIZE;
        if (at<int32_t>(entry, X4_XML_ATTRIBUTE_OFFSET_ID) == attribute) {
            return at<std::string>(entry, X4_XML_ATTRIBUTE_OFFSET_VALUE).c_str();
        }
    }
    return nullptr;
}

int32_t SaveXmlNode::element(const void* node) {
    if (!node || (at<uint8_t>(node, X4_XML_NODE_OFFSET_FLAGS) & X4_XML_NODE_FLAG_TEXT)) return 0;
    return at<int32_t>(node, X4_XML_NODE_OFFSET_ELEMENT);
}

size_t SaveXmlNode::children(const void* node, void** out, size_t capacity) {
    if (!node) return 0;
    const size_t count = at<size_t>(node, X4_XML_NODE_OFFSET_CHILD_COUNT);
    if (out) {
        const uint8_t* data = entries(node, X4_XML_NODE_OFFSET_CHILDREN, X4_XML_NODE_OFFSET_CHILD_CAPACITY);
        for (size_t i = 0; i < count && i < capacity; ++i) out[i] = at<void*>(data, i * sizeof(void*));
    }
    return count;
}

} // namespace x4n
