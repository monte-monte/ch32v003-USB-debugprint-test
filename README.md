# Example to test USB-debugprintf functionality on CH32V003
The idea is to use the same ``printf`` function that is used to print to minichlink terminal via SWIO pin but with USB. 
Provided example would output both to minichlink and [WebLink](https://subjectiverealitylabs.com/WeblinkUSB/)'s termial (not at the same time ideally).

# TODO:
- Add usb terminal polling function to minichlink

# To try:
``git clone https://github.com/monte-monte/ch32v003-USB-debugprint-test/``

``cd ch32v003-USB-debugprint-test``

``git submodule update``

``make``

``make flash``