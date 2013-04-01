import string

def patternConfirm(pattern, file):
	chunkCounter = 0
	while True:
		chunk = file.read(len(pattern))
		if chunk == "": # The end of the file
			print("All good.")
			return
		elif chink != pattern:
			print("Not good. Chink " + str(chunkCounter))
			print(chunk)
			return
		chunkCounter += 1

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

patternConfirm(pattern, f)