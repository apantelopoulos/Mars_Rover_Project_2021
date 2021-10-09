This folder contains the Python code required to run locally to read from the Serial port and send the battery status to the server with the help of a TCP client.

There are also 3 Arduino files, each having a specific use case:
 - PVpaneltest - was used to obtain the information on the IV and PV curves
 - FinalDesignV2 -Is the charging arduino program that was fully completed and tested prior to the battery melting incident. Also the code that needs to be run to send battery percentage to Command
 - FinalDesignDischarging - Is the discharging arduino program , not tested if it works or debugged since i could not use the batteries
