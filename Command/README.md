Command Subsystem
=======
The command subsystem consists of a webpage based on HTML/CSS/JavaScript that allows the control of the rover via a TCP server that runs on node JS.


Setting up the server
---------------------

1. Connect to the remote server you are using. (AWS, Azure, Google Cloud)
2. Install Node.js with the command:
	
	a. `sudo apt install nodejs`
	
3. All necessary packages are included in the github repo

4. Run the server with the command: 
	
	a. `node index.js`

5. The webpage is hosted on port 3000, so simply access <IP_ADDESS>:3000 from a browser to get access to the webpage
