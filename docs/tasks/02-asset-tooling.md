# 02. Asset tooling — Gemini generator + PNG→ZX converter

**Status**: DONE
**Depends on**: —
**Blocks**: 03

## Goal

Host-side Python tooling that (a) generates art via Gemini "Nano Banana" and (b) converts PNGs into ZX formats:
a 6912-byte `.scr` loading screen and `const` C byte arrays for UDGs/sprites.

## Context

See `docs/PROJECT.md` → Graphics pipeline. Two scripts in `tools/`, run on the host (not in Docker). The API
key lives in `.env` as `GEMINI_API_KEY` (gitignored) and must never be printed. To keep this task verifiable
without spending API calls, the converter is validated against a locally synthesized test PNG (Pillow), and the
generator supports a `--dry-run` that validates config and lists planned outputs without calling the API.

## Acceptance criteria

- [ ] `tools/gen_assets.py`: reads `GEMINI_API_KEY` from `.env`; iterates prompt files in `assets/prompts/*.txt`;
      writes `assets/png/<name>.png` via `google-genai` (model `gemini-2.5-flash-image`); skips already-generated
      files; never prints the key. A `--dry-run` flag lists planned outputs and validates config without any API
      call.
- [ ] `tools/png2zx.py`: a 256×192 PNG → `assets/scr/loading.scr` exactly 6912 bytes (6144 bitmap + 768 attr)
      with ZX-legal per-cell attributes (≤2 colours/cell + bright); sprite/tile PNGs → `src/assets_gfx.{c,h}` as
      `const` byte arrays, file marked "generated — do not edit by hand".
- [ ] `requirements.txt` lists `google-genai` and `Pillow`; `.gitignore` ignores `.env` and `assets/png/`.
- [ ] Verified locally without API: feeding a Pillow-synthesized 256×192 test image to `png2zx.py` yields a
      `loading.scr` of exactly 6912 bytes and a syntactically valid `src/assets_gfx.h`.

## Test plan

```
skip: host-side tooling, no on-target runtime behaviour. Verified by running png2zx.py on a synthesized 256×192 test PNG → loading.scr is exactly 6912 bytes and assets_gfx.h is valid C; gen_assets.py --dry-run exits 0 without an API call.
```

## Out of scope

- Final art content (produced in art-consuming tasks 03/04/08/09).
- `gemini-3-pro-image-preview` (Nano Banana Pro) upgrade.

## Completion note

Implemented 2026-06-23. Host-side Python tooling complete: `gen_assets.py` with Gemini integration (google-genai, `gemini-2.5-flash-image`, `--dry-run` support, API key never printed) and `png2zx.py` with verified ZX interleave formula for 6912-byte `.scr` files and 2-colour-per-cell quantization. Test outputs (synthesized banner and loading screen) are throwaway and will be regenerated with real art in tasks 03/04.
