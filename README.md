Detects emulated keyboard presses like keyboard service callback and keybd_event from usermode.
If you have reversed any Windows USB driver you will notice a lot of Etw logging. This is essentially abusing this fact.

This does not work for all keyboards since this only checks ETW logs from UCX. More logs have to be gathered for it to
work on all keyboards.
Make sure the keyboard is on usb 3.0 you might have to change fid_URB_TransferBufferLength to be the right size for your keyboard.
This POC is created very quickly and that's why it requires some manual work.

//Instructions
Never touch the mouse during the testing. Preferably unplug it.
1. Open the program and press F1 to start the keyboard monitoring.
2. Press a few buttons on the keyboard and then press F1 again to stop the monitoring and wait to see if it was simulated or not.

3. https://github.com/Oliver-1-1/EtwKeyboardDetection
