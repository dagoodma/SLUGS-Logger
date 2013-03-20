import string
f = open("/Volumes/Nice/newfile.txt", "r")
pattern = """Abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
abcdefghijklmnopqrstuvwxZ
"""


print("Length: " + str(len(pattern)))
print("Pattern")
print(pattern)

chunkCounter = 0
while True:
	chunk = f.read(len(pattern))
	if(chunkCounter == 0):
		chunkCounter += 1
		continue
	if chunk == "":
		print("All good.")
		break
	elif chunk != pattern:
		print("Not good. Chunk " + str(chunkCounter))
		print(chunk)
		break
	chunkCounter += 1
