// ---------------------------------------------------------------------------
// struct_verify.h -- Runtime verification of libxml2 struct layout
// ---------------------------------------------------------------------------
// The game statically links libxml2. We need to confirm that xmlNode, xmlAttr,
// xmlDoc structs match the standard layout before constructing DOM trees.
//
// Call verify_libxml2_layout() once after the first savegame loads.
// It inspects the DOM tree returned by the original ParseFromMemory to
// confirm field offsets match our assumptions.
// ---------------------------------------------------------------------------
#pragma once
#include <cstdint>

// libxml2 element types (from libxml/tree.h -- stable since 2.0)
enum XmlElementType : int {
    XML_ELEMENT_NODE    = 1,
    XML_ATTRIBUTE_NODE  = 2,
    XML_TEXT_NODE       = 3,
    XML_DOCUMENT_NODE   = 9,
};

// Our assumed offsets for xmlNode on x64.
// These are verified at runtime before we construct any DOM.
namespace xml_offsets {
    // xmlNode
    static constexpr size_t NODE_TYPE       = 0x08;  // xmlElementType
    static constexpr size_t NODE_NAME       = 0x10;  // const xmlChar*
    static constexpr size_t NODE_CHILDREN   = 0x18;  // xmlNode*
    static constexpr size_t NODE_LAST       = 0x20;  // xmlNode*
    static constexpr size_t NODE_PARENT     = 0x28;  // xmlNode*
    static constexpr size_t NODE_NEXT       = 0x30;  // xmlNode*
    static constexpr size_t NODE_PREV       = 0x38;  // xmlNode*
    static constexpr size_t NODE_DOC        = 0x40;  // xmlDoc*
    static constexpr size_t NODE_CONTENT    = 0x50;  // xmlChar*
    static constexpr size_t NODE_PROPERTIES = 0x58;  // xmlAttr*

    // xmlDoc
    static constexpr size_t DOC_TYPE        = 0x08;  // xmlElementType (should be 9)
    static constexpr size_t DOC_CHILDREN    = 0x18;  // xmlNode* (root element)
    static constexpr size_t DOC_DOC         = 0x40;  // xmlDoc* (self-pointer)

    // xmlAttr
    static constexpr size_t ATTR_TYPE       = 0x08;  // xmlElementType (should be 2)
    static constexpr size_t ATTR_NAME       = 0x10;  // const xmlChar*
    static constexpr size_t ATTR_CHILDREN   = 0x18;  // xmlNode* (text node with value)
    static constexpr size_t ATTR_PARENT     = 0x28;  // xmlNode* (owning element)
    static constexpr size_t ATTR_NEXT       = 0x30;  // xmlAttr*
    static constexpr size_t ATTR_DOC        = 0x40;  // xmlDoc*

    // Minimum struct sizes (for allocation)
    static constexpr size_t SIZEOF_NODE     = 0x78;
    static constexpr size_t SIZEOF_ATTR     = 0x60;
    static constexpr size_t SIZEOF_DOC      = 0x90;  // conservative
}

// Verify the layout by inspecting a real xmlDoc returned from the game's parser.
// Returns true if all offsets match. Logs details to extension log.
bool verify_libxml2_layout(void* xmlDocPtr);
