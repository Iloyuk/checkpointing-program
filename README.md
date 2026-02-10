# Compiling the programs
Simply run ```make build```. You must be on Linux for this to work.

# Checkpointing
Run ```./ckpt ./process-to-run```. An example program, ```counter```, has been provided (you will need to ```make build``` first).
To checkpoint the program while it is running, run ```kill -12 [PID]``` -- the process ID will be printed when running the checkpointing process initially. Alternatively, run ```kill -12 `pgrep -n [running-process]` ```

# Restarting
Run ```./restart myckpt.img```. myckpt.img is the image file that will be generated once a process has been checkpointed.

