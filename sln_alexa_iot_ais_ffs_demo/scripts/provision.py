#!/usr/bin/python
#
# Copyright 2020 Amazon.com, Inc. or its affiliates. All rights reserved.
#
# AMAZON PROPRIETARY/CONFIDENTIAL
#
# You may not use this file except in compliance with the terms and conditions
# set forth in the accompanying LICENSE.TXT file. This file is a
# Modifiable File, as defined in the accompanying LICENSE.TXT file.
#
# THESE MATERIALS ARE PROVIDED ON AN "AS IS" BASIS. AMAZON SPECIFICALLY
# DISCLAIMS, WITH RESPECT TO THESE MATERIALS, ALL WARRANTIES, EXPRESS,
# IMPLIED, OR STATUTORY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
#

#
# Copyright 2020 NXP.
# This software is owned or controlled by NXP and may only be used strictly in accordance with the
# license terms that accompany it. By expressly accepting such terms or by downloading, installing,
# activating and/or otherwise using the software, you are agreeing that you have read, and that you
# agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
# applicable license terms, then you may not retain, install, activate or otherwise use the software.
#

from __future__ import print_function
import argparse
import random
import serial
import time
import re
import sys
import json
import time
from datetime import datetime
from multiprocessing.pool import ThreadPool
import textwrap

################################################################################
INPUT_PORT = 'comport'
INPUT_COMMAND = 'command'
INPUT_DEVICE_INFO_CONFIG = 'device_info_config'
INPUT_SPEED = 'speed'
INPUT_DELAY_MS = 'delay_ms'
INPUT_CERT_FILE = 'cert_file'
INPUT_JSON_FILE = 'json_file'
DEFAULT_BT_DEVICE_NAME = 'NXP_REF_DEV'
DEFAULT_FFS_PIN = '123ABC456'

PARAM_LENGTH = 32

def convert_ms_to_sec(ms):
    return ms / 1000.0

################################################################################


class DeviceCli:

    def __init__(self, serial_port, delay_ms):
        self.serial_port = serial_port
        self.delay_sec = convert_ms_to_sec(delay_ms)

    def execute_command(self, command, timeout=1):
        pool = ThreadPool(processes=1)
        result = pool.apply_async(self.__get_execution_status, args=(timeout,))
        self.write_to_serial(command.encode(), self.delay_sec)
        res = result.get()
        pool.close()
        pool.join()
        print("[" + str(res) + "] " + command[:-2] + "!")
        return res

    def execute_set_command(self, final_command, param, timeout=1):
        written_param_size = 0
        param_size = 0
        for line in param.splitlines():
            param_size += len(line) + 1
        param_size = param_size - 1

        command = 'ffs_provision param_begin ' + str(param_size) + '\r\n'
        res = self.execute_command(command, timeout)
        if not res:
            print("")
            return False

        for line in param.splitlines():
            chunks = textwrap.wrap(line, PARAM_LENGTH)
            for chunk in chunks:
                command = 'ffs_provision param_chunk \"' + chunk + '\"\r\n'
                res = self.execute_command(command, timeout)
                if not res:
                    print("")
                    return False
                written_param_size += len(chunk)

            if written_param_size < param_size:
                res = self.execute_command('ffs_provision param_newline\r\n', timeout)
                if not res:
                    print("")
                    return False
                written_param_size += 1

        res = self.execute_command(final_command, timeout)
        print("")
        return res

    def set_device_info(self, device_serial, device_type, ffs_pid, dss_pub_key, client_id):
        set_device_info_completed = True
        timeout = 1

        res = self.execute_command('ffs_provision start\r\n', timeout)
        if not res:
            set_device_info_completed = False

        if res:
            res = self.execute_set_command('ffs_provision device_info_set dsn\r\n', device_serial)
            if not res:
                set_device_info_completed = False

            res = self.execute_set_command('ffs_provision device_info_set type\r\n', device_type)
            if not res:
                set_device_info_completed = False

            res = self.execute_set_command('ffs_provision device_info_set ffs_pid\r\n', ffs_pid)
            if not res:
                set_device_info_completed = False

            res = self.execute_set_command('ffs_provision device_info_set ffs_pin\r\n', DEFAULT_FFS_PIN)
            if not res:
                set_device_info_completed = False

            res = self.execute_set_command('ffs_provision device_info_set bt_name\r\n', DEFAULT_BT_DEVICE_NAME)
            if not res:
                set_device_info_completed = False

            res = self.execute_set_command('ffs_provision device_info_set dss_pub_key\r\n', dss_pub_key)
            if not res:
                set_device_info_completed = False

            res = self.execute_set_command('ffs_provision device_info_set client_id\r\n', client_id)
            if not res:
                set_device_info_completed = False

            res = self.execute_command('ffs_provision stop\r\n', timeout)
            if not res:
                set_device_info_completed = False

        return set_device_info_completed

    def set_cert_from_file(self, cert_file):
        cert_pem = get_from_file(cert_file)
        self.set_cert(cert_pem)

    def write_to_serial(self, command, delay_sec):
        if delay_sec == 0.0:
            self.serial_port.write(command)
        else:
            # some devices can't be write to at normal speed and needs a delay
            # for these insert a delay between chars
            for c in command:
                self.serial_port.write(c.encode())
                time.sleep(delay_sec)

    def set_cert(self, cert):
        print('\r\n>>>>>>>>>>>>>>> Setting Certificate Chain Start')
        cert_size = 0
        cert_lines = cert.splitlines()
        for line in cert_lines:
            cert_size += len(line) + 1
        cert_size -= 1

        res = self.execute_command('ffs_provision start\r\n', 1)
        if res:
            res = self.execute_set_command('ffs_provision dha_cert_set\r\n', cert, 10)
            if res:
                print('>>>>>>>>>>>>>>> Setting Certificate Chain Succeed\r\n')
            else:
                print('>>>>>>>>>>>>>>> Setting Certificate Chain Failed <<<<<<<<<<<<<<<\r\n')

            self.execute_command('ffs_provision stop\r\n', 1)
            return res

    def init_keygen(self):
        command = 'ffs_provision dha_keygen\r\n'
        return self.execute_command(command, 3)

    def get_csr(self):
        res = self.execute_command('ffs_provision start\r\n', 1)
        csr = ''
        if res:
            if self.init_keygen():
                pool = ThreadPool(processes=1)
                result = pool.apply_async(self.__get_csr, args=(3, ))
                command = 'ffs_provision dha_get_field csr\r\n'
                self.write_to_serial(command.encode(), self.delay_sec)
                res, csr = result.get()
                pool.close()
                pool.join()

                print("[" + str(res) + "] " + command[:-2] + "\r\n" + csr)
            else:
                res = False
                csr = ''

            self.execute_command('ffs_provision stop\r\n', 1)

        return res, csr

    def get_cert(self):
        pool = ThreadPool(processes=1)
        result = pool.apply_async(self.__get_cert, ())
        command = 'ffs_provision dha_get_field crt_chain\r\n'
        self.write_to_serial(command, self.delay_sec)
        cert = result.get(timeout=2)
        pool.close()
        pool.join()
        return cert

    def __get_csr(self, timeout):
        start = time.time()
        csr = ''

        while True:
            response = self.serial_port.readline().lstrip().decode()
            if "-----BEGIN CERTIFICATE REQUEST-----" in str(response):
                csr += "-----BEGIN CERTIFICATE REQUEST-----\n"
                break
            elif "ffs_provision command success" in response:
                return False, ''
            elif "ffs_provision command fail" in response:
                return False, ''
            if time.time() - start > timeout:
                print("[ERROR] Read result TIMEOUT")
                return False, ''

        while True:
            response = self.serial_port.readline().lstrip().decode()
            csr += response
            if response.startswith("-----END CERTIFICATE REQUEST-----"):
                break
            elif "ffs_provision command success" in response:
                return False, ''
            elif "ffs_provision command fail" in response:
                return False, ''
            if time.time() - start > timeout:
                print("[ERROR] Read result TIMEOUT")
                return False, ''

        result = self.__get_execution_status(timeout - (time.time() - start))
        return result, csr

    def __get_cert(self):
        cert = ''
        while True:
            response = self.serial_port.readline().lstrip()
            if response.startswith('-----BEGIN CERTIFICATE-----'):
                cert += response
                break

        while True:
            response = self.serial_port.readline().lstrip()
            cert += response
            if response.startswith('-----END CERTIFICATE-----'):
                break

        return cert

    def __get_all_responses(self):
        response = self.serial_port.readline().lstrip()
        while response != '':
            sys.stdout.write(response)
            response = self.serial_port.readline().lstrip()

    def __get_execution_status(self, timeout):
        start = time.time()
        while True:
            response = self.serial_port.readline()
            #if str(response) != '':
            #    print("!" + str(response) + "!")
            if "ffs_provision command success" in str(response):
                # print("Command: success")
                return True
            if "ffs_provision command fail" in str(response):
                # print("Command: fail")
                return False
            if "Command not recognized." in str(response):
                return False

            if time.time() - start > timeout:
                print("[ERROR] Read result TIMEOUT")
                return False


################################################################################

class DeviceProvisioner(DeviceCli):

    def provision_device_info(self, device_serial, device_type, ffs_pid, dss_pub_key, client_id):
        print('\r\n>>>>>>>>>>>>>>> Provisioning Device Info Start')
        set_device_info_completed = self.set_device_info(device_serial, device_type, ffs_pid, dss_pub_key, client_id)
        if set_device_info_completed:
            print('>>>>>>>>>>>>>>> Provisioning Device Info Succeed\r\n')
        else:
            print('>>>>>>>>>>>>>>> Provisioning Device Info Failed <<<<<<<<<<<<<<<\r\n')

    def save_csr(self):
        print('\r\n>>>>>>>>>>>>>>> Generating CSR Start')
        res, csr = self.get_csr()
        if res:
            file_name = 'csr.pem'
            self.__write_to_file(file_name, csr)
            print('>>>>>>>>>>>>>>> Generating CSR Succeed: (' + file_name + ' created)\r\n')
        else:
            print('>>>>>>>>>>>>>>> Generating CSR Failed <<<<<<<<<<<<<<<\r\n')

    def save_cert(self):
        cert = self.get_cert()
        file_name = 'cert.pem'
        self.__write_to_file(file_name, cert)
        print(file_name + " created")
        return

    def __write_to_file(self, file_name, data):
        with open(file_name, 'w') as write_file:
            write_file.write(data)

################################################################################


def parse_inputs():
    kwargs = []

    try:
        parser = argparse.ArgumentParser(
            formatter_class=argparse.RawTextHelpFormatter,
            epilog='\n================================================================================\n',
            usage='''provision.py [-h] -p <COM_PORT> -c COMMAND [-f CERT_FILE]
                    [-x DEVICE_INFO_CONFIG]
                    \r\n================================================================================
                     \r=============== Please follow below steps to provision a device ================
                     \r================================================================================
                   \r\n  1. Create a developer account (if you do not already have one) on developer.amazon.com.
                     \r  2. Go to https://developer.amazon.com/acs-devices/console.
                     \r  3. Cilck on the "Manage>Device Certificates" link on the sidebar.
                     \r  4. Click "Request new DSN button". (You can have up to 5 DSNs per device type)
                     \r  5. Click "Get New Certificate" to the right of the DSN you want a certificate for.
                     \r  6. Download the Device Data JSON file and put it at the same directory as this script.
                     \r  7. In your terminal, run ./provision.py -p <COM-port> -c provision -x <JSON-filename>.
                     \r\n     For example, ./provision.py -p /dev/ttyUSB0 -c provision -x device-info
                     \r\n  8. Then, run ./provision.py -p <COM-port> -c get_csr. A csr.pem will be generated.
                     \r\n     For example, ./provision.py -p /dev/ttyUSB0 -c get_csr
                     \r\n  9. Copy and paste the CSR into the text box in the "Get New Certificate" window.
                     \r     and click "Submit". Wait a few seconds. The button should update to "Download Certificate".
                     \r 10. Put the downloaded Certificate at the same directory as this script.
                     \r 11. In your terminal, run ./provision.py <COM-port> -c save_cert -f <Certificate-filename>
                     \r\n     For example, ./provision.py -p /dev/ttyUSB0 -c save_cert -f XYZ.pem
                     \r\n================================================================================''')
        parser.add_argument(
            '-p', '--port',
            dest=INPUT_PORT,
            required=True,
            type=str,
            help='COM port that ESP32 device is connected to')
        parser.add_argument(
            '-c',
            '--command',
            dest=INPUT_COMMAND,
            required=True,
            type=str,
            help='''Commands for this script:
            python provision.py -p <comport> -c <provision | get_csr | save_cert> -x <device_info_config_file_path>
            -c option meanings:
            provision: provision the device with the device info config file
            get_csr: use to get the csr from the device to sign it in the portal then use save_cert
            save_cert: use with fileName of the signed certificate from the csr from get_csr
            get_device_info: get the device info and print it out
            ''')
        parser.add_argument(
            '-f',
            '--cert_file',
            dest=INPUT_CERT_FILE,
            required=False,
            type=str,
            help='The file of the signed certificate to load to device.')
        parser.add_argument(
            '-x',
            '--device_info_config',
            dest=INPUT_DEVICE_INFO_CONFIG,
            required=False,
            type=str,
            help='The file path of device_info config.')
        parser.add_argument(
            '-s',
            '--speed',
            dest=INPUT_SPEED,
            required=False,
            type=str,
            default='normal',
            choices=['normal', 'slow'],
            help='Speed to run script.  Default is %(default)s.  '
            'If device is dropping bytes or part of commands send from PC, use -s slow. to insert a delay.')
        parser.add_argument(
            '-dms',
            '--delay_ms',
            dest=INPUT_DELAY_MS,
            required=False,
            type=int,
            default=10,
            choices=range(1, 1000),
            metavar='int(1, 1000)',
            help='Millisecond delay to insert if --speed is set to slow.  Default is %(default)s ms.')
        kwargs = vars(parser.parse_args())
        # Remove None values
        kwargs = {k: v for k, v in kwargs.items() if v}
    except Exception as e:
        print(str(e))

    return kwargs

################################################################################


def get_from_file(file_name):
    with open(file_name) as read_file:
        content = read_file.readlines()
        return ''.join(content)

################################################################################


def get_device_info_config(config_file):
    print('\r\n>>>>>>>>>>>>>>> Parsing Device Info Start')
    try:
        with open(config_file) as f:
            device_info_config = json.load(f)
            try:
                print('device_serial = ' + device_info_config["device_serial"])
            except Exception:
                print('Invalid device_serial')
                print('>>>>>>>>>>>>>>> Parsing Device Info Failed <<<<<<<<<<<<<<<\r\n')
                return None
            try:
                print('device_type   = ' + device_info_config["device_type"])
            except Exception:
                print('Invalid device_type')
                print('>>>>>>>>>>>>>>> Parsing Device Info Failed <<<<<<<<<<<<<<<\r\n')
                return None
            try:
                print('ffs_pid       = ' + device_info_config["ffs_pid"])
            except Exception:
                print('Invalid ffs_pidn')
                print('>>>>>>>>>>>>>>> Parsing Device Info Failed <<<<<<<<<<<<<<<\r\n')
                return None
            try:
                print('dss_pub_key   = ' + device_info_config["dss_pub_key"])
            except Exception:
                print('Invalid dss_pub_key')
                print('>>>>>>>>>>>>>>> Parsing Device Info Failed <<<<<<<<<<<<<<<\r\n')
                return None
            try:
                print('client_id     = ' + device_info_config["client_id"])
            except Exception:
                print('Invalid client_id')
                print('>>>>>>>>>>>>>>> Parsing Device Info Failed <<<<<<<<<<<<<<<\r\n')
                return None
            print('>>>>>>>>>>>>>>> Parsing Device Info Succeed\r\n')
            return device_info_config

    except Exception:
        print('Invalid device info config: ' + config_file)
        print('Please pass the correct one in with -x')
        print('>>>>>>>>>>>>>>> Parsing Device Info Failed <<<<<<<<<<<<<<<\r\n')
        return None

################################################################################


def provision_device_info(provisioner, device_serial, device_type, ffs_pid, dss_pub_key, client_id):
    provisioner.provision_device_info(device_serial, device_type, ffs_pid, dss_pub_key, client_id)


def write_device_csr_from_file(provisioner, file_name):
    provisioner.set_cert_from_file(file_name)


def save_device_csr(provisioner):
    provisioner.save_csr()


def save_device_cert(provisioner):
    provisioner.save_cert()

################################################################################


################################################################################

def execute(serial_port, kwargs):
    try:
        command = kwargs[INPUT_COMMAND]
        speed = kwargs[INPUT_SPEED]
        delay_ms = 0.0
        if speed == 'slow':
            delay_ms = kwargs[INPUT_DELAY_MS]
        provisioner = DeviceProvisioner(serial_port, delay_ms)

        if command == 'provision':
            if INPUT_DEVICE_INFO_CONFIG in kwargs:
                device_info_config = get_device_info_config(kwargs[INPUT_DEVICE_INFO_CONFIG])
                if device_info_config is not None:
                    device_serial = device_info_config["device_serial"]
                    device_type = device_info_config["device_type"]
                    ffs_pid = device_info_config["ffs_pid"]
                    dss_pub_key = device_info_config["dss_pub_key"]
                    client_id = device_info_config["client_id"]

                    provision_device_info(provisioner, device_serial, device_type, ffs_pid, dss_pub_key, client_id)
                else:
                    print("[ERROR] Bad provisioning *.json file")
            else:
                print("[ERROR] Missing provisioning *.json file")
        elif command == 'get_csr':
            save_device_csr(provisioner)
        elif command == 'save_cert':
            if INPUT_CERT_FILE in kwargs:
                file_name = kwargs[INPUT_CERT_FILE]
                write_device_csr_from_file(provisioner, file_name)
            else:
                print("[ERROR] Missing file of signed cert please pass in with -f")
        else:
            print("[ERROR] Invalid command: " + command)

    except Exception as e:
        print("[ERROR] The process failed with the following exception: " + str(e))

################################################################################


def main():
    serial_port = 0
    kwargs = parse_inputs()
    try:
        serial_port = serial.Serial(
            port=kwargs[INPUT_PORT],
            baudrate=115200,
            timeout=0.2,
            xonxoff=False,
            rtscts=False,
            dsrdtr=False,
            writeTimeout=2
        )
        execute(serial_port, kwargs)

    except Exception as e:
        print(type(e))
        print(str(e))
    finally:
        if serial_port:
            serial_port.close()

################################################################################
# don't execute if imported


if __name__ == '__main__':
    main()
