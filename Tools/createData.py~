# fileName n
# writes n sectors to fileName
import sys

if len(sys.argv) < 3 :
    print("takes two arguments: file name, number of sectors")
    exit()
    
bytesPerSector = 512 - 6
file = open(sys.argv[1], "bw")
n = int(sys.argv[2])

for i in range(n) :
    for j in range(bytesPerSector-1) :
        file.write(ord(str(i)[-1]).to_bytes(1,'big'))
    file.write(b'\n')
