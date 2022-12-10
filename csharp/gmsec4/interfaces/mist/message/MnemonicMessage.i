/*
 * Copyright 2007-2021 United States Government as represented by the
 * Administrator of The National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S. Code.
 * All Rights Reserved.
 */

%rename(MnemonicMessage) gmsec::api::mist::message::MnemonicMessage;

%module MnemonicMessage

%{
#include <gmsec4/mist/message/MnemonicMessage.h>
using namespace gmsec::api::mist::message;
%}

%ignore gmsec::api::mist::message::MnemonicMessage::operator=(const MnemonicMessage&);

/* Ignore deprecated methods */
%ignore gmsec::api::mist::message::MnemonicMessage::getNumMnemonics() const;
%ignore gmsec::api::mist::message::MnemonicMessage::getMnemonic(size_t) const;

%rename("AddMnemonic") addMnemonic;
%rename("GetMnemonicIterator") getMnemonicIterator;
%rename("ConvertMessage") convertMessage;

%include <gmsec4/util/wdllexp.h>
%include <gmsec4/mist/message/MnemonicMessage.h>
