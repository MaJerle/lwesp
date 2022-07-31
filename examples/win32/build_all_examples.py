import os
import re

# Get presets from the path
# Use cmake command line to list actual presets visible to cmake
def get_presets():
	presets = []
	resp = os.popen("cmake --list-presets").read().strip()
	for line in resp.split("\n"):
		l = line.strip()
		r = re.findall("\"(.*)\"", l)
		if r:
			presets.append(r[0])
	return presets

# Main execution
if __name__ == '__main__':	
	# Get all presets
	failed = []
	presets = get_presets()
	for preset in presets:
		print("-------------------------------")
		print("Configuring preset " + preset)
		print("-------------------------------")
		ret = os.system("cmake --preset " + preset)
		if ret != 0:
			print("!!!! Command failed !!!! with result code: " + str(ret))
			failed.append(preset)
		print("Return: " + str(ret))
		print("-------------------------------")
		print("Building preset " + preset)
		print("-------------------------------")
		ret = os.system("cmake --build --preset " + preset)
		if ret != 0:
			print("!!!! Command failed !!!! with result code: " + str(ret))
			failed.append(preset)
		print("Return: " + str(ret))
		print("-------------------------------")
	print("Failed presets:")
	for p in failed:
		print(p)
	print("-------------------------------")