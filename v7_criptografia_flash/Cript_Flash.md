# Carregar Aplicação Criptografada

Para apagar a flash do dispositivo (recomendado rodar na primeira vez):  

``` bat
python %IDF_PATH%\components\esptool_py\esptool\esptool.py --chip esp32 erase_flash
```

Para criptografar o bootloader, tabela de partições e aplicação (após a build):  

``` bat
python %IDF_PATH%\components\esptool_py\esptool\espsecure.py encrypt_flash_data --keyfile key.bin --address 0x1000 -o enc/bootloader.bin build/bootloader/bootloader.bin

python %IDF_PATH%\components\esptool_py\esptool\espsecure.py encrypt_flash_data --keyfile key.bin --address 0xf000 -o enc/partition-table.bin build/partition_table/partition-table.bin

python %IDF_PATH%\components\esptool_py\esptool\espsecure.py encrypt_flash_data --keyfile key.bin --address 0x20000 -o enc/v7_criptografia_flash.bin build/v7_criptografia_flash.bin
```

Para carregar os dados no dispositivo:  

``` bat
python %IDF_PATH%\components\esptool_py\esptool\esptool.py -p COM4 -b 460800 --before default_reset --after no_reset --chip esp32  write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x1000 enc\bootloader.bin 0xf000 enc\partition-table.bin 0x20000 enc\v7_criptografia_flash.bin --force
```