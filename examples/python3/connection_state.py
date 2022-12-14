#!/usr/bin/env python3


# Copyright 2007-2021 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.


""" 
 @file connection_state.py
 
 This file contains an example which demonstrates how to utilize
 EventCallbacks to track the state of the GMSEC Connection and its connection
 to the middleware.
"""

import libgmsec_python3
import sys


class ConnectionStateCallback(libgmsec_python3.ConnectionManagerEventCallback):
    def __init__(self):
        libgmsec_python3.ConnectionManagerEventCallback.__init__(self)

    def on_event(self, connMgr, status, event):
        if (event == libgmsec_python3.Connection.CONNECTION_SUCCESSFUL_EVENT):
            libgmsec_python3.log_info("[on_event]: Connected to the middleware server")
                
        elif (event == libgmsec_python3.Connection.CONNECTION_BROKEN_EVENT):
            libgmsec_python3.log_info("[on_event]: Connection to the middleware lost or terminated")
    
        elif (event == libgmsec_python3.Connection.CONNECTION_RECONNECT_EVENT):
            libgmsec_python3.log_info("[on_event]: Attempting to reestablish the connection to the middleware")

        else:
            libgmsec_python3.log_info("[on_event]: " + status.get());


def main():
    if(len(sys.argv) <= 1):        
        usageMessage = "usage: " + sys.argv[0] + " mw-id=<middleware ID>"
        print(usageMessage)
        return -1


    cb = ConnectionStateCallback()

    # Load the command-line input into a GMSEC Config object
    config = libgmsec_python3.Config(sys.argv)

    # If it was not specified in the command-line arguments, set LOGLEVEL
    # to 'INFO' and LOGFILE to 'stdout' to allow the program report output
    # on the terminal/command line
    initializeLogging(config)

    # Print the GMSEC API version number using the GMSEC Logging
    # interface
    libgmsec_python3.log_info(libgmsec_python3.ConnectionManager.get_API_version())

    try:
        # Create the Connection
        connMgr = libgmsec_python3.ConnectionManager(config)

        # Register the event callback with the connection to catch
        # connection state changes, including:
        # 1. Connection successfully established to middleware
        # 2. Connection broken from middleware
        # 3. Reconnecting to the middleware
        connMgr.register_event_callback(libgmsec_python3.Connection.CONNECTION_SUCCESSFUL_EVENT, cb)
        connMgr.register_event_callback(libgmsec_python3.Connection.CONNECTION_BROKEN_EVENT, cb)
        connMgr.register_event_callback(libgmsec_python3.Connection.CONNECTION_RECONNECT_EVENT, cb)

        # Connect
        connMgr.initialize()

        # Output middleware version
        libgmsec_python3.log_info("Middleware version = " + connMgr.get_library_version())

        # Assuming that the user provided proper reconnection syntax
        # for the corresponding middleware they are using, one could
        # kill the middleware server at this point, then start it back
        # up to demonstrate the EventCallback.on_event() function
        # triggering
            
        # Wait for user input to end the program
        libgmsec_python3.log_info("Waiting for Connection events to occur, press <enter> to exit the program")

        input("")

        # Clean up the ConnectionManager before exiting the program
        connMgr.cleanup()
        
    except libgmsec_python3.GmsecError as e:
        libgmsec_python3.log_error(str(e))
        return -1

    return 0


def initializeLogging(config):
    logLevel = config.get_value("LOGLEVEL")
    logFile  = config.get_value("LOGFILE")

    if (not logLevel):
        config.add_value("LOGLEVEL", "INFO")
      
    if (not logFile):
        config.add_value("LOGFILE", "STDERR")


#
# Main entry point of script
#
if __name__=="__main__":
    sys.exit(main())

