
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
	def get_api_version(cls):
		try:
			return int(cls.get_line_item("api_version"), 10)
		except:
			return 0

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
		api = cls.get_api_version()
		item = 'card_id' if api > 0 else 'module_id'
		result = ((int(m.split('.')[1],10),m) for m in os.listdir(cls.path) if m.startswith(item))
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

class Unipi1Id(UnipiId):
	path = "/etc/unipi-id/"


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
		product_info.vars.update({"unipi_platform": "{:04x}".format(product_id)})
		return product_info

	# try fallback method via product_name for legacy eeprom
	product_name = UnipiId.get_line_item("product_model")
	product_info = lib.unipi_product_info_by_name(product_name)
	if not product_info:
		warning("Unknown product %s %04x" % (product_name, product_id))
	return product_info


def get_mainboard_info():
	api = UnipiId.get_api_version()
	item = 'mainboard_id' if api > 0 else 'baseboard_id'
	board_id = UnipiId.get_hex_item(item)
	board_info = lib.unipi_board_info(board_id)
	if not board_info and (is_valid_id(board_id)):
		warning("Unknown board %04x" % (board_id,))
	return board_info


def merge_dict(dest, source, filter_str=None):
	for k,v in source.items():
		if (filter is not None) and (isinstance(v, list)) and (len(v) > 1):
			specific = list(filter(lambda x: filter_str in x, v))
			if len(specific) == 1:
				v = specific[0]
		try:
			dest[k].append(v)
		except KeyError:
			dest[k]= [v]

def print_recursive(data):
        outstr = ""
        if isinstance(data, str):
                outstr += (data + " ")
        elif isinstance(data, list):
                for item in data:
                        outstr += print_recursive(item)
        return outstr

def main_overlays():
	result = {}
	product_info = get_product_info()
	if product_info:
		merge_dict(result, product_info.vars)

	board_info = get_mainboard_info()
	if board_info:
		merge_dict(result, board_info.vars)

	for slot, card_id in UnipiId.slot_ids():
		card_info = lib.unipi_board_info(card_id, slot)
		if not card_info and (is_valid_id(card_id)):
			warning("Unknown card %04x in slot %d" % (card_id, slot))
		if card_info:
			merge_dict(result, card_info.vars,  product_info.vars.get("unipi_platform"))

	if product_info.vars.get('use_etc_modules', '0') == '1':
		for slot, card_id in Unipi1Id.slot_ids():
			card_info = lib.unipi_board_info(card_id, slot)
			if not card_info and (is_valid_id(card_id)):
				warning("Unknown module %04x in position %d" % (card_id, slot))
			if card_info:
				merge_dict(result, card_info.vars,  product_info.vars.get("unipi_platform"))

	return result


if __name__ == "__main__":
	import argparse
	a = argparse.ArgumentParser(prog='os-configurator.py')
	a.add_argument('-u','--update', help='run action to modify os', action='store_const', const=True, default=False )
	a.add_argument('-f','--force', help='run action to modify os by ignoring errors', action='store_const', const=True, default=False )
	#a.add_argument('description', metavar="file", help='input yaml description', type=str, nargs=1)
	args = a.parse_args()
	env = None
	try:
		env = main_overlays()
		if args.update:
			os.environ.update(**{k.upper(): print_recursive(v).strip() for k,v in env.items()})
			os.execlpe("run-parts", "run-parts", "--verbose",
				"--regex=.sh$", "--exit-on-error",
				"/opt/unipi/os-configurator/run.d",
				os.environ)
		else:
			for k,v in env.items():
				print("{}='{}'".format(k.upper(), print_recursive(v).strip()))

		if not args.force:
			sys.exit(0)

	except FileNotFoundError as E:
		print("Missing unipi-id module or bad id eprom.\n")
	except ValueError as E:
		print("Bad ID value in unipi-id eprom.\n", str(E))

	if args.force:
		if env: os.environ.update(**{k.upper(): print_recursive(v).strip() for k,v in env.items()})
		os.execlpe("run-parts", "run-parts", "--verbose",
			"--regex=.sh$",
			"/opt/unipi/os-configurator/run.d",
			os.environ)

	sys.exit(1)
