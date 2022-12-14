/*
 * Copyright 2007-2021 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */

#include "MBWire.h"

#include <gmsec4/internal/Encoder.h>
#include <gmsec4/internal/StringUtil.h>

#include <gmsec4/Exception.h>

#include <gmsec4/util/Log.h>
#include <gmsec4/util/StdUniquePtr.h>

#include <string>


using namespace gmsec::api;
using namespace gmsec::api::internal;
using namespace gmsec::api::util;

using namespace gmsec_messagebus;


bool MBWire::m_isCompress = false;



class ArrayOwner
{
public:
	ArrayOwner(char* c = NULL)
		: p(c)
	{
	}

	~ArrayOwner()
	{
		delete [] p;
	}

	char* getArray()
	{
		return p;
	}

private:
	// not implemented
	ArrayOwner(const ArrayOwner&);
	ArrayOwner& operator=(const ArrayOwner&);

	char* p;
};



void MBWire::setCompress(bool isCompress)
{
	m_isCompress = isCompress;
}


bool MBWire::isCompress()
{
	return m_isCompress;
}



// this function serializes the message into a binary format
//  MSG format
//
//  |For conpatability|
//  |with the server  |
//  |routing internals|
//  |Should be removed|
//  |once concept is  |
//  |proven           |
//  -------------------
//           |
//  <CMD_PUB>SUBJECT=<subject>[\0][msgtype:1][encoded content]
//
//  The subject is no longer double-quoted since it makes parsing harder.
//  The message contents (fields) are encoded using Encoder.h support.

void MBWire::serialize(const Message& message, char*& data, size_t& size)
{
	// first get the buffer size required to serialize this message
	data = 0;
	size = 0;

	// serialize the content
	char*          content = 0;
	GMSEC_U64      contentSize;
	MessageEncoder encoder;

	try
	{
		encoder.encode(message, contentSize, content);
	}
	catch (Exception& e)
	{
		GMSEC_WARNING << "MBWire::Serialize: content encoding error " << e.what();
		return;
	}

	if (contentSize > 100000000)
	{
		GMSEC_WARNING << "MBWire::Serialize: excessive content size " << long(contentSize);
		return;
	}

	//
	const char* subject    = message.getSubject();
	size_t      subjectLen = StringUtil::stringLength(subject);

	// now allocate memory to hold the serialized message
	// CMD_PUB + "SUBJECT=" + <subject> + '\0' + <type>
	int    preFieldSize = 1 + 8 + subjectLen + 1 + 1;
	size_t bufferSize   = preFieldSize + contentSize;
	char*  buffer       = new char[bufferSize];

	GMSEC_I32 cSize = 0;

	buffer[0] = CMD_PUB;
	cSize += 1;

	StringUtil::copyBytes(buffer + cSize, "SUBJECT=", 8);
	cSize += 8;

	// subject string with null terminator
	StringUtil::copyBytes(buffer + cSize, subject, subjectLen + 1);
	cSize += subjectLen + 1;

	buffer[cSize] = (unsigned char) message.getKind();
	cSize += 1;

	StringUtil::copyBytes(buffer + cSize, content, contentSize);

	data = buffer;
	size = bufferSize;
}


// this function populates the message using the format generated by Serialize()
//  MSG format
//
//  |For conpatability|
//  |with the server  |
//  |routing internals|
//  |Should be removed|
//  |once concept is  |
//  |proven           |
//  -------------------
//           |
//  SUBJECT=<subject>[\0][msgtype:1][encoded content]

#define SUBJECT_START "SUBJECT="

bool MBWire::deserialize(const char* data, int size, gmsec::api::Message*& message, const gmsec::api::Config& msgConfig)
{
	message = 0;

	int index = 0;

	if (data[index] != CMD_PUB)
	{
		GMSEC_DEBUG << "MBWire::deserialize: data[0] != CMD_PUB";
		return false;
	}

	++index;

	// get the message's subject
	size_t subSpace = StringUtil::stringLength(data + index);

	size_t startLen = StringUtil::stringLength(SUBJECT_START);

	if (subSpace < startLen)
	{
		GMSEC_ERROR << "MBWire::deserialize: subSpace is not big enough for " SUBJECT_START;
		return false;
	}

	if (StringUtil::stringCompareCount(data + index, SUBJECT_START, startLen) != 0)
	{
		GMSEC_ERROR << "MBWire::deserialize: subSpace does not start with " SUBJECT_START;
		return false;
	}

	size_t subBegin     = index + startLen;
	size_t subLength    = subSpace - subBegin + 1;
	const char* subject = data + subBegin;

	index += subSpace + 1;

	// get the message's kind
	char msgKind = data[index];
	index += 1;

	// create the GMSEC message; first construct the subject
	std::string tmpSubject(subject, subLength);

	bool result = true;

	try
	{
		StdUniquePtr<Message> gmsecMessage(new Message(tmpSubject.c_str(), static_cast<Message::MessageKind>(msgKind), msgConfig));

		MessageDecoder decoder;

		decoder.decode(*gmsecMessage.get(), size - index, &data[index]);

		message = gmsecMessage.release();
	}
	catch (const Exception& e)
	{
		GMSEC_WARNING << "MBWire::deserialize: unable to create Message: " << e.what();
		result = false;
	}

	return result;
}
