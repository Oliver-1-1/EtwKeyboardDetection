Detects emulated keyboard presses like keyboard service callback and keybd_event from usermode.
If you have reversed any Windows USB driver you will notice a lot of Etw logging. This is essentially abusing this fact.

This does not work for all keyboards since this only checks ETW logs from UCX. More logs have to be gathers for it to
work on all keyboards.
