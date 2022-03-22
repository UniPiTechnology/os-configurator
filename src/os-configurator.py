
import os, sys

sys.path.append("/opt/unipi/os-configurator")
import unipi_values as lib


def is_valid_id(board_id):
	return not (board_id in (0, 0xffff))

class UnipiId:
	path = "/sys/devices/platform/unipi-id/unipi-id/"

	def __init__(self):
		pass

	@classmethod
	def get_line_item(cls, item):
		with open(cls.path+item, "r") as f:
			return f.readline().strip()

	@classmethod
	def get_item(cls, item):
		with open(cls.path+item, "r") as f:
			return f.read()

	@classmethod
	def get_hex_item(cls, item):
		return int(cls.get_line_item(item), 16)

	@classmethod
	def _slots(cls):
		result = ((int(m.split('.')[1],10),m) for m in os.listdir(cls.path) if m.startswith('module_id.'))
		return sorted(result)

	@classmethod
	def slots(cls):
		return (slot for slot,name in cls._slots())

	@classmethod
	def slot_ids(cls):
		return ((slot, cls.get_hex_item(name)) for slot,name in cls._slots())

	'''
	@classmethod
	def for_each_module_id(cls, callback):
		for m in os.listdir(cls.path):
			if m.startswith('module_id.'):
				slot = int(m.split('.')[1],10)
				module_id = cls.get_hex_item(m)
				if not callback(module_id, slot): break
	'''


def warning(message):
	print("WARNING: %s" % message)


def get_product_info():
	'''
	Read product identification from eeprom
	Find and return info block from library
	Can return None if product is not listed in library
	'''
	product_id = UnipiId.get_hex_item("platform_id")
	# validate product_id in library
	product_info = lib.unipi_product_info(product_id)
	if product_info:
		return product_info

	# try fallback method via product_name for legacy eeprom
	product_name = UnipiId.get_line_item("product_model")
	product_info = lib.unipi_product_info_by_name(product_name)
	if not product_info:
		warning("Unknown product %s %04x" % (product_name, product_id))
	return product_info


def get_baseboard_info():
	board_id = UnipiId.get_hex_item("baseboard_id")
	board_info = lib.unipi_board_info(board_id)
	if not board_info and (is_valid_id(board_id)):
		warning("Unknown board %04x" % (board_id,))
	return board_info


def merge_dict(dest, source):
	for k,v in source.items():
		try:
			dest[k].append(v)
		except KeyError: 
			dest[k]= [v]


def main_overlays():
	result = {}
	product_info = get_product_info()
	if product_info:
		merge_dict(result, product_info.vars)

	board_info = get_baseboard_info()
	if board_info:
		merge_dict(result, board_info.vars)

	for slot, module_id in UnipiId.slot_ids():
		module_info = lib.unipi_board_info(module_id, slot)
		if not module_info and (is_valid_id(module_id)):
			warning("Unknown board %04x in slot %d" % (module_id, slot))
		if module_info:
			merge_dict(result, module_info.vars)

	return result


if __name__ == "__main__":
	try:
		env = main_overlays()
		if False:
			for k,v in env.items():
				print("%s='%s'" % (k.upper(), " ".join(v)))
		else:
			os.environ.update(**{k.upper(): " ".join(v) for k,v in env.items()})
			os.execlpe("run-parts", "run-parts", "--verbose",
				"--regex=.sh$",
				"/opt/unipi/os-configurator/run.d",
				os.environ)
		sys.exit(0)

	except FileNotFoundError as E:
		print("Missing unipi-id module or bad id eprom.\n", str(E))
	except ValueError as E:
		print("Bad ID value in unipi-id eprom.\n", str(E))

	sys.exit(1)
