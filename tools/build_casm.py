import json, os

with open("src/importers/sff1/sff1_types.h") as f: types_h = f.read()
with open("src/importers/sff1/sff1_reader.h") as f: reader_h = f.read()
with open("docs/importers/casm-structure-notes.md") as f: casm_doc = f.read()

# Extract just the enums section
enum_start = types_h.find("enum class SffSectionType")
enum_end = types_h.find("struct SffChunk")
enum_section = types_h[enum_start:enum_end]

prompt = "You are implementing CASM semantic parsing for an existing C++ SFF parser.\n\n"
prompt += "CASM structure (confirmed from real Yamaha Genos files):\n"
prompt += "- Starts with 4-byte CSEG marker\n"
prompt += "- Contains sub-chunks: Sdec (section def), Ctb2 (track config blocks)\n"
prompt += "- Each Ctb2 entry = 47 bytes: track name(20) + config(27)\n"
prompt += "- Config byte0-1=note_range, byte2=NTR, byte5=NTT\n"
prompt += "- NTR: 0=Root,1=Fifth,2=Chord,3=Bass,6=Fixed\n"
prompt += "- NTT: 0=Bypass,1=Default,2=Guitar,4=Bass,6=Perc,7=SFF2\n\n"
prompt += "EXISTING sff1_reader.h:\n" + reader_h + "\n\n"
prompt += "EXISTING types enums:\n" + enum_section + "\n\n"
prompt += "IMPLEMENT:\n"
prompt += "1. sff1_types.h: add CasmTrackConfig struct (NtrType, NttType enums, high_key, low_key, is_megavoice) to namespace\n"
prompt += "2. sff1_types.h ParseResult: add std::vector<CasmTrackConfig> casm_configs\n"
prompt += "3. sff1_reader.h: add private methods parseCasm, parseCtb2Block\n"
prompt += "4. sff1_reader.cpp parseBuffer: after CASM chunk, call parseCasm(chunk.data)\n"
prompt += "5. parseCasm: validate CSEG marker, iterate sub-chunks, call parseCtb2Block for Ctb2\n"
prompt += "6. parseCtb2Block: parse 47-byte entries, extract NTR/NTT/note_range\n\n"
prompt += "Output ONLY new code additions. Mark with ----- FILE: filename ----- lines.\n"
prompt += "EXTEND, do NOT rewrite. Keep existing methods unchanged."

payload = {"model": "claude-sonnet-4-20250514", "max_tokens": 6000, "messages": [{"role": "user", "content": prompt}]}
with open("/tmp/casm_prompt3.json", "w") as f:
    json.dump(payload, f)
print("Done: prompt is", len(prompt), "chars")
