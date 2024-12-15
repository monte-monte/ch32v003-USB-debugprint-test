# Example to test USB-debugprintf functionality on CH32V003
The idea is to use the same ``printf`` function that is used to print to minichlink terminal via SWIO pin but with USB. 
Provided example would output both to minichlink and [WebLink](https://subjectiverealitylabs.com/WeblinkUSB/)'s termial (not at the same time ideally).

# TODO:
- [x] ~~Add usb terminal polling function to minichlink~~
- [ ] Make unlocking DM more realiable and efficient

# To try:
``git clone https://github.com/monte-monte/ch32v003-USB-debugprint-test/ --recursive``

``cd ch32v003-USB-debugprint-test``

Edit ``usb_config.h`` to match your GPIO settings for USB;

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

If you want to run the tool locally, or maybe make changes to it, you can get it here:
https://github.com/Subjective-Reality-Labs/WebLink_USB

# Additions to minichlink
Instead of using web tool, you can try new version of minichlink with USB terminal support. Submodule in this git pulls updated branch, so all you need to do is:

``./ch32v003fun/minichlink/minichlink -kT -c 1209d003`` 

and use it like normal minichlink terminal

# What is it actually about

If you have used [ch32v003fun](https://github.com/cnlohr/ch32v003fun) before, you must have noticed that there is a special way to get ``printf`` prints out of the chip.
By default it uses a single wire programming interface to get those strings to the ``minichlink``'s terminal. You don't need to attach UART and use additional pins for it. Also it's fast.
But you still need a programmer - either WCH-LinkE or a custom ESP32-S2 programmer built by cnlohr. Not a big deal considering you already use it to program CH32 in the first place.

But then, we also have the ability to ditch the programmer completely and use USB to program CH32 thanks to cnlohr's [rv003usb](https://github.com/cnlohr/rv003usb). If you have a board that is designed to use USB as an interface and not only for power ([UIAPduino](https://www.uiap.jp/en/uiapduino/pro-micro/ch32v003/v1dot4) for example), you can now use a single cable to program and run your board.

But what about debugging with ``printf``? Now you have to go back to using UART, or a programmer with SWIO. Not fun. Could we somehow use the same USB port we are now using to program our boards to communicate with it?

Sure, we could use the same ``rv003usb`` library to print debug messages from CH32V003 and then send some commands to it via USB HID. But we would need to make some kind of a protocol and rewrite our application to use it instead of ``printf`` we used before. And then if we decide to connect the programmer, for example for debugging, we won't get any output in minichlink's terminal.

What if we could keep our ``printf`` that is routed via SWIO by default, but route its output to a USB HID when needed? That's exactly what we are doing here!

# How it works, and why it sometimes doesn't

So what are we testing here if the solution is simple and has already been found?

CH32V003 has a built-in Debug Module (further referenced as DM) that is used for programming and debugging via a compatible programmer. cnlohr's ``printf`` that is used by default in ch32v003fun leverages this DM to implement communication between the MCU and a host PC. How does it do this? It uses two debug registers ``DMDATA0`` and ``DMDATA1`` to write data to and read from. It's simple and fast, you can see how it is done [here](https://github.com/cnlohr/ch32v003fun/blob/2491e928d61f4296fe421705624ff3788c7ac1f7/ch32v003fun/ch32v003fun.c#L1623).

Now, what I thought, we could just read ``DMDATA0/1`` internally and send the contents of it via USB HID to a compatible terminal. Then we won't change any logic but only enhance it with our little addition. The pros of doing it this way are that we have both interface useful within same firmware with very little added code, and it becomes mostly portable.

But there is a caveat, I've found once I started to tinker with this idea. DM in CH32V003 is an external module from the perspective of the core that is running our firmware. Until the specific command is sent to the DM via SWIO from a programmer we can't access ``DMDATA0/1`` from the firmware itself. It's not a big deal when used with a programmer, it does everything automatically during interface initialization. But we want to use it without a programmer, only with USB attached.

Can we somehow unlock the DM from inside the firmware? I would like to find a way to access the DM or its registers from the core itself, but after hours of reading manuals, datasheets, and experimenting with code I haven't found anything. If you know a way, please tell me! The only way left is to try to self-program via bit-banging SWIO pin from inside. But! The SWIO pin by default can be used only as an input and is reserved for programming purposes. We can switch it to be a plain GPIO but then it is detached from DM and can't be used to communicate with it. 

So here is a dilemma, but a physical world with its rules comes to a rescue! What we can do is: *Make SWIO GPIO > Turn it 0/1 > Make SWIO programmable pin again > Repeat as needed*. It may induce a panic attack or feeling of unwell in some, but it works! Once the unlock sequence is sent to DM it lets our firmware access both ``DMDATA0/1`` and do all we want. The only issue is that it could potentially crash/halt the core or not work at all. So that's exactly what this repo is meant to test.

The firmware is programmed to send a string ``dmlock`` while it detects that ``DMDATA0/1`` is unavailable. So if you see a continuous ``dmlockmdlockdmlock`` string in the terminal it means that unlock command either failed or wasn't issued. By default unlocking is done automatically on connecting to the terminal, but for debugging purposes, it can be turned to the manual, then ``u`` button will appear in the interface, and pressing it will send the unlock command to the CH32V003.

The other thing to mention is ``T1COEFF`` variable (named for legacy purposes) it is basically a small delay between HIGH and LOW writes to the DM. Debug protocol needs to be timed correctly so tweaking this may be required, but in my testing ``2`` works always, and ``3`` or ``4`` work sometimes. If enough tests will show that ``2`` is the correct value, it could be hardcoded as a constant. For now, it can also be changed in settings and will apply to the next unlocking command.
