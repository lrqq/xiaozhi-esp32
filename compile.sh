idf.py build
 /home/lrqq/.espressif/python_env/idf5.3_py3.8_env/bin/python /home/lrqq/esp/esp-idf-v5.3.2/esp-idf/components/esptool_py/esptool/esptool.py -p /dev/ttyACM0 -b 460800 --before default_reset --after hard_reset --chip esp32s3 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB 0x100000 ./build/xiaozhi.bin
 idf.py -p /dev/ttyACM0 monitor
