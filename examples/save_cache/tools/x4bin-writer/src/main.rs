//! x4bin-writer -- Convert X4 savegame .xml.gz to .x4bin binary DOM cache
//!
//! Usage:
//!   x4bin-writer <save_file.xml.gz>
//!   x4bin-writer <save_directory>        # converts all .xml.gz
//!
//! The .x4bin is written alongside the .xml.gz. The DLL picks it up on load.

use flate2::read::GzDecoder;
use quick_xml::events::Event;
use quick_xml::Reader;
use sha2::{Digest, Sha256};
use std::collections::HashMap;
use std::fs;
use std::io::{self, Read};
use std::path::{Path, PathBuf};
use std::time::Instant;

// --- Binary format (must match binary_format.h) ---

const X4BN_MAGIC: u32 = 0x4E42_3458;
const X4BN_VERSION: u32 = 1;
const IDX_NONE: u32 = 0xFFFF_FFFF;
const HEADER_SIZE: usize = 56;

// --- String table ---

struct StringTable {
    map: HashMap<String, u32>,
    buf: Vec<u8>,
}

impl StringTable {
    fn new() -> Self {
        Self {
            map: HashMap::with_capacity(100_000),
            buf: Vec::with_capacity(4 << 20),
        }
    }

    fn intern(&mut self, s: &str) -> u32 {
        if let Some(&off) = self.map.get(s) {
            return off;
        }
        let off = self.buf.len() as u32;
        self.map.insert(s.to_owned(), off);
        self.buf.extend_from_slice(s.as_bytes());
        self.buf.push(0);
        off
    }
}

// --- Tree records ---

struct NodeRec {
    name_off: u32,
    first_child: u32,
    next_sib: u32,
    first_attr: u32,
    child_count: u16,
}

struct AttrRec {
    name_off: u32,
    val_off: u32,
    next_attr: u32,
}

// --- SAX-based tree builder ---

fn build_tree(xml: &[u8], strs: &mut StringTable) -> (Vec<NodeRec>, Vec<AttrRec>) {
    let mut nodes: Vec<NodeRec> = Vec::with_capacity(500_000);
    let mut attrs: Vec<AttrRec> = Vec::with_capacity(2_500_000);
    let mut stack: Vec<u32> = Vec::with_capacity(256);
    let mut prev_at: Vec<u32> = vec![IDX_NONE; 256];

    let mut reader = Reader::from_reader(xml);
    reader.config_mut().trim_text(true);
    let mut buf = Vec::with_capacity(8192);

    loop {
        match reader.read_event_into(&mut buf) {
            Ok(Event::Start(ref e)) => {
                let idx = push_element(e, &mut nodes, &mut attrs, strs, &stack, &mut prev_at);
                stack.push(idx);
                let d = stack.len();
                if d < prev_at.len() {
                    prev_at[d] = IDX_NONE;
                }
            }
            Ok(Event::Empty(ref e)) => {
                push_element(e, &mut nodes, &mut attrs, strs, &stack, &mut prev_at);
            }
            Ok(Event::End(_)) => {
                stack.pop();
            }
            Ok(Event::Eof) => break,
            Err(e) => {
                eprintln!("  XML error at pos {}: {:?}", reader.buffer_position(), e);
                break;
            }
            _ => {}
        }
        buf.clear();
    }

    (nodes, attrs)
}

fn push_element(
    e: &quick_xml::events::BytesStart<'_>,
    nodes: &mut Vec<NodeRec>,
    attrs: &mut Vec<AttrRec>,
    strs: &mut StringTable,
    stack: &[u32],
    prev_at: &mut Vec<u32>,
) -> u32 {
    let qname = e.name();
    let tag = std::str::from_utf8(qname.as_ref()).unwrap_or("?");
    let name_off = strs.intern(tag);
    let idx = nodes.len() as u32;
    let depth = stack.len();

    // Attributes
    let attr_list: Vec<_> = e.attributes().filter_map(|a| a.ok()).collect();
    let first_attr = if attr_list.is_empty() {
        IDX_NONE
    } else {
        let first = attrs.len() as u32;
        for (i, a) in attr_list.iter().enumerate() {
            let k = std::str::from_utf8(a.key.as_ref()).unwrap_or("?");
            let v = a.unescape_value().unwrap_or_default();
            attrs.push(AttrRec {
                name_off: strs.intern(k),
                val_off: strs.intern(&v),
                next_attr: if i + 1 < attr_list.len() {
                    attrs.len() as u32 + 1
                } else {
                    IDX_NONE
                },
            });
        }
        first
    };

    // Link as child of parent
    if let Some(&parent) = stack.last() {
        let p = &mut nodes[parent as usize];
        if p.first_child == IDX_NONE {
            p.first_child = idx;
        }
        p.child_count += 1;
    }

    // Link as next sibling
    if depth >= prev_at.len() {
        prev_at.resize(depth + 1, IDX_NONE);
    }
    if prev_at[depth] != IDX_NONE {
        nodes[prev_at[depth] as usize].next_sib = idx;
    }
    prev_at[depth] = idx;

    nodes.push(NodeRec {
        name_off,
        first_child: IDX_NONE,
        next_sib: IDX_NONE,
        first_attr,
        child_count: 0,
    });

    idx
}

// --- Writer ---

fn write_x4bin(gz_path: &Path) -> io::Result<PathBuf> {
    let out_path = gz_path.with_extension("").with_extension("x4bin");
    let t0 = Instant::now();

    // Hash
    let gz_bytes = fs::read(gz_path)?;
    let hash = u64::from_le_bytes(Sha256::digest(&gz_bytes)[..8].try_into().unwrap());

    // Decompress
    eprint!("  Decompressing...");
    let mut xml = Vec::with_capacity(gz_bytes.len() * 10);
    GzDecoder::new(&gz_bytes[..]).read_to_end(&mut xml)?;
    let t1 = t0.elapsed();
    eprintln!(" {:.1} MB in {:.1}s", xml.len() as f64 / 1e6, t1.as_secs_f64());
    drop(gz_bytes);

    // Parse
    eprint!("  Parsing...");
    let mut strs = StringTable::new();
    let (nodes, attrs) = build_tree(&xml, &mut strs);
    let t2 = t0.elapsed();
    eprintln!(
        " {} nodes, {} attrs, {} strings in {:.1}s",
        nodes.len(),
        attrs.len(),
        strs.map.len(),
        (t2 - t1).as_secs_f64()
    );
    drop(xml);

    // Pack
    eprint!("  Writing...");
    let str_off = HEADER_SIZE as u32;
    let node_off = str_off + strs.buf.len() as u32;
    let attr_off = node_off + nodes.len() as u32 * 20;
    let total = attr_off as usize + attrs.len() * 12;

    let mut out = Vec::with_capacity(total);

    // Header
    out.extend_from_slice(&X4BN_MAGIC.to_le_bytes());
    out.extend_from_slice(&X4BN_VERSION.to_le_bytes());
    out.extend_from_slice(&hash.to_le_bytes());
    out.extend_from_slice(&(nodes.len() as u32).to_le_bytes());
    out.extend_from_slice(&(attrs.len() as u32).to_le_bytes());
    out.extend_from_slice(&str_off.to_le_bytes());
    out.extend_from_slice(&(strs.buf.len() as u32).to_le_bytes());
    out.extend_from_slice(&node_off.to_le_bytes());
    out.extend_from_slice(&attr_off.to_le_bytes());
    out.extend_from_slice(&[0u8; 16]);
    debug_assert_eq!(out.len(), HEADER_SIZE);

    // String table
    out.extend_from_slice(&strs.buf);

    // Nodes (20 bytes each)
    for n in &nodes {
        out.extend_from_slice(&n.name_off.to_le_bytes());
        out.extend_from_slice(&n.first_child.to_le_bytes());
        out.extend_from_slice(&n.next_sib.to_le_bytes());
        out.extend_from_slice(&n.first_attr.to_le_bytes());
        out.extend_from_slice(&n.child_count.to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
    }

    // Attrs (12 bytes each)
    for a in &attrs {
        out.extend_from_slice(&a.name_off.to_le_bytes());
        out.extend_from_slice(&a.val_off.to_le_bytes());
        out.extend_from_slice(&a.next_attr.to_le_bytes());
    }

    let tmp = out_path.with_extension("x4bin.tmp");
    fs::write(&tmp, &out)?;
    fs::rename(&tmp, &out_path)?;

    let t3 = t0.elapsed();
    eprintln!(
        " {:.1} MB\n  Total: {:.1}s -> {}",
        out.len() as f64 / 1e6,
        t3.as_secs_f64(),
        out_path.display()
    );

    Ok(out_path)
}

// --- Main ---

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: x4bin-writer <save_file.xml.gz | save_directory>");
        std::process::exit(1);
    }

    let target = Path::new(&args[1]);

    if target.is_dir() {
        let mut files: Vec<PathBuf> = fs::read_dir(target)
            .expect("cannot read dir")
            .filter_map(|e| e.ok())
            .map(|e| e.path())
            .filter(|p| p.to_string_lossy().ends_with(".xml.gz"))
            .collect();
        files.sort();

        if files.is_empty() {
            eprintln!("No .xml.gz files in {}", target.display());
            std::process::exit(1);
        }

        eprintln!("Found {} saves\n", files.len());
        for f in &files {
            let cache = f.with_extension("").with_extension("x4bin");
            if cache.exists() {
                if let (Ok(gm), Ok(cm)) = (f.metadata(), cache.metadata()) {
                    if let (Ok(gt), Ok(ct)) = (gm.modified(), cm.modified()) {
                        if ct > gt {
                            eprintln!(
                                "Skipping {} (up to date)",
                                f.file_name().unwrap().to_string_lossy()
                            );
                            continue;
                        }
                    }
                }
            }
            eprintln!("Converting: {}", f.file_name().unwrap().to_string_lossy());
            if let Err(e) = write_x4bin(f) {
                eprintln!("  ERROR: {e}");
            }
            eprintln!();
        }
    } else if target.exists() {
        eprintln!("Converting: {}", target.file_name().unwrap().to_string_lossy());
        if let Err(e) = write_x4bin(target) {
            eprintln!("ERROR: {e}");
            std::process::exit(1);
        }
    } else {
        eprintln!("Not found: {}", target.display());
        std::process::exit(1);
    }
}
