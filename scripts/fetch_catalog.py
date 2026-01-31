"""
Fetches the HYG star catalog or falls back to a built-in list.

This script attempts to download the HYG database CSV from various mirrors.
If successful, it filters for stars visible to the naked eye (magnitude <= 5.0)
and writes them to 'stars.csv'. If all downloads fail, it writes a small
fallback catalog.
"""

import csv
import gzip
import io
import sys
import urllib.request

# URLs to try for the HYG database
_URLS = [
    "https://raw.githubusercontent.com/johanley/star-catalog/master/hygdata_v3.csv",
    "https://raw.githubusercontent.com/astronexus/HYG-Database/main/hygdata_v3.csv.gz",
    "https://raw.githubusercontent.com/astronexus/HYG-Database/master/hygdata_v3.csv.gz",
    "https://raw.githubusercontent.com/astronexus/HYG-Database/main/hygdata_v3.csv",
    "https://raw.githubusercontent.com/astronexus/HYG-Database/master/hygdata_v3.csv",
]

_OUTPUT_FILE = "stars.csv"

_FALLBACK_STARS = """Name,RA (deg),Dec (deg)
Sirius,101.28708,-16.716111
Canopus,95.98791,-52.695833
Arcturus,213.91542,19.179722
Rigil Kentaurus,219.89958,-60.835278
Vega,279.23458,38.783611
Capella,79.1725,45.998056
Rigel,78.63458,-8.201667
Procyon,114.82542,5.225
Achernar,24.42875,-57.236667
Betelgeuse,88.79292,7.406944
Hadar,210.95583,-60.373056
Altair,297.70833,8.868333
Aldebaran,68.98,-16.509167
Antares,247.35167,-26.431944
Spica,201.29833,-11.164167
Pollux,116.32875,28.026111
Fomalhaut,344.41292,-29.622222
Mimosa,191.97167,-59.688611
Deneb,310.35792,45.280278
Acrux,186.64958,-63.099167
Regulus,152.09292,11.967222
Adhara,104.65625,-28.972222
Gacrux,187.79125,-57.113333
Shaula,263.40208,-37.103889
Bellatrix,81.28292,6.349722
El Nath,81.57292,28.6075
Miaplacidus,138.3,-69.717222
Alnilam,84.05333,-1.201944
Al Na'ir,332.05833,-46.961111
Alioth,193.50708,55.959722
Alnitak,85.18958,-1.942778
Mirfak,51.08083,49.628056
Wezen,108.625,-26.833333
Kaus Australis,274.09167,-34.375
Alkaid,206.24167,49.316667
Sargas,259.08333,-42.916667
Castor,113.64833,31.888889
Gienah,185.25,-17.533333
Adara,104.65625,-28.972222
Peacock,304.77083,-56.716667
Alhhena,109.95833,16.408333
Dubhe,165.03333,61.75
Polaris,37.95,89.266667
Mirzam,100.0,-17.95
Alpheratz,2.133333,29.091667
"""


def get_name(row):
    """
    Extracts the common name of a star from a CSV row.

    Args:
        row: A dictionary representing a row from the HYG database.

    Returns:
        The best available name (Proper, Bayer, Gliese, HR, HIP, HD) or 'Unknown'.
    """
    if row.get('proper'):
        return row['proper']
    if row.get('bf'):
        return row['bf']
    if row.get('gl'):
        return row['gl']
    if row.get('hr'):
        return f"HR {row['hr']}"
    if row.get('hip'):
        return f"HIP {row['hip']}"
    if row.get('hd'):
        return f"HD {row['hd']}"
    return "Unknown"


def fetch_and_process():
    """
    Main function to fetch, process, and save the star catalog.
    """
    data = None
    for url in _URLS:
        print(f"Downloading catalog from {url}...")
        try:
            req = urllib.request.Request(
                url, headers={'User-Agent': 'Mozilla/5.0'})
            response = urllib.request.urlopen(req)
            content = response.read()

            # Check for HTML (common with soft 404s or redirects)
            if b"<!doctype html>" in content.lower() or b"<html" in content.lower():
                print("Received HTML instead of data. Skipping.")
                continue

            # Decompress if gzip
            if url.endswith('.gz'):
                try:
                    data = gzip.decompress(content).decode('utf-8')
                except OSError:
                    data = content.decode('utf-8')
            else:
                data = content.decode('utf-8')

            print("Download successful.")
            break
        except Exception as e:
            print(f"Failed to download from {url}: {e}")

    if not data:
        print("Could not download catalog from any source. Using fallback list.")
        data = None  # Proceed to fallback

    if data:
        print("Processing downloaded data...")
        reader = csv.DictReader(io.StringIO(data))

        stars = []
        count = 0

        for row in reader:
            try:
                mag = float(row.get('mag', 0))
                if mag <= 5.0 or row.get('proper'):
                    name = get_name(row)
                    # Convert hours to degrees if needed (HYG uses hours)
                    ra = float(row['ra']) * 15.0
                    dec = float(row['dec'])

                    stars.append((name, ra, dec, mag))
                    count += 1
            except ValueError:
                continue

        stars.sort(key=lambda x: x[3])
        print(f"Found {count} stars.")

        with open(_OUTPUT_FILE, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['name', 'ra', 'dec'])
            for star in stars:
                writer.writerow([star[0], f"{star[1]:.6f}", f"{star[2]:.6f}"])

    else:
        print("Writing fallback catalog...")
        with open(_OUTPUT_FILE, 'w', newline='', encoding='utf-8') as f:
            f.write("name,ra,dec\n")  # Ensure correct header for app
            # Parse the fallback CSV string which has headers Name,RA (deg),Dec (deg)
            # Skip header
            lines = _FALLBACK_STARS.strip().split('\n')[1:]
            for line in lines:
                f.write(line + '\n')
        print(f"Wrote {len(lines)} stars from fallback list.")

    print(f"Successfully wrote {_OUTPUT_FILE}")


if __name__ == "__main__":
    fetch_and_process()