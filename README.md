# LinuxPoEDriver

Linux LKM for Rugged Science models equipped with software controlled PoE ports.  
Will create a folder under /sys/kernel/poe for each controllable port.  

Each folder will contain a 'mode' and 'state' file.  

There are two modes that can be set. (The device supports 4 but the other two have not yet been implemented)  
1 = manual operation. Must use state file to control the port.  
3 = auto operation. Port will automatically detect when a PD is plugged in and supply power.  

There are two states as well.  
0 = Disabled. No power supplied to the port, no matter what the current mode is.  
1 = Enabled. Power will be supplied to the port based on the current mode.  

Currently there is one known issue where setting the state to 0 (disabled) while the mode is 3 (auto) will disable power to the port.  
But setting the state back to 1 (enabled) will not re-enable power to the port.  
The workaround is to set the mode to 1 (manual) and back to 3 (auto). This will re-enable power to the port.

