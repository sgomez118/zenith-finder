"""
Fetches the JPL Ephemeris file (DE405) for NOVAS.
"""

import urllib.request
import sys
import os

# DE405 is a good balance. 
# URL from a reliable mirror (e.g. PyEphem or similar, or direct from JPL FTP converted to binary)
# NOVAS expects a binary ephemeris file.
# The standard 'lnxp1600.405' is big.
# 'de405.bin' is what we often need.

# For now, let's try to find a small one or just a standard one.
# https://github.com/brandon-rhodes/python-jplephem offers ASCII to binary tools.
# But downloading a pre-built binary is easier.

# Smithsonian/SuperNOVAS repository might have one for testing?
# https://github.com/Smithsonian/SuperNOVAS/tree/master/tests/data
# 'JPLEPH' is there!

URL = "https://raw.githubusercontent.com/Smithsonian/SuperNOVAS/main/tests/data/JPLEPH"
OUTPUT_FILE = "JPLEPH"

def fetch_ephemeris():
    if os.path.exists(OUTPUT_FILE):
        print(f"{OUTPUT_FILE} already exists.")
        return

    print(f"Downloading ephemeris from {URL}...")
    try:
        req = urllib.request.Request(URL, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            with open(OUTPUT_FILE, 'wb') as f:
                f.write(response.read())
        print("Download successful.")
    except Exception as e:
        print(f"Failed to download: {e}")

if __name__ == "__main__":
    fetch_ephemeris()
