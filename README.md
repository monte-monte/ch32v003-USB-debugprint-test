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

If you're on linux, you may need to add udev rule:

```
echo -ne "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", MODE=\"0664\", GROUP=\"plugdev\"\n" | sudo tee /etc/udev/rules.d/20-hidraw.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```
For this to work you need to use Google Chrome, or any Chromium-based browser (currently it doesn't work on Android, sadly).

Go to https://subjectiverealitylabs.com/WeblinkUSB/ and press ``t`` button in the UI.
There are relevant terminal settings that can be reached by pressing cog button in the top right corner and selecting ``Terminal`` section.

(If you hate animations, there is also an option to disable them)

<img src="https://github.com/user-attachments/assets/f6c79832-5721-48b6-a186-eb1d9c997e35" width="500px">

If you see continuous string of ``dmlock`` try pressing ``u`` button in the UI. It should unlock Debug Module and you should see the terminal output as shown above. If not, please share you results, so we can figure out universal unlocking procedure.

If you want to run the tool locally, or maybe make changes to it, you can get it here:
https://github.com/Subjective-Reality-Labs/WebLink_USB
