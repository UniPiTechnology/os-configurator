
Scripts in  /opt/unipi/os-configurator/run.d can be shell or python scripts or binary programs and so on,
but the extension of such program is required to be .sh
Do not modify scripts installed by other packeges, Install rather you own script. Scripts in directory are started
in alphabetical order - use format NN-name.sh for your script, where NN is two digits number.

Examples in this directory shows one shell script used to configure unipi-one-modbus, one python script from Evok package
and simple shell script for one-shot action.

Environment variables delivered by os-configurator to script depends on specific hardware but there are some common ones.

UNIPI_PLATFORM = hex code
UNIPI_PRODUCT_ID = hex code
UNIPI_PRODUCT_NAME = string
UNIPI_PRODUCT_SERIAL = number
CARDS = list of codes
DT = list of device tree overlayes
