# Copyright 2007-2021 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.








GMSEC_HOME=../../..
include $(GMSEC_HOME)/config/$(GMSEC_PLATFORM)

TARGET = gmsub_disp

default: $(BINDIR)/$(TARGET)

clean:
	rm -f $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): gmsub_disp.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

