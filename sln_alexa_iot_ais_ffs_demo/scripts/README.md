## Provisioning of the board 

Works on:
	1. Windows cmd.exe (tested with python3.6.8)
	2. WSL (Windows Subsystem for Linux) (tested with python2.7.15 and python3.6.9)

1. Install dependencies:
(cmd.exe)(WSL)
	python -m easy_install --upgrade pyOpenSSL
	python -m pip install pyserial

2. Connect the RT106A board to your PC. 

3. Export that serial interface:
(cmd.exe) The COM number it receives will be available in Device Manager (for example: COM6).
	cd scripts
	SET DEVICE_SERIAL_NUMBER=COM6

(WSL) The COM number it receives will be available in WSL as /dev/ttyS<number> (for example: /dev/ttyS6 for COM6).
	cd scripts
	export DEVICE_SERIAL_NUMBER=/dev/ttyS6

4. Create new digital Device on Amazon platform.
Navigate to the ACS portal registration website at this link: https://developer.amazon.com/acs-devices/console
Using this page, you will be able to get the data values you need to register your device. Once you have logged in, navigate to the Manage Device Certificates page: https://developer.amazon.com/acs-devices/console/manage?
Click on the Request New DSN button to pop open the device type submenu, and then select your board type. You should now see a new row that contains the name of your product, a Device Serial Number, and an orange button labeled Get New Certificate. Click on the Get New Certificate button, and you should see a Get New Certificate pop-up that contains three steps. Click on the Download button underneath the first step to receive a JSON file containing the DSN, DT, RPI_PUB_KEY, and CLIENT_ID.
Move this JSON file to where your provisioning script (provision.py) is: scripts/device_provision.

5. Set device_info fields.
(cmd.exe)
	python provision.py -p %DEVICE_SERIAL_NUMBER% -c provision -x [json_file_name]

(WSL)
	python provision.py -p $DEVICE_SERIAL_NUMBER -c provision -x [json_file_name]

6. Make board to generate a CSR (Certificate Signing Request). The below command extracts the CSR from the board and saves it to a file called "csr.pem" in the "scripts/device_provision" folder.
(cmd.exe)
	python provision.py -p %DEVICE_SERIAL_NUMBER% -c get_csr 

(WSL)
	python provision.py -p $DEVICE_SERIAL_NUMBER -c get_csr 

7. Copy the text from the "csr.pem" file and paste it into the text field under step 3 of the Get New Certificate pop-up on the ACS portal website, and click submit. After a couple of seconds, the orange Get New Certificate button should change to a green Download Certificate button; click on this button to download your signed certificate. Move this signed certificate to where your provisioning script is located (scripts/device_provision folder).

8. Validate your signed certificate, and if the certificate is valid, then the program will output Device Registered and will gracefully terminate. Replace [signed_certificate_name] with name of the downloaded signed certificate file in the previous step.
(cmd.exe)
	python provision.py -p %DEVICE_SERIAL_NUMBER% -c save_cert -f [signed_certificate_name]
	
(WSL)
	python provision.py -p $DEVICE_SERIAL_NUMBER -c save_cert -f [signed_certificate_name]
