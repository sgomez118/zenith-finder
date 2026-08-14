#!/usr/bin/env python3
"""
Download planetary ephemeris files (JPL SPICE BSP format) from NASA JPL
for high-precision celestial tracking in Zenith Finder.
"""

import argparse
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

JPL_BASE_URL = "https://ssd.jpl.nasa.gov/ftp/eph/planets/bsp/"
DEFAULT_EPHEMERIS = "de442.bsp"

AVAILABLE_EPHEMERIDES = {
    "de442": "de442.bsp",
    "de442.bsp": "de442.bsp",
    "de440": "de440.bsp",
    "de440.bsp": "de440.bsp",
    "de440s": "de440s.bsp",
    "de440s.bsp": "de440s.bsp",
    "de421": "de421.bsp",
    "de421.bsp": "de421.bsp",
    "de405": "de405.bsp",
    "de405.bsp": "de405.bsp",
}


def format_bytes(num_bytes: float) -> str:
    """Format bytes to human readable format."""
    for unit in ["B", "KB", "MB", "GB"]:
        if abs(num_bytes) < 1024.0:
            return f"{num_bytes:.1f} {unit}"
        num_bytes /= 1024.0
    return f"{num_bytes:.1f} TB"


def download_file(url: str, output_path: Path, chunk_size: int = 64 * 1024) -> None:
    """Download a file with a live progress bar and speed indicator."""
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "ZenithFinder/0.3.0 (Ephemeris Fetcher)"},
    )

    print(f"Connecting to: {url}")
    with urllib.request.urlopen(req, timeout=60) as response:
        total_size = response.headers.get("Content-Length")
        total_size = int(total_size) if total_size else None

        output_path.parent.mkdir(parents=True, exist_ok=True)
        downloaded = 0
        start_time = time.time()
        last_update = 0.0

        temp_path = output_path.with_suffix(output_path.suffix + ".part")

        with open(temp_path, "wb") as f:
            while True:
                chunk = response.read(chunk_size)
                if not chunk:
                    break
                f.write(chunk)
                downloaded += len(chunk)

                current_time = time.time()
                if current_time - last_update > 0.1:
                    last_update = current_time
                    elapsed = max(current_time - start_time, 0.001)
                    speed = downloaded / elapsed

                    if total_size:
                        percent = (downloaded / total_size) * 100.0
                        progress_bar_len = 30
                        filled = int(progress_bar_len * downloaded / total_size)
                        bar = "=" * filled + "-" * (progress_bar_len - filled)
                        sys.stdout.write(
                            f"\r[{bar}] {percent:5.1f}% | {format_bytes(downloaded)} / "
                            f"{format_bytes(total_size)} | {format_bytes(speed)}/s   "
                        )
                    else:
                        sys.stdout.write(
                            f"\rDownloaded: {format_bytes(downloaded)} | {format_bytes(speed)}/s   "
                        )
                    sys.stdout.flush()

        sys.stdout.write("\n")
        sys.stdout.flush()

        # Rename temp file to final destination
        temp_path.replace(output_path)

    elapsed_total = time.time() - start_time
    avg_speed = downloaded / max(elapsed_total, 0.001)
    print(
        f"Download complete: {output_path.resolve()} "
        f"({format_bytes(downloaded)} in {elapsed_total:.1f}s, avg {format_bytes(avg_speed)}/s)"
    )


def main():
    script_dir = Path(__file__).resolve().parent
    default_out_dir = script_dir.parent

    parser = argparse.ArgumentParser(
        description="Download JPL binary planetary ephemeris files for Zenith Finder."
    )
    parser.add_argument(
        "--ephemeris",
        "-e",
        default=DEFAULT_EPHEMERIS,
        help=(
            f"Ephemeris file name to download (default: {DEFAULT_EPHEMERIS}). "
            f"Supported: {', '.join(sorted(set(AVAILABLE_EPHEMERIDES.values())))}"
        ),
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        default=None,
        help="Output destination path (default: project root/<ephemeris_name>)",
    )
    parser.add_argument(
        "--force",
        "-f",
        action="store_true",
        help="Overwrite existing file if already downloaded",
    )
    parser.add_argument(
        "--url",
        default=None,
        help="Custom URL to download from (overrides standard JPL URL)",
    )

    args = parser.parse_args()

    eph_name = AVAILABLE_EPHEMERIDES.get(args.ephemeris.lower(), args.ephemeris)
    if not eph_name.endswith(".bsp"):
        eph_name += ".bsp"

    out_path = args.output if args.output else default_out_dir / eph_name
    download_url = args.url if args.url else f"{JPL_BASE_URL}{eph_name}"

    if out_path.exists() and not args.force:
        print(
            f"File already exists at {out_path.resolve()} ({format_bytes(out_path.stat().st_size)}).\n"
            "Use --force / -f to re-download."
        )
        return

    try:
        download_file(download_url, out_path)
    except urllib.error.HTTPError as e:
        print(f"\nHTTP Error {e.code}: Could not fetch {download_url}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"\nDownload failed: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
