#pragma once
// ---------------------------------------------------------------------------
// The save XML node layout — private to the core
// ---------------------------------------------------------------------------
// MSVC's layout of the game's node: a small vector of attributes, each holding a
// std::string, and a small vector of child pointers. Extensions never see it;
// they build and read nodes through x4n::savexml (save_xml_node.h).
// ---------------------------------------------------------------------------

// A save XML node: one element of a save's in-memory DOM, built and read by the
// game's writers and readers. See docs/rev/SAVE_XML.md.
// Verified against X4.exe v9.00 build 900-611726 —
//   BlacklistData_Export (RVA 0x003725E0) allocates each node with GameAlloc(0xF8,
//   ..., 0x10) and constructs it inline: word [+0x02] = -1, dword [+0x04] = -1,
//   qword [+0x08] = 0, qword [+0x10] = 0, qword [+0xB8] = 4, qword [+0xC0] = 0,
//   qword [+0xE8] = 4, qword [+0xF0] = 0, byte [+0x00] = 3, then, when byte [+0x00]
//   has no flag 8, dword [+0x0C] = the element number (the qword store at +0x08
//   covers +0x08..+0x0F, so it clears the element too and has to come before it);
//   it appends the node to the
//   parent's child vector at +0xC8 through XmlNodeVector_PushBack;
//   XmlNode_SetAttribute (RVA 0x00123700) finds or adds the attribute in the
//   vector at +0x18, 0x28-byte entries with the number at +0 and the value at +8.
// A small vector holds 4 entries inline at its start; past that, its start holds
// a pointer to the entries on the heap and its capacity is no longer 4.
#define X4_XML_NODE_SIZE                0xF8        /* bytes per node */
#define X4_XML_NODE_ALIGNMENT           0x10        /* GameAlloc alignment for a node */
#define X4_XML_NODE_OFFSET_FLAGS        0x00        /* uint8 — X4_XML_NODE_FLAG_* */
#define X4_XML_NODE_OFFSET_SOURCE_LINE  0x02        /* uint16 — X4_XML_NODE_NO_SOURCE_LINE on a built node */
#define X4_XML_NODE_OFFSET_SOURCE_COLUMN 0x04       /* int32 — X4_XML_NODE_NO_SOURCE_COLUMN on a built node */
#define X4_XML_NODE_OFFSET_PREFIX       0x08        /* uint32 — 0 on a built node; the element follows at +0x0C */
#define X4_XML_NODE_OFFSET_ELEMENT      0x0C        /* int32 — the element's SaveXML number */
#define X4_XML_NODE_OFFSET_TEXT         0x10        /* a text node's content; 0 on a built element node */
#define X4_XML_NODE_OFFSET_ATTRIBUTES   0x18        /* small vector of X4_XML_ATTRIBUTE_SIZE-byte attributes */
#define X4_XML_NODE_OFFSET_ATTRIBUTE_CAPACITY 0xB8  /* size_t — the attribute vector's capacity */
#define X4_XML_NODE_OFFSET_ATTRIBUTE_COUNT    0xC0  /* size_t — the attribute vector's count */
#define X4_XML_NODE_OFFSET_CHILDREN     0xC8        /* small vector of child node pointers */
#define X4_XML_NODE_OFFSET_CHILD_CAPACITY     0xE8  /* size_t — the child vector's capacity */
#define X4_XML_NODE_OFFSET_CHILD_COUNT        0xF0  /* size_t — the child vector's count */
#define X4_XML_NODE_FLAG_ELEMENT        0x03        /* flags of an element node */
#define X4_XML_NODE_FLAG_TEXT           0x08        /* flag bit of a text node */
#define X4_XML_NODE_NO_SOURCE_LINE      0xFFFF      /* source line of a built node */
#define X4_XML_NODE_NO_SOURCE_COLUMN    (-1)        /* source column of a built node */
#define X4_XML_NODE_INLINE_CAPACITY     4           /* a small vector's capacity while its entries are inline */
#define X4_XML_ATTRIBUTE_SIZE           0x28        /* bytes per attribute */
#define X4_XML_ATTRIBUTE_OFFSET_ID      0x00        /* int32 — the attribute's SaveXML number */
#define X4_XML_ATTRIBUTE_OFFSET_VALUE   0x08        /* std::string — the attribute's value */
