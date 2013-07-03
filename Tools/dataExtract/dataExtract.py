slog = open(argv[1], "rb") # open the first given file in binary mode
nextSector = slog.readLine(limit=512)
print(nextSector)
