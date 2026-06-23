#!/usr/bin/env python3
"""
gen_assets.py -- Generate PNG assets via Gemini image generation.

Reads GEMINI_API_KEY from .env (via python-dotenv).
Iterates assets/prompts/*.txt; for each <name>.txt with no existing
assets/png/<name>.png, generates the image and saves it.

--dry-run: validate config (key present, prompts dir exists), list planned
           outputs, NO API call, exit 0.
           On missing key: print "GEMINI_API_KEY not set in .env", exit 1.
"""

import argparse
import os
import sys
from io import BytesIO
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MODEL = "gemini-2.5-flash-image"


def load_env() -> None:
    """Load .env from repo root using python-dotenv (silent if file absent)."""
    try:
        from dotenv import load_dotenv
        load_dotenv(REPO_ROOT / ".env")
    except ImportError:
        pass  # python-dotenv not installed; rely on environment as-is


def get_api_key() -> str | None:
    """Return GEMINI_API_KEY from environment, or None if not set."""
    key = os.environ.get("GEMINI_API_KEY")
    if not key or not key.strip():
        return None
    return key.strip()


def list_prompts() -> list[Path]:
    """Return sorted list of .txt files in assets/prompts/."""
    prompts_dir = REPO_ROOT / "assets" / "prompts"
    if not prompts_dir.is_dir():
        return []
    return sorted(prompts_dir.glob("*.txt"))


def planned_outputs(prompt_files: list[Path]) -> list[tuple[Path, Path]]:
    """
    Return [(prompt_path, png_path), ...] for prompts that have no existing PNG.
    """
    png_dir = REPO_ROOT / "assets" / "png"
    result = []
    for p in prompt_files:
        out = png_dir / (p.stem + ".png")
        if not out.exists():
            result.append((p, out))
    return result


def generate_image(client, model: str, prompt_text: str) -> bytes:
    """
    Call Gemini image generation and return raw PNG bytes.
    Raises RuntimeError if no image part is found in the response.
    """
    response = client.models.generate_content(
        model=model,
        contents=[prompt_text],
    )
    for part in response.candidates[0].content.parts:
        if part.inline_data:
            return part.inline_data.data
    raise RuntimeError("Gemini response contained no inline_data image part.")


def cmd_dry_run(prompt_files: list[Path], api_key: str | None) -> int:
    if api_key is None:
        print("GEMINI_API_KEY not set in .env", file=sys.stderr)
        return 1

    outputs = planned_outputs(prompt_files)

    print(f"Model   : {DEFAULT_MODEL}")
    print(f"Prompts : {len(prompt_files)} found")
    print(f"Pending : {len(outputs)} to generate")
    for prompt_path, png_path in outputs:
        print(f"  {prompt_path.name} -> {png_path.relative_to(REPO_ROOT)}")

    skipped = len(prompt_files) - len(outputs)
    if skipped:
        print(f"Skipped : {skipped} (PNG already exists)")

    print("Dry-run complete. No API calls made.")
    return 0


def cmd_generate(prompt_files: list[Path], api_key: str) -> int:
    try:
        from google import genai
        from PIL import Image
    except ImportError as exc:
        print(f"ERROR: missing dependency: {exc}. Run: pip install -r requirements.txt", file=sys.stderr)
        return 1

    outputs = planned_outputs(prompt_files)
    if not outputs:
        print("Nothing to generate (all PNGs already exist).")
        return 0

    client = genai.Client(api_key=api_key)

    png_dir = REPO_ROOT / "assets" / "png"
    png_dir.mkdir(parents=True, exist_ok=True)

    errors = 0
    for prompt_path, png_path in outputs:
        prompt_text = prompt_path.read_text(encoding="utf-8").strip()
        print(f"Generating {png_path.name} from {prompt_path.name} ...", end=" ", flush=True)
        try:
            raw_png = generate_image(client, DEFAULT_MODEL, prompt_text)
            img = Image.open(BytesIO(raw_png))
            img.save(png_path)
            print(f"OK ({img.width}x{img.height})")
        except Exception as exc:
            print(f"FAILED: {exc}", file=sys.stderr)
            errors += 1

    return 0 if errors == 0 else 1


def main() -> int:
    load_env()

    parser = argparse.ArgumentParser(
        description="Generate PNG assets for ZX Spectrum sprites via Gemini."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate config and list planned outputs; make no API calls.",
    )
    args = parser.parse_args()

    prompt_files = list_prompts()

    if args.dry_run:
        api_key = get_api_key()
        return cmd_dry_run(prompt_files, api_key)

    api_key = get_api_key()
    if api_key is None:
        print("GEMINI_API_KEY not set in .env", file=sys.stderr)
        return 1

    return cmd_generate(prompt_files, api_key)


if __name__ == "__main__":
    sys.exit(main())
