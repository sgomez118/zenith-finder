#!/usr/bin/env python3
"""
Fetch astronomical star catalog data from the SIMBAD TAP service
and save it in JSON format for Zenith Finder.
"""

import argparse
import json
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

SIMBAD_TAP_URL = "https://simbad.cds.unistra.fr/simbad/sim-tap/sync"


def clean_adql_query(sql_text: str, top_count: int | None = None) -> str:
    """Strip C-style block comments and adjust TOP clause if requested."""
    # Remove block comments /* ... */
    cleaned = re.sub(r"/\*.*?\*/", "", sql_text, flags=re.DOTALL).strip()

    if top_count is not None:
        # Replace TOP <number> with the user requested count
        cleaned = re.sub(
            r"(?i)\bSELECT\s+TOP\s+\d+\b",
            f"SELECT TOP {top_count}",
            cleaned,
            count=1,
        )

    return cleaned


def fetch_simbad_stars(
    sql_query: str, output_path: Path, timeout: int = 120
) -> int:
    """Send synchronous ADQL query to SIMBAD TAP and write JSON result."""
    params = {
        "REQUEST": "doQuery",
        "LANG": "ADQL",
        "FORMAT": "json",
        "QUERY": sql_query,
    }

    data = urllib.parse.urlencode(params).encode("utf-8")
    req = urllib.request.Request(
        SIMBAD_TAP_URL,
        data=data,
        headers={"User-Agent": "ZenithFinder/0.3.0 (Star Catalog Fetcher)"},
    )

    print(f"Connecting to SIMBAD TAP service ({SIMBAD_TAP_URL})...")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            content = response.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        print(f"Error: SIMBAD TAP service returned HTTP {e.code}", file=sys.stderr)
        try:
            error_body = e.read().decode("utf-8")
            print(f"Server response:\n{error_body}", file=sys.stderr)
        except Exception:
            pass
        raise
    except urllib.error.URLError as e:
        print(f"Network Error: {e.reason}", file=sys.stderr)
        raise

    try:
        catalog_json = json.loads(content)
    except json.JSONDecodeError as e:
        print(f"Error parsing response JSON: {e}", file=sys.stderr)
        print(f"Response preview:\n{content[:500]}", file=sys.stderr)
        raise

    data_rows = catalog_json.get("data", [])
    star_count = len(data_rows)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(catalog_json, f, indent=2)

    print(f"Successfully fetched {star_count} stars.")
    print(f"Saved catalog to {output_path.resolve()} ({output_path.stat().st_size:,} bytes).")

    if data_rows:
        print("\nTop 5 Brightest Objects:")
        for row in data_rows[:5]:
            main_id = row[0] if len(row) > 0 else "N/A"
            ra = row[1] if len(row) > 1 else 0.0
            dec = row[2] if len(row) > 2 else 0.0
            v_mag = row[11] if len(row) > 11 else "N/A"
            ids = row[14] if len(row) > 14 and row[14] else ""
            common_name = next(
                (item[5:] for item in ids.split("|") if item.startswith("NAME ")),
                main_id,
            )
            print(f"  - {common_name} ({main_id}): V={v_mag}, RA={ra:.4f}°, Dec={dec:.4f}°")

    return star_count


def main():
    script_dir = Path(__file__).resolve().parent
    default_sql = script_dir / "top_5000.sql"
    default_out = script_dir.parent / "stars.json"

    parser = argparse.ArgumentParser(
        description="Fetch star catalog from SIMBAD TAP service for Zenith Finder."
    )
    parser.add_argument(
        "--sql",
        type=Path,
        default=default_sql,
        help=f"Path to SQL query file (default: {default_sql})",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        default=default_out,
        help=f"Output path for JSON catalog (default: {default_out})",
    )
    parser.add_argument(
        "--count",
        "-n",
        type=int,
        default=None,
        help="Override TOP N stars count in the SQL query",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=120,
        help="Request timeout in seconds (default: 120)",
    )

    args = parser.parse_args()

    if not args.sql.is_file():
        print(f"Error: SQL file not found at {args.sql}", file=sys.stderr)
        sys.exit(1)

    with open(args.sql, "r", encoding="utf-8") as f:
        sql_content = f.read()

    query = clean_adql_query(sql_content, top_count=args.count)

    try:
        fetch_simbad_stars(query, args.output, timeout=args.timeout)
    except Exception as e:
        print(f"Failed to generate star catalog: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
