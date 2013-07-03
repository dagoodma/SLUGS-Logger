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
    
    # Open files
slog = open(sys.argv[1], "rb") # open the first given file for binary reading
rawLog = open(sys.argv[2], "wb") # open the second file for binary writing

    # Main loop
nextSector = slog.read(512)
fileNumber = nextSector[2]
while nextSector:
        #process the sector
    #verify header & footer tags
    if nextSector[:2] != b'%^' :
        print("Header Tag Failed")
        exit()
    if nextSector[-2:] != b'%$' :
        print("Footer Tag Failed")
        exit()
    #verify header number
    if nextSector[2] != fileNumber :
        print("Number Failed")
        exit()
    #verify footer checksum
    if nextSector[-3] != checkSum(nextSector[3:-3]):
        print("Checksum Failed")
        exit()
    rawLog.write(nextSector[3:-3])
    nextSector = slog.read(512)
print("Success.")
