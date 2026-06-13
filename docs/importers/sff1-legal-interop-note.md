# SFF1 Importer — Legal & Interoperability Note

> **Status:** DRAFT — not legal advice, for discussion only  
> **Date:** 2026-06-13

---

## Scope

This note applies to the SFF1 importer planned for Gate 5.
It does NOT cover:
- Bundling or distributing Yamaha styles
- Reverse-engineering Yamaha firmware
- Using copyrighted Yamaha sample content

## Legal Principles

1. **User-Owned Files Only** — The importer loads files the user already owns. No bundled styles.
2. **No DRM Bypass** — Encrypted/DRM-protected styles are REJECTED, not decrypted.
3. **No ROM Extraction** — No extraction of sample ROM or firmware content.
4. **Clean Room Design** — Importer specification based on publicly available documentation and user-provided files. No reference to proprietary Yamaha source code.
5. **No Commercial Claim** — Marketing must not claim "Yamaha compatibility" without explicit legal review.

## Interoperability

Under applicable copyright law (including but not limited to EU Directive 2019/790 Art. 6 and US DMCA exemptions for interoperability):

- Reading user-owned file formats for interoperability purposes is generally lawful
- Re-implementing file format parsers based on clean-room analysis is standard industry practice
- MegaVoice articulation mapping is functional, not creative

## Risk Register

| Risk | Level | Mitigation |
|------|-------|------------|
| DMCA anti-circumvention claims | Low | No DRM bypass; reject encrypted files |
| Trademark: "Yamaha compatible" | Low | Use "User-loaded style support"; no trademark in marketing |
| Copyright: bundled styles | None | No bundled styles; user-loaded only |
| Patent: MegaVoice mapping | Unknown | Use articulation profile abstraction; no patent-specific implementation |
| YAMAHA IP: reverse engineering claims | Low | Clean-room implementation based on public spec + user files |

## Recommended Next Steps

1. Consult legal counsel before any commercial release
2. Document clean-room process for the importer specification
3. Keep importer code in isolated `src/importers/` layer
4. No Yamaha brand references in runtime or UI
