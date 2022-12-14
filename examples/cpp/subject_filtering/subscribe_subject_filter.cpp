/*
 * Copyright 2007-2021 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */


/** 
 * @file subscribe_subject_filter.cpp
 * 
 * This file contains a simple example outlining how to subscribe
 * to and retrieve messages from the middleware bus while using subject
 * exclusion filtering to reduce the scope of a wildcarded subject subscription.
 *
 * This example program is intended to be run before starting the
 * 'publish_subject_filter' example program.
 */

#include <gmsec4_cpp.h>

#include <string>
#include <iostream>

using namespace gmsec::api;
using namespace gmsec::api::mist;

const char* EXAMPLE_SUBSCRIPTION_SUBJECT = "GMSEC.>";
const char* FILTERED_MESSAGE_SUBJECT     = "GMSEC.TEST.FILTERED";

//o Helper function
void initializeLogging(Config& config);

int main (int argc, char* argv[])
{
	if (argc <= 1)
	{
		std::cout << "usage: " << argv[0] << " mw-id=<middleware ID>" << std::endl;
		return -1;
	}

	//o Load the command-line input into a GMSEC Config object
	// A Config object is basically a key-value pair map which
	// is used to pass configuration options into objects such
	// as Connection objects, ConnectionManager objects, Subscribe
	// and Publish calls, Message objects, etc.
	Config config(argc, argv);

	//o If it was not specified in the command-line arguments, set LOGLEVEL
	// to 'INFO' and LOGFILE to 'stdout' to allow the program report output
	// on the terminal/command line
	initializeLogging(config);
	
	//o Print the GMSEC API version number using the GMSEC Logging
	// interface
	// This is useful for determining which version of the API is
	// configured within the environment
	GMSEC_INFO << ConnectionManager::getAPIVersion();

	try
	{
		//o Create a ConnectionManager object
		// This is the linchpin for all communications between the
		// GMSEC API and the middleware server
		ConnectionManager connMgr(config);

		//o Open the connection to the middleware
		GMSEC_INFO << "Opening the connection to the middleware server";
		connMgr.initialize();

		//o Output middleware client library version
		GMSEC_INFO << connMgr.getLibraryVersion();

		//o Set up a subscription to listen for Messages with the topic
		// "GMSEC.TEST.PUBLISH" which are published to the middleware
		// bus
		// Subscription subject wildcard syntax:
		// * - Matches any one token separated by periods in a subject
		// > - Reading left to right, matches everything up to and ONE
		//     or more tokens in a subject
		// + - Reading left to right, matches everything up to and ZERO 
		//     or more tokens in a subject
		// For more information on wildcards, please see the GMSEC API
		// User's Guide
		GMSEC_INFO << "Subscribing to the topic: " << EXAMPLE_SUBSCRIPTION_SUBJECT;
		connMgr.subscribe(EXAMPLE_SUBSCRIPTION_SUBJECT);

		//o Add a specific message subject to the Connection's exclusion
		// filter
		GMSEC_INFO << "Adding a filter rule for the topic: " << FILTERED_MESSAGE_SUBJECT;
		connMgr.excludeSubject(FILTERED_MESSAGE_SUBJECT);

		//o Call receive() to synchronously retrieve a message that has
		// been received by the middleware client libraries
		// Timeout periods:
		// -1 - Wait forever
		//  0 - Return immediately
		// >0 - Time in milliseconds before timing out
		GMSEC_INFO << "Waiting to receive a Message";
		Message* message = connMgr.receive(-1);

		// Example error handling for calling receive() with a timeout
		if (message != NULL)
		{
			GMSEC_INFO << "Received message:\n" << message->toXML();
		}

		GMSEC_INFO << "Waiting 5 seconds to demonstrate that a second message will not be received";
		message = connMgr.receive(5000);

		if (message != NULL)
		{
			GMSEC_ERROR << "Unexpectedly received a filtered message:\n" << message->toXML();
		}

		//o Disconnect from the middleware and clean up the Connection
		connMgr.cleanup();
	}
	catch (Exception e)
	{
		GMSEC_ERROR << e.what();
	}
}

void initializeLogging(Config& config)
{
	// If it was not specified in the command-line arguments, set
	// LOGLEVEL to 'INFO' and LOGFILE to 'stdout' to allow the 
	// program report output on the terminal/command line
	const char* logLevel  = config.getValue("LOGLEVEL");
	const char* logFile   = config.getValue("LOGFILE");

	if (!logLevel)
	{
		config.addValue("LOGLEVEL", "INFO");
	}
	if (!logFile)
	{
		config.addValue("LOGFILE", "STDERR");
	}
}

