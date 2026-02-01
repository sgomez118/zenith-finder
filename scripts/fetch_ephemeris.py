"""
Fetches the JPL Ephemeris file (DE405) for NOVAS.
"""

import os
import urllib.request
import shutil
import ssl

# The JPLEPH file is a binary JPL ephemeris file required by NOVAS for high-precision 
# planetary positions.
# We try to fetch the Linux binary (little-endian, compatible with Windows usually)
# from the JPL SSD site.
_URLS = [
    "https://ssd.jpl.nasa.gov/ftp/eph/planets/Linux/de405/unxp2000.405",
    "ftp://ssd.jpl.nasa.gov/pub/eph/planets/Linux/de405/unxp2000.405",
    "https://raw.githubusercontent.com/Smithsonian/SuperNOVAS/v1.3.0/tests/data/JPLEPH",
]
_OUTPUT_FILE = "JPLEPH"


def fetch_ephemeris():
  """
  Downloads the binary ephemeris file.
  """
  if os.path.exists(_OUTPUT_FILE):
    print(f"{_OUTPUT_FILE} already exists.")
    return

  # Create an unverified SSL context to bypass certificate errors
  ssl_context = ssl._create_unverified_context()

  for url in _URLS:
    print(f"Attempting download from {url}...")
    try:
      req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
      # Pass the context to urlopen
      with urllib.request.urlopen(req, context=ssl_context) as response:
        content = response.read()
        if len(content) < 1000: # Sanity check for small error pages
             print(f"Downloaded file too small from {url}, skipping.")
             continue
        with open(_OUTPUT_FILE, 'wb') as f:
          f.write(content)
      print(f"Download successful from {url}.")
      return
    except Exception as e:
      print(f"Failed to download from {url}: {e}")
  
  print("All download attempts failed.")


if __name__ == "__main__":
  fetch_ephemeris()
