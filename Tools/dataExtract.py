import sys

# a basic checksum function
def checkSum(data):
    cs = 0
    for n in data:
        cs = cs ^ n
    return cs
    
if len(sys.argv) < 3 :
    print("This script requires 2 arguments: a logger file to extract from and a file to dump extracted data. WARNING: The second file will be overwritten!")
    exit()
    
header_len = 3
num_pos = 2
check_pos = -3
footer_len = 3
    # Open files
slog = open(sys.argv[1], "rb") # open the first given file for binary reading
rawLog = open(sys.argv[2], "wb") # open the second file for binary writing

    # Main loop
nextSector = slog.read(512)
fileNumber = nextSector[num_pos]
sectorNumber = 0
while nextSector:
        #process the sector
    #verify header & footer tags
    if nextSector[:2] != b'%^' :
        print("Header Tag Failed at sector " + str(sectorNumber))
        exit()
    if nextSector[-2:] != b'%$' :
        print("Footer Tag Failed at sector " + str(sectorNumber))
    #verify header number
    if nextSector[num_pos] != fileNumber :
        print("Number Failed at sector " + str(sectorNumber))
        exit()
    #verify footer checksum
    if nextSector[check_pos] != checkSum(nextSector[header_len:-footer_len]):
        print("Checksum Failed at sector " + str(sectorNumber))
        exit()
    rawLog.write(nextSector[header_len:-footer_len])
    sectorNumber += 1
    nextSector = slog.read(512)
print("Success.")
