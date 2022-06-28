#!/usr/bin/env python3

"""

Copyright 2020 NXP.

This software is owned or controlled by NXP and may only be used strictly in accordance with the
license terms that accompany it. By expressly accepting such terms or by downloading, installing,
activating and/or otherwise using the software, you are agreeing that you have read, and that you
agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
applicable license terms, then you may not retain, install, activate or otherwise use the software.

File
++++
/smart_home_avs_code_generator.py

Brief
+++++
** 
    Script to convert the JSON discovery file to a source object to be used in
    the application
**

.. versionadded:: 0.0

"""

import datetime
import os
import sys
import time
import zipfile
import shutil as sh
import distutils.dir_util as dr
import zipfile
import subprocess
import shutil 
import operator
import json
import discovery_endpoint_gen as Generator

xml_file = ""

error_code = ["No Error", "Failed to read file", "Failed to Validate JSON"]

def print_usage():
    print("Usage: argv[0] JSON Endpoint Discovery File")
    print("       Please supply a JSON file that conforms to the AVS Discovery Interface")
    quit()

def validate_arguments():
    if len(sys.argv) != 2:
        print_usage()

    global xml_file
    xml_file = sys.argv[1]

def read_file(xml_file):
    json_contents = None

    with open(xml_file, 'r') as xml_json_file:
        print(xml_json_file)
        json_contents = json.load(xml_json_file)
    #except:
    #    print(xml_file)
    #    json_contents = None

    return json_contents

def validate_json(json_contents):
    json_object = None

    print(json_contents)
    try:
        json_object = json.loads(json_contents)
    except ValueError as e:
        print("[validate_json] - Error parsing JSON from file provided")

    return json_object

def structure_create_callback():
    print("Callback")

def auto_generate_discovery_interface_files(json_object):
    generator = Generator.DiscoveryEndpointGen()
    generator.execute(json_object, "discovery_endpoint_json", structure_create_callback)


def main():
    status = 0
    json_contents = None

    validate_arguments()
    print(xml_file)
    json_contents = read_file(xml_file)

    print(json_contents)
    if json_contents == None:
        status = 1

    if status == 0:
        status = auto_generate_discovery_interface_files(json_contents)

    if status == 0:
        print("[MAIN] - Failed to generate JSON code for Smart Home Directives")
        print("[MAIN] - Error: &s", error_code[status])

if __name__== "__main__":
    main()