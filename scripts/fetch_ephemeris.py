"""
Fetches the JPL Ephemeris file (DE405) for NOVAS.
"""

import os
import urllib.request

# The JPLEPH file is a binary JPL ephemeris file required by NOVAS for high-precision 
# planetary positions. This mirror is from the SuperNOVAS test suite.
_URL = "https://raw.githubusercontent.com/Smithsonian/SuperNOVAS/main/tests/data/JPLEPH"
_OUTPUT_FILE = "JPLEPH"


def fetch_ephemeris():
  """
  Downloads the binary ephemeris file from the repository.
  """
  if os.path.exists(_OUTPUT_FILE):
    print(f"{_OUTPUT_FILE} already exists.")
    return

  print(f"Downloading ephemeris from {_URL}...")
  try:
    req = urllib.request.Request(_URL, headers={'User-Agent': 'Mozilla/5.0'})
    with urllib.request.urlopen(req) as response:
      with open(_OUTPUT_FILE, 'wb') as f:
        f.write(response.read())
    print("Download successful.")
  except Exception as e:
    print(f"Failed to download: {e}")


if __name__ == "__main__":
  fetch_ephemeris()
