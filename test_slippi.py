import slippi

def test_copy_data():
	bytes_copied = slippi.slippi('test-game.slp')

	print(f"Length of file {bytes_copied}".format(bytes_copied))
