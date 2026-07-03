import serial
import time
import os
import sys

BAUD_RATE    = 9600     

def main():
    # Check if the user provided the file argument
    if len(sys.argv) < 2:
        print("\n[Error] Missing input file argument!")
        print("Usage: python script_name.py <input_file>")
        print("Example: python script_name.py abc.txt\n")
        return

    # Grab the input file from the terminal argument
    INPUT_FILE = sys.argv[1]

    # Quick sanity check to ensure the source file exists
    if not os.path.exists(INPUT_FILE):
        print(f"[Error] Source file '{INPUT_FILE}' not found in this directory!")
        return

    # Automatically generate the output file name: <input_file>_processed.txt
    file_base, file_ext = os.path.splitext(INPUT_FILE)
    OUTPUT_FILE = f"{file_base}_processed{file_ext}"

    SERIAL_PORT  = "COM7"

    print(f"Opening {SERIAL_PORT} at {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.5)
        time.sleep(2)  # Allow hardware connection to stabilize
    except Exception as e:
        print(f"Failed to open port {SERIAL_PORT}: {e}")
        return

    print(f"Starting encryption stream: {INPUT_FILE} -> {OUTPUT_FILE}\n")

    # Open files for processing
    with open(INPUT_FILE, "r", encoding="utf-8") as infile, \
         open(OUTPUT_FILE, "w", encoding="utf-8") as outfile:

        char_count = 0
        while True:
            # 1. Read exactly one character
            char = infile.read(1)
            if not char:
                break  # End of File reached
            
            # 2. Transmit the character to the FPGA
            ser.write(char.encode('utf-8'))
            char_count += 1
            
            # 3. Wait for the FPGA to process and send a response back
            timeout_start = time.time()
            while ser.in_waiting == 0:
                if time.time() - timeout_start > 1.0: # 1-second timeout guard
                    print(f"\n[Warning] Timeout waiting for response to character #{char_count}")
                    break
                time.sleep(0.001)
            
            # 4. Read the processed character from UART and save it
            if ser.in_waiting > 0:
                response_bytes = ser.read(ser.in_waiting)
                try:
                    response_char = response_bytes.decode('utf-8', errors='ignore')
                    outfile.write(response_char)
                except Exception as e:
                    print(f"\nError decoding received byte: {e}")

    ser.close()
    print("\n=====================================================")
    print(f" SUCCESS: Processed {char_count} characters.")
    print(f" Saved as: '{OUTPUT_FILE}'")
    print("=======================================================")

if __name__ == '__main__':
    main()