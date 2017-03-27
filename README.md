Callum Colvine
V00789488

About all functionality should be implemented.

It seems possible to have errors if packet loss occurs during initial 2-way handshake.

If the server and client do not start sending from handshake, please re-try. Error
control has been implemented for main file sending.

This program uses multiple threads to send and receive packets.

Runtime should be around 60 seconds.

Testing was done on the included 1_mb_file.bin
