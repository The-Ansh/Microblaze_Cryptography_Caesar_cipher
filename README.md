# Microblaze_Cryptography_Caesar_cipher
Contains a custom Verilog module for cryptography and complete Hardware design including the microblaze processor soft IP and a  custom bare-metal c code to execute on the device. Target device is Basys 3 FPGA board with Artix 7 FPGA by AMD-Xilinx.

After the device is sucessfully programmed with the bitstream, Send a file to encrypt/decrypt using the crypto.py python script.
Go to terminal and run command-
python -u crypto.py abc.txt

The file will be processed and stored in the same folder as abc_processed.txt
