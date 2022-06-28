#!/usr/bin/env python3

"""

Copyright 2020 NXP.

This software is owned or controlled by NXP and may only be used
strictly in accordance with the license terms that accompany it. By
expressly accepting such terms or by downloading, installing,
activating and/or otherwise using the software, you are agreeing that
you have read, and that you agree to comply with and are bound by,
such license terms. If you do not agree to be bound by the applicable
license terms, then you may not retain, install, activate or otherwise
use the software.

File
++++
/scripts/nxp/gen_prov_json.py

Brief
+++++
** Generates Provision JSON file for use with provision.py **

.. versionadded:: 0.0


This script takes a JSON file with metadata common to devices of the same
FFS Product family and inserts the provided device serial string to
produce a JSON file suitable for use with the NXP provision.py script.

The serial string can be provided as an argument on the command line or
read directly from the device via two methods. The first method, invoked
with the '-c' or '--cli_read_serial' parameters, uses the NXP cli embedded
in the SDK applications. The second method, invoked with -i or
--invaldi_read_serial parameters, uses the Ivaldi Python module.

The provided device serial string may contain '+' and '/' characters
as the serial string will converted to a hex string prior to use in the
output JSON file.

The input, metadata JSON, file has the default name './metadata.json'
and the output, provision JSON, file has the default name './provision.json'

The content of the metadata and provision JSON files is as follows:

{
    "client_id"     : "<CLIENT_ID>",
    "device_serial" : "<DSN>",
    "device_type"   : "<DT>",
    "dss_pub_key"   : "<PUB_KEY>",
    "ffs_pid"       : "<FFS_PID>"
}

##########
NOTA BENE:
  1. The device serial string must be a valid Base64 string including
     any trailing '=' padding characters

  2. The <PUB_KEY> string must not have any line breaks, but must be one
     continuous string of characters

  3. Use "sudo <path>/gen_prov_json.py ..." on Linux-type systems to avoid
     permission problem when reading serial string directly from device
     with NXP cli

  4. The J27 jumper on the front of the device must be moved to the two
     pins towards the center of the device when reading the serial string
     directly from the device with the Ivaldi Python module.

  5. Avoid "sudo" on Linux-type systems when reading serial string directly
     from device with Ivaldi Python module as the virtual environment for the
     Ivaldi Python module will be activated in the user's rather than root's
     environment.
##########


execute "gen_prod_json.py --help" for usage information.

"""

import sys
import logging
import json
import binascii
import base64


FFS_DSN = "device_serial"


def hex_str_for_ffs_from_base64(inStr):
    """
        Decode existing base64 input to hex for ffs

        :param inStr: base64 input string to format
        :type inStr: String

        :returns: (str) hex encoded string
    """

    try:
        # Decode Base64 and convert to lower-case hex without leading '0x'
        ret = base64.b64decode(inStr, validate=True).hex()
        return ret

    except binascii.Error:
        print("\nERROR: Invalid base64 encoding on input string, '%s'." % inStr)
        print("       Check if trailing padding '=' characters have been removed\n")
        return ''


def check_metadata(meta_dict):
    """
    Check FFS product metadata

    :param meta_dict: dict of metadata loaded from JSON file
    :type  meta_dict: dict

    :returns: (bool) True upon success and False on errors

    """
    # Note device_serial entry not needed in metadata JSON file
    # as it will be added below
    expected_metadata_list = ["client_id", "device_type"
                              , "dss_pub_key", "ffs_pid" ]

    metadata_set = set(iter(meta_dict))
    try:
        metadata_set.remove("device_serial")
    except:
        pass

    error_flag = False
    for meta_item in expected_metadata_list:
        metadata = meta_dict.get(meta_item,None)
        if (metadata is None or len(metadata) == 0
            or (metadata[0] == '<' and metadata[-1] == '>')):

            if not error_flag:
                print("\nMissing input metadata entries:")
            print("    %s" % meta_item)
            error_flag = True

        else:
            metadata_set.remove(meta_item)

    if len(metadata_set) != 0:
        if error_flag: print("")
        print("Extra or malformed entries found in input metadata:")
        error_flag = True

        for extra_item in metadata_set:
            print("    %s : %s" % (extra_item, meta_dict.get(extra_item)))

    return not error_flag


def gen_ffs_json(iot_kit_serial_str, metadata_json_fname, prov_json_fname):

    """
    Generates device json for use with NXP's provision.py script
    Specifically, specializes metadata json with entries common to device instances

    :param iot_kit_serial_str: base64url representation of kit serial number
    :type  iot_kit_serial_str: string

    :param metadata_json_fname: Path of file containing metadata json data
    :type  metadata_json_fname: string

    :param json_fname: Path of provision file for json data
    :type  json_fname: string

    :returns: (bool) True upon success and False on errors

    """

    # Read metadata JSON data with metadata common to devices in
    # this product family
    try:
        with open(metadata_json_fname,'r') as fp:
            metadata_dict = json.load(fp)

    except OSError as e:
        print("\nERROR: Problem accessing metadata (input) JSON file: %s" % e)
        return False

    # except json.decoder.JSONDecodeError as e:
    except Exception as e:
        print("\nERROR: Problem parsing metadata (input) JSON file: %s" % e)
        return False

    # Check loaded metadata
    if not check_metadata(metadata_dict):
        return False

    # Update metadata JSON data with DSN value
    metadata_dict[FFS_DSN] = iot_kit_serial_str

    # Write JSON data for this device
    try:
        with open(prov_json_fname,'w') as fp:
            json.dump(metadata_dict, fp, indent=4, sort_keys=True)

    except OSError as e:
        print("\nERROR: Problem accessing provision (output) JSON file: %s" % e)
        return False

    return True


def ivaldi_read_serial(ivaldi_root):
    """
        Read serial number (unique ID) from device via Ivaldi module

        :param ivaldi_root: path to root of Ivalidi module
        :type  ivaldi_root: string

        :returns: Base64 encoded serial string
    """
    
    flashloader_path = os.path.normpath(ivaldi_root + '/Flashloader/ivt_flashloader.bin')

    ser_str = ''

    sdp = sdphost.SDPHost('0x1fc9', '0x135')

    # Check communication with device
    print()
    print('Establishing connection...')
    if sdp.error_status()['ret']:
        print('\nERROR: Could not establish communication with device.')
        print('       Check that jumper is towards center of device.')
        print('       Power-cycle device for each connection attempt!')
        return ser_str
    else:
        print('SUCCESS: Communication established with device.')

    # Load flashloader onto device
    print('Loading flashloader...')

    if sdp.write_file('0x20000000', flashloader_path)['ret']:
        print('\nERROR: Could not write file!')
        return ser_str
    else:
        print('SUCCESS: Flashloader loaded successfully.')

    # Jump to flash loader entry point
    print('Jumping to flashloader entry point...')
    if sdp.jump_to_address('0x20000400')['ret']:
        print('\nERROR: Could not jump to address!')
        return ser_str
    else:
        print('SUCCESS: Device jumped to execute flashloader.')

    bl = blhost.BLHost()

    # Poll device to make sure it is ready
    print('Waiting for device to be ready for blhost...')
    waitCount = 0
    while bl.get_property('0x01')['ret']:
        time.sleep(0.5)
        waitCount += 1
        if waitCount == 10:
            print('\nERROR: Timeout waiting for device to be ready. Power cycle device and try again.')
            return ser_str

    print('SUCCESS: Device is ready for blhost!')

    print('Reading device unique ID...')
    prop = bl.get_property('0x12')

    if prop['ret']:
        print('\nERROR: Could not read device unique ID!')
        return ser_str
    else:
        ser_str = helpers.encode_unique_id(prop['response'])

    return ser_str


def cli_read_serial(port_str):

    """
        Read serial number (unique ID) from device via cli

        :param port_str: serial port ex "COM<num>" or "/dev/ttyS<num>"
        :type  port_str: string

        :returns: Base64 encoded serial string
    """

    ser_str = ''
    serial_port = None

    try:
        serial_port = serial.Serial(
            port_str,
            baudrate=115200,
            timeout=0.2,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False,
            writeTimeout=2
        )

        sio = io.TextIOWrapper(io.BufferedRWPair(serial_port, serial_port, 1))

        sio.write("serial_number\r\n")
        # Must flush buffer to force output
        sio.flush()
            
        # Consume echo of command
        raw_line = sio.readline()
        # Read command output
        raw_line = sio.readline()

        try:
            ser_str = re.search("^SHELL>> (\S+)$", raw_line).groups(1)[0]
        except Exception as e:
            ser_str = ''

    # serial.SerialException not supported on all PySerial versions
    except Exception as e:
        print("\nERROR: Could not open serial port %s - %s" % (port_str,e))
        print("       Try sudo <path>/gen_prov_json.py if on Linux-type system\n")
        return ser_str

    finally:
        if serial_port: serial_port.close()

    return ser_str


if __name__ == "__main__":

    import argparse

    DEF_META_JSON = "./metadata.json"
    DEF_PROV_JSON = "./provision.json"

    formatter = logging.Formatter(
        '%(filename)s:%(funcName)s@%(lineno)d: %(message)s')
    conHdlr = logging.StreamHandler()
    conHdlr.setFormatter(formatter)

    lgg = logging.getLogger(__name__)
    lgg.addHandler(conHdlr)


    parser = argparse.ArgumentParser()

    parser.add_argument("--lglvl", type=int, default=logging.ERROR
                        , help="logging threshold level (5 - deep debug, 10 - debug)")

    parser.add_argument("--meta_fname", default=DEF_META_JSON
                        , help="path to metadata (input) json file")

    parser.add_argument("--prov_fname", default=DEF_PROV_JSON
                        , help="path to provision (output) json file")

    parser.add_argument("-i", "--ivaldi_read_serial", type=str, default='', dest="ivaldi_root"
                        , help="root path of Ivaldi module and use Ivaldi to read kit/device serial string directly from device")

    parser.add_argument("-c", "--cli_read_serial", type=str, default='', dest="cli_port"
                        , help="port string for device virtual com port and use cli to read kit/device serial string directly from device")

    parser.add_argument("kit_serial", type=str, nargs="?", default=''
                        , help="SLN_ALEXA_IOT kit serial string including trailing '=' characters")


    args = parser.parse_args()

    # argparse exlusive argument groups has problem w/ parameter and argument
    args_present = int(0)
    if args.kit_serial != ''   : args_present += 1
    if args.ivaldi_root != ''  : args_present += 1
    if args.cli_port != ''     : args_present += 1
    
    if args_present > 1:
        print("\nERROR: parameters --ivaldi_read_serial, --cli_read_serial, and argument kit_serial are mutually exclusive\n")
        parser.print_help()
        sys.exit(-1)
    elif args_present < 1:
        print("\nERROR: One of parameters --ivaldi_read_serial, --cli_read_serial, and argument kit_serial is required\n")
        parser.print_help()        
        sys.exit(-1)

    lgg.setLevel(args.lglvl)

    if args.ivaldi_root != '':
        # Only incur Ivaldi dependency when reading serial string
        # directly from device
        try:
            import Ivaldi.blhost as blhost
            import Ivaldi.sdphost as sdphost
            import Ivaldi.helpers as helpers
            import os.path
            import time
        except ModuleNotFoundError as e:
            print("\nERROR: Unable to import modules - %s" % e)
            print("       Ensure not using sudo on Linux-type systems\n")
            sys.exit(-1)

        # Read serial string from device using Ivaldi module
        serial_str = ivaldi_read_serial(args.ivaldi_root)
        if serial_str == '':
            print("\nERROR: Unable to read serial string from device via Ivaldi module\n")
            sys.exit(-1)

    elif args.cli_port != '':
        # Only incur serial and io dependency when reading serial string
        # directly from device
        try:
            import serial
            import io
            import re
        except ModuleNotFoundError as e:
            print("\nERROR: Unable to import modules - %s" % e)
            sys.exit(-1)

        # Read serial string from device using cli    
        serial_str = cli_read_serial(args.cli_port)
        if serial_str == '':
            print("\nERROR: Unable to read serial string from device via cli\n")
            sys.exit(-1)
    else:
        # Use provided serial string
        serial_str = args.kit_serial


    # Replace Base64 encoded serial string with hex string
    ffs_serial = hex_str_for_ffs_from_base64(serial_str)

    if ffs_serial == '':
        sys.exit(-1)

    print()
    if ffs_serial != serial_str:
        print("Using '%s' for DSN modified from input serial '%s'\n"
              % (ffs_serial,serial_str))

    if not gen_ffs_json(ffs_serial, args.meta_fname, args.prov_fname):
        print()
        sys.exit(-1)

    print("SUCCESS, JSON data for provision.py is in file: '%s'\n"
          % args.prov_fname)

    sys.exit(0)
