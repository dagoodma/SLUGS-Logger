"""
This script will copy a log file to a new file, removing headers and footers along the way.
Run in command line: "python3 dataExtract.py [log file] [desination file]"
Example: "python3 dataExtract.py 0001.txt Log21Aug.txt"
"""

import sys

def checkSum(data):
    """Calculates the XOR checksum of the input."""
    cs = 0
    for n in data:
        cs = cs ^ n
    return cs

# Process input arguments
if len(sys.argv) < 3 :
    print("This script requires 2 arguments: a logger file to extract from and a file to dump extracted data. WARNING: The second file will be overwritten!")
    exit()

# Set some constants describing the file format
HEADER_TAG_LEN = 2
HEADER_LEN = HEADER_TAG_LEN + 1
FOOTER_TAG_LEN = 2
FOOTER_LEN = FOOTER_TAG_LEN + 1
FILE_NUMBER_POS = 2
CHECKSUM_POS = -3

# Open files
with open(sys.argv[1], "rb") as slog, open(sys.argv[2], "wb") as outlog:

    # Main loop, reading file by sectors and processing each of them.
    nextSector = slog.read(512)
    fileNumber = nextSector[FILE_NUMBER_POS]
    sectorNumber = 0
    while nextSector:
        # Verify header & footer tags
        if nextSector[:HEADER_TAG_LEN] != b'%^':
            print("ERROR: Header tag failed at sector {}, exiting.".format(sectorNumber))
            sys.exit()
        if nextSector[-FOOTER_TAG_LEN:] != b'%$':
            print("ERROR: Footer tag failed at sector {}, exiting.".format(sectorNumber))
            sys.exit()

        # Verify header number
        if nextSector[FILE_NUMBER_POS] != fileNumber:
            print("WARNING: File number mismatch at sector {}. This usually happens when files don't end on a 128kiB boundary. Please verify that all data is intact if possible.".format(sectorNumber))
            sys.exit()

        # Verify footer checksum
        if nextSector[CHECKSUM_POS] != checkSum(nextSector[HEADER_LEN:-FOOTER_LEN]):
            print("ERROR: Checksum failed at sector {}, exiting.".format(sectorNumber))
            sys.exit()

        # Output the actual sector data and prepare for next iteration
        outlog.write(nextSector[HEADER_LEN:-FOOTER_LEN])
        sectorNumber += 1
        nextSector = slog.read(512)

    print("Successfully decoded {}, comprising {} bytes.".format(sys.argv[2], (sectorNumber + 1) * 506))
